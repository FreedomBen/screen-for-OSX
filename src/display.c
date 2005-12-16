/* Copyright (c) 1993
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 ****************************************************************
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")


#include <sys/types.h>
#include <fcntl.h>

#include "config.h"
#include "screen.h"
#include "extern.h"
#include "braille.h"

static int  CountChars __P((int));
static int  PutChar __P((int));
static int  BlankResize __P((int, int));
static int  CallRewrite __P((int, int, int, int));
static void FreeCanvas __P((struct canvas *));
static void disp_readev_fn __P((struct event *, char *));
static void disp_writeev_fn __P((struct event *, char *));
static void disp_status_fn __P((struct event *, char *));
static void disp_hstatus_fn __P((struct event *, char *));
static void cv_winid_fn __P((struct event *, char *));
#ifdef MAPKEYS
static void disp_map_fn __P((struct event *, char *));
#endif
static void WriteLP __P((int, int));
static void  INSERTCHAR __P((int));
static void  RAW_PUTCHAR __P((int));


extern struct layer *flayer;
extern struct win *windows;
extern struct LayFuncs WinLf;

extern int  use_hardstatus;
extern int  MsgWait, MsgMinWait;
extern int  Z0width, Z1width;
extern char *blank, *null;
extern struct mline mline_blank, mline_null;
extern struct mchar mchar_null, mchar_blank, mchar_so;

/* XXX shouldn't be here */
extern char *hstatusstring;
extern char *captionstring;

/*
 * tputs needs this to calculate the padding
 */
#ifndef NEED_OSPEED
extern
#endif /* NEED_OSPEED */
short ospeed;


struct display *display, *displays;

#ifndef MULTI
struct display TheDisplay;
#endif

/*
 *  The default values
 */
int defobuflimit = OBUF_MAX;
#ifdef AUTO_NUKE
int defautonuke = 0;
#endif
int captionalways;
int hardstatusemu = HSTATUS_IGNORE;

/*
 *  Default layer management
 */

void
DefProcess(bufp, lenp)
char **bufp;
int *lenp;
{
  *bufp += *lenp;
  *lenp = 0;
}

void
DefRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  if (isblank == 0 && y >= 0)
    DefClearLine(y, xs, xe);
}

void
DefClearLine(y, xs, xe)
int y, xs, xe;
{
  LClearLine(flayer, y, xs, xe, (struct mline *)0);
}

/*ARGSUSED*/
int
DefRewrite(y, xs, xe, rend, doit)
int y, xs, xe, doit;
struct mchar *rend;
{
  return EXPENSIVE;
}

/*ARGSUSED*/
int
DefResize(wi, he)
int wi, he;
{
  return -1;
}

void
DefRestore()
{
  LAY_DISPLAYS(flayer, InsertMode(0));
  /* ChangeScrollRegion(0, D_height - 1); */
  LKeypadMode(flayer, 0);
  LCursorkeysMode(flayer, 0);
  LCursorVisibility(flayer, 0);
  LSetRendition(flayer, &mchar_null);
  LSetFlow(flayer, FLOW_NOW);
}

/*
 *  Blank layer management
 */

struct LayFuncs BlankLf =
{
  DefProcess,
  0,
  DefRedisplayLine,
  DefClearLine,
  DefRewrite,
  BlankResize,
  DefRestore
};

/*ARGSUSED*/
static int
BlankResize(wi, he)
int wi, he;
{
  flayer->l_width = wi;
  flayer->l_height = he;
  return 0;
}


/*
 *  Generate new display, start with a blank layer.
 *  The termcap arrays are not initialised here.
 *  The new display is placed in the displays list.
 */

struct display *
MakeDisplay(uname, utty, term, fd, pid, Mode)
char *uname, *utty, *term;
int fd, pid;
struct mode *Mode;
{
  struct user **u;
  struct baud_values *b;

  if (!*(u = FindUserPtr(uname)) && UserAdd(uname, (char *)0, u))
    return 0;	/* could not find or add user */

#ifdef MULTI
  if ((display = (struct display *)calloc(1, sizeof(*display))) == 0)
    return 0;
#else
  if (displays)
    return 0;
  display = &TheDisplay;
#endif
  display->d_next = displays;
  displays = display;
  D_flow = 1;
  D_nonblock = 0;
  D_userfd = fd;
  D_readev.fd = D_writeev.fd = fd;
  D_readev.type  = EV_READ;
  D_writeev.type = EV_WRITE;
  D_readev.data = D_writeev.data = (char *)display;
  D_readev.handler  = disp_readev_fn;
  D_writeev.handler = disp_writeev_fn;
  evenq(&D_readev);
  D_writeev.condpos = &D_obuflen;
  D_writeev.condneg = &D_obuffree;
  evenq(&D_writeev);
  D_statusev.type = EV_TIMEOUT;
  D_statusev.data = (char *)display;
  D_statusev.handler = disp_status_fn;
  D_hstatusev.type = EV_TIMEOUT;
  D_hstatusev.data = (char *)display;
  D_hstatusev.handler = disp_hstatus_fn;
#ifdef MAPKEYS
  D_mapev.type = EV_TIMEOUT;
  D_mapev.data = (char *)display;
  D_mapev.handler = disp_map_fn;
#endif
  D_OldMode = *Mode;
  Resize_obuf();  /* Allocate memory for buffer */
  D_obufmax = defobuflimit;
  D_obuflenmax = D_obuflen - D_obufmax;
#ifdef AUTO_NUKE
  D_auto_nuke = defautonuke;
#endif
  D_obufp = D_obuf;
  D_printfd = -1;
  D_userpid = pid;

#ifdef POSIX
  if ((b = lookup_baud((int)cfgetospeed(&D_OldMode.tio))))
    D_dospeed = b->idx;
#else
# ifdef TERMIO
  if ((b = lookup_baud(D_OldMode.tio.c_cflag & CBAUD)))
    D_dospeed = b->idx;
# else
  D_dospeed = (short)D_OldMode.m_ttyb.sg_ospeed;
# endif
#endif

  debug1("New displays ospeed = %d\n", D_dospeed);
  strncpy(D_usertty, utty, sizeof(D_usertty) - 1);
  D_usertty[sizeof(D_usertty) - 1] = 0;
  strncpy(D_termname, term, sizeof(D_termname) - 1);
  D_termname[sizeof(D_termname) - 1] = 0;
  D_user = *u;
  D_processinput = ProcessInput;
  return display;
}


void
FreeDisplay()
{
  struct win *p;
  struct canvas *cv, *cvp;
#ifdef MULTI
  struct display *d, **dp;
#endif

#ifdef FONT
  FreeTransTable();
#endif
  if (D_userfd >= 0)
    {
      Flush();
      SetTTY(D_userfd, &D_OldMode);
      fcntl(D_userfd, F_SETFL, 0);
    }
  freetty();
  if (D_tentry)
    free(D_tentry);
  D_tentry = 0;
  if (D_processinputdata)
    free(D_processinputdata);
  D_processinputdata = 0;
  D_tcinited = 0;
  evdeq(&D_hstatusev);
  evdeq(&D_statusev);
  evdeq(&D_readev);
  evdeq(&D_writeev);
#ifdef MAPKEYS
  evdeq(&D_mapev);
#endif
#ifdef HAVE_BRAILLE
  if (bd.bd_dpy == display)
    {
      bd.bd_start_braille = 0;
      StartBraille();
    }
#endif

#ifdef MULTI
  for (dp = &displays; (d = *dp) ; dp = &d->d_next)
    if (d == display)
      break;
  ASSERT(d);
  if (D_status_lastmsg)
    free(D_status_lastmsg);
  if (D_obuf)
    free(D_obuf);
  *dp = display->d_next;
  cv = display->d_cvlist;
#else /* MULTI */
  ASSERT(display == displays);
  ASSERT(display == &TheDisplay);
  cv = display->d_cvlist;
  display->d_cvlist = 0;
  displays = 0;
#endif /* MULTI */

  for (p = windows; p; p = p->w_next)
    {
      if (p->w_pdisplay == display)
	p->w_pdisplay = 0;
    }
  for (; cv; cv = cvp)
    {
      cvp = cv->c_next;
      FreeCanvas(cv);
    }
#ifdef MULTI
  free((char *)display);
#endif
  display = 0;
}

int
MakeDefaultCanvas()
{
  struct canvas *cv;
 
  ASSERT(display);
  if ((cv = (struct canvas *)calloc(1, sizeof *cv)) == 0)
    return -1;
  cv->c_xs      = 0;
  cv->c_xe      = D_width - 1;
  cv->c_ys      = 0;
  cv->c_ye      = D_height - 1 - (D_has_hstatus == HSTATUS_LASTLINE) - captionalways;
  cv->c_xoff    = 0;
  cv->c_yoff    = 0;
  cv->c_next = 0;
  cv->c_display = display;
  cv->c_vplist = 0;
  cv->c_captev.type = EV_TIMEOUT;
  cv->c_captev.data = (char *)cv;
  cv->c_captev.handler = cv_winid_fn;

  cv->c_blank.l_cvlist = cv;
  cv->c_blank.l_width = cv->c_xe - cv->c_xs + 1;
  cv->c_blank.l_height = cv->c_ye - cv->c_ys + 1;
  cv->c_blank.l_x = cv->c_blank.l_y = 0;
  cv->c_blank.l_layfn = &BlankLf;
  cv->c_blank.l_data = 0;
  cv->c_blank.l_next = 0;
  cv->c_blank.l_bottom = &cv->c_blank;
  cv->c_blank.l_blocking = 0;
  cv->c_layer = &cv->c_blank;
  cv->c_lnext = 0;

  D_cvlist = cv;
  RethinkDisplayViewports();
  D_forecv = cv; 	      /* default input focus */
  return 0;
}

void
FreeCanvas(cv)
struct canvas *cv;
{
  struct viewport *vp, *nvp;
  struct win *p;

  p = Layer2Window(cv->c_layer);
  SetCanvasWindow(cv, 0);
  if (p)
    WindowChanged(p, 'u');
  if (flayer == cv->c_layer)
    flayer = 0;
  for (vp = cv->c_vplist; vp; vp = nvp)
    {
      vp->v_canvas = 0;
      nvp = vp->v_next;
      vp->v_next = 0;
      free(vp);
    }
  evdeq(&cv->c_captev);
  free(cv);
}

int
AddCanvas()
{
  int hh, h, i, j;
  struct canvas *cv, **cvpp;

  for (cv = D_cvlist, j = 0; cv; cv = cv->c_next)
    j++;
  j++;	/* new canvas */
  h = D_height - (D_has_hstatus == HSTATUS_LASTLINE);
  if (h / j <= 1)
    return -1;

  for (cv = D_cvlist; cv; cv = cv->c_next)
    if (cv == D_forecv)
      break;
  ASSERT(cv);
  cvpp = &cv->c_next;

  if ((cv = (struct canvas *)calloc(1, sizeof *cv)) == 0)
    return -1;

  cv->c_xs      = 0;
  cv->c_xe      = D_width - 1;
  cv->c_ys      = 0;
  cv->c_ye      = D_height - 1;
  cv->c_xoff    = 0;
  cv->c_yoff    = 0;
  cv->c_display = display;
  cv->c_vplist  = 0;
  cv->c_captev.type = EV_TIMEOUT;
  cv->c_captev.data = (char *)cv;
  cv->c_captev.handler = cv_winid_fn;

  cv->c_blank.l_cvlist = cv;
  cv->c_blank.l_width = cv->c_xe - cv->c_xs + 1;
  cv->c_blank.l_height = cv->c_ye - cv->c_ys + 1;
  cv->c_blank.l_x = cv->c_blank.l_y = 0;
  cv->c_blank.l_layfn = &BlankLf;
  cv->c_blank.l_data = 0;
  cv->c_blank.l_next = 0;
  cv->c_blank.l_bottom = &cv->c_blank;
  cv->c_blank.l_blocking = 0;
  cv->c_layer = &cv->c_blank;
  cv->c_lnext = 0;

  cv->c_next    = *cvpp;
  *cvpp = cv;

  i = 0;
  for (cv = D_cvlist; cv; cv = cv->c_next)
    {
      hh = h / j-- - 1;
      cv->c_ys = i;
      cv->c_ye = i + hh - 1;
      cv->c_yoff = i;
      i += hh + 1;
      h -= hh + 1;
    }

  RethinkDisplayViewports();
  ResizeLayersToCanvases();
  return 0;
}

void
RemCanvas()
{
  int hh, h, i, j;
  struct canvas *cv, **cvpp;
  int did = 0;

  h = D_height - (D_has_hstatus == HSTATUS_LASTLINE);
  for (cv = D_cvlist, j = 0; cv; cv = cv->c_next)
    j++;
  if (j == 1)
    return;
  i = 0;
  j--;
  for (cvpp = &D_cvlist; (cv = *cvpp); cvpp = &cv->c_next)
    {
      if (cv == D_forecv && !did)
	{
	  *cvpp = cv->c_next;
	  FreeCanvas(cv);
	  cv = *cvpp;
	  D_forecv = cv ? cv : D_cvlist;
	  D_fore = Layer2Window(D_forecv->c_layer);
	  flayer = D_forecv->c_layer;
	  if (cv == 0)
	    break;
	  did = 1;
	}
      hh = h / j-- - 1;
      if (!captionalways && i == 0 && j == 0)
	hh++;
      cv->c_ys = i;
      cv->c_ye = i + hh - 1;
      cv->c_yoff = i;
      i += hh + 1;
      h -= hh + 1;
    }
  RethinkDisplayViewports();
  ResizeLayersToCanvases();
}

void
OneCanvas()
{
  struct canvas *mycv = D_forecv;
  struct canvas *cv, **cvpp;

  for (cvpp = &D_cvlist; (cv = *cvpp);)
    {
      if (cv == mycv)
        {
	  cv->c_ys = 0;
	  cv->c_ye = D_height - 1 - (D_has_hstatus == HSTATUS_LASTLINE) - captionalways;
	  cv->c_yoff = 0;
	  cvpp = &cv->c_next;
        }
      else
        {
	  *cvpp = cv->c_next;
	  FreeCanvas(cv);
        }
    }
  RethinkDisplayViewports();
  ResizeLayersToCanvases();
}

int
RethinkDisplayViewports()
{
  struct canvas *cv;
  struct viewport *vp, *vpn;

  /* free old viewports */
  for (cv = display->d_cvlist; cv; cv = cv->c_next)
    {
      for (vp = cv->c_vplist; vp; vp = vpn)
	{
	  vp->v_canvas = 0;
	  vpn = vp->v_next;
          bzero((char *)vp, sizeof(*vp));
          free(vp);
	}
      cv->c_vplist = 0;
    }
  display->d_vpxmin = -1;
  display->d_vpxmax = -1;

  for (cv = display->d_cvlist; cv; cv = cv->c_next)
    {
      if ((vp = (struct viewport *)malloc(sizeof *vp)) == 0)
	return -1;
#ifdef HOLE
      vp->v_canvas = cv;
      vp->v_xs = cv->c_xs;
      vp->v_ys = (cv->c_ys + cv->c_ye) / 2;
      vp->v_xe = cv->c_xe;
      vp->v_ye = cv->c_ye;
      vp->v_xoff = cv->c_xoff;
      vp->v_yoff = cv->c_yoff;
      vp->v_next = cv->c_vplist;
      cv->c_vplist = vp;

      if ((vp = (struct viewport *)malloc(sizeof *vp)) == 0)
	return -1;
      vp->v_canvas = cv;
      vp->v_xs = (cv->c_xs + cv->c_xe) / 2;
      vp->v_ys = (3 * cv->c_ys + cv->c_ye) / 4;
      vp->v_xe = cv->c_xe;
      vp->v_ye = (cv->c_ys + cv->c_ye) / 2 - 1;
      vp->v_xoff = cv->c_xoff;
      vp->v_yoff = cv->c_yoff;
      vp->v_next = cv->c_vplist;
      cv->c_vplist = vp;

      if ((vp = (struct viewport *)malloc(sizeof *vp)) == 0)
	return -1;
      vp->v_canvas = cv;
      vp->v_xs = cv->c_xs;
      vp->v_ys = (3 * cv->c_ys + cv->c_ye) / 4;
      vp->v_xe = (3 * cv->c_xs + cv->c_xe) / 4 - 1;
      vp->v_ye = (cv->c_ys + cv->c_ye) / 2 - 1;
      vp->v_xoff = cv->c_xoff;
      vp->v_yoff = cv->c_yoff;
      vp->v_next = cv->c_vplist;
      cv->c_vplist = vp;

      if ((vp = (struct viewport *)malloc(sizeof *vp)) == 0)
	return -1;
      vp->v_canvas = cv;
      vp->v_xs = cv->c_xs;
      vp->v_ys = cv->c_ys;
      vp->v_xe = cv->c_xe;
      vp->v_ye = (3 * cv->c_ys + cv->c_ye) / 4 - 1;
      vp->v_xoff = cv->c_xoff;
      vp->v_yoff = cv->c_yoff;
      vp->v_next = cv->c_vplist;
      cv->c_vplist = vp;
#else
      vp->v_canvas = cv;
      vp->v_xs = cv->c_xs;
      vp->v_ys = cv->c_ys;
      vp->v_xe = cv->c_xe;
      vp->v_ye = cv->c_ye;
      vp->v_xoff = cv->c_xoff;
      vp->v_yoff = cv->c_yoff;
      vp->v_next = cv->c_vplist;
      cv->c_vplist = vp;
#endif

      if (cv->c_xs < display->d_vpxmin || display->d_vpxmin == -1)
        display->d_vpxmin = cv->c_xs;
      if (cv->c_xe > display->d_vpxmax || display->d_vpxmax == -1)
        display->d_vpxmax = cv->c_xe;
    }
  return 0;
}

void
RethinkViewportOffsets(cv)
struct canvas *cv;
{
  struct viewport *vp;

  for (vp = cv->c_vplist; vp; vp = vp->v_next)
    {
      vp->v_xoff = cv->c_xoff;
      vp->v_yoff = cv->c_yoff;
    }
}

/*
 * if the adaptflag is on, we keep the size of this display, else
 * we may try to restore our old window sizes.
 */
void
InitTerm(adapt)
int adapt;
{
  ASSERT(display);
  ASSERT(D_tcinited);
  D_top = D_bot = -1;
  PutStr(D_TI);
  PutStr(D_IS);
  /* Check for toggle */
  if (D_IM && strcmp(D_IM, D_EI))
    PutStr(D_EI);
  D_insert = 0;
#ifdef MAPKEYS
  PutStr(D_KS);
  PutStr(D_CCS);
#else
  /* Check for toggle */
  if (D_KS && strcmp(D_KS, D_KE))
    PutStr(D_KE);
  if (D_CCS && strcmp(D_CCS, D_CCE))
    PutStr(D_CCE);
#endif
  D_keypad = 0;
  D_cursorkeys = 0;
  PutStr(D_ME);
  PutStr(D_EA);
  PutStr(D_CE0);
  D_rend = mchar_null;
  D_atyp = 0;
  if (adapt == 0)
    ResizeDisplay(D_defwidth, D_defheight);
  ChangeScrollRegion(0, D_height - 1);
  D_x = D_y = 0;
  Flush();
  ClearDisplay();
  debug1("we %swant to adapt all our windows to the display\n", 
	 (adapt) ? "" : "don't ");
  /* In case the size was changed by a init sequence */
  CheckScreenSize((adapt) ? 2 : 0);
}

void
FinitTerm()
{
  ASSERT(display);
  if (D_tcinited)
    {
      ResizeDisplay(D_defwidth, D_defheight);
      InsertMode(0);
      ChangeScrollRegion(0, D_height - 1);
      KeypadMode(0);
      CursorkeysMode(0);
      CursorVisibility(0);
      SetRendition(&mchar_null);
      SetFlow(FLOW_NOW);
#ifdef MAPKEYS
      PutStr(D_KE);
      PutStr(D_CCE);
#endif
      if (D_hstatus)
	ShowHStatus((char *)0);
      D_x = D_y = -1;
      GotoPos(0, D_height - 1);
      AddChar('\r');
      AddChar('\n');
      PutStr(D_TE);
    }
  Flush();
}


static void
INSERTCHAR(c)
int c;
{
  ASSERT(display);
  if (!D_insert && D_x < D_width - 1)
    {
      if (D_IC || D_CIC)
	{
	  if (D_IC)
	    PutStr(D_IC);
	  else
	    CPutStr(D_CIC, 1);
	  RAW_PUTCHAR(c);
	  return;
	}
      InsertMode(1);
      if (!D_insert)
	{
          RefreshLine(D_y, D_x, D_width-1, 0);
	  return;
	}
    }
  RAW_PUTCHAR(c);
}

void
PUTCHAR(c)
int c;
{
  ASSERT(display);
  if (D_insert && D_x < D_width - 1)
    InsertMode(0);
  RAW_PUTCHAR(c);
}

void
PUTCHARLP(c)
int c;
{
  if (D_x < D_width - 1)
    {
      if (D_insert)
	InsertMode(0);
      RAW_PUTCHAR(c);
      return;
    }
  if (D_CLP || D_y != D_bot)
    {
      RAW_PUTCHAR(c);
      return;
    }
  D_lp_missing = 1;
  D_rend.image = c;
  D_lpchar = D_rend;
#ifdef KANJI
  D_lp_mbcs = D_mbcs;
  D_mbcs = 0;
#endif
}

/*
 * RAW_PUTCHAR() is for all text that will be displayed.
 * NOTE: charset Nr. 0 has a conversion table, but c1, c2, ... don't.
 */

STATIC void
RAW_PUTCHAR(c)
int c;
{
  ASSERT(display);

#ifdef FONT
# ifdef KANJI
  if (D_rend.font == KANJI)
    {
      int t = c;
      if (D_mbcs == 0)
	{
	  D_mbcs = c;
	  D_x++;
	  return;
	}
      D_x--;
      if (D_x == D_width - 1)
	D_x += D_AM ? 1 : -1;
      c = D_mbcs;
      c &= 0x7f;
      t &= 0x7f;
      if (D_kanji == EUC)
	{
	  c |= 0x80;
	  t |= 0x80;
	}
      else if (D_kanji == SJIS)
	{
	  t += (c & 1) ? ((t <= 0x5f) ? 0x1f : 0x20) : 0x7e;
	  c = (c - 0x21) / 2 + ((c < 0x5f) ? 0x81 : 0xc1);
	}
      D_mbcs = t;
    }
  else if (D_rend.font == KANA)
    {
      if (D_kanji == EUC)
	{
	  AddChar(0x8e);	/* SS2 */
	  c |= 0x80;
	}
      else if (D_kanji == SJIS)
	c |= 0x80;
    }
  kanjiloop:
# endif
  if (D_xtable && D_xtable[(int)(unsigned char)D_rend.font] && D_xtable[(int)(unsigned char)D_rend.font][(int)(unsigned char)c])
    AddStr(D_xtable[(int)(unsigned char)D_rend.font][(int)(unsigned char)c]);
  else
    AddChar(D_rend.font != '0' ? c : D_c0_tab[(int)(unsigned char)c]);
#else /* FONT */
    AddChar(c);
#endif /* FONT */

  if (++D_x >= D_width)
    {
      if (D_AM == 0)
        D_x = D_width - 1;
      else if (!D_CLP || D_x > D_width)
	{
	  D_x -= D_width;
	  if (D_y < D_height-1 && D_y != D_bot)
	    D_y++;
	}
    }
#ifdef KANJI
  if (D_mbcs)
    {
      c = D_mbcs;
      D_mbcs = 0;
      goto kanjiloop;
    }
#endif
}

static int
PutChar(c)
int c;
{
  /* this PutChar for ESC-sequences only (AddChar is a macro) */
  AddChar(c);
  return c;
}

void
PutStr(s)
char *s;
{
  if (display && s)
    {
      ospeed = D_dospeed;
      tputs(s, 1, PutChar);
    }
}

void
CPutStr(s, c)
char *s;
int c;
{
  if (display && s)
    {
      ospeed = D_dospeed;
      tputs(tgoto(s, 0, c), 1, PutChar);
    }
}


/* Insert mode is a toggle on some terminals, so we need this hack:
 */
void
InsertMode(on)
int on;
{
  if (display && on != D_insert && D_IM)
    {
      D_insert = on;
      if (on)
	PutStr(D_IM);
      else
	PutStr(D_EI);
    }
}

/* ...and maybe keypad application mode is a toggle, too:
 */
void
KeypadMode(on)
int on;
{
#ifdef MAPKEYS
  if (display)
    D_keypad = on;
#else
  if (display && D_keypad != on && D_KS)
    {
      D_keypad = on;
      if (on)
	PutStr(D_KS);
      else
	PutStr(D_KE);
    }
#endif
}

void
CursorkeysMode(on)
int on;
{
#ifdef MAPKEYS
  if (display)
    D_cursorkeys = on;
#else
  if (display && D_cursorkeys != on && D_CCS)
    {
      D_cursorkeys = on;
      if (on)
	PutStr(D_CCS);
      else
	PutStr(D_CCE);
    }
#endif
}

void
ReverseVideo(on)
int on;
{
  if (display && D_revvid != on && D_CVR)
    {
      D_revvid = on;
      if (D_revvid)
	PutStr(D_CVR);
      else
	PutStr(D_CVN);
    }
}

void
CursorVisibility(v)
int v;
{
  if (display && D_curvis != v)
    {
      if (D_curvis)
	PutStr(D_VE);		/* do this always, just to be safe */
      D_curvis = 0;
      if (v == -1 && D_VI)
	PutStr(D_VI);
      else if (v == 1 && D_VS)
	PutStr(D_VS);
      else
	return;
      D_curvis = v;
    }
}

static int StrCost;

/* ARGSUSED */
static int
CountChars(c)
int c;
{
  StrCost++;
  return c;
}

int
CalcCost(s)
register char *s;
{
  ASSERT(display);
  if (s)
    {
      StrCost = 0;
      ospeed = D_dospeed;
      tputs(s, 1, CountChars);
      return StrCost;
    }
  else
    return EXPENSIVE;
}

static int
CallRewrite(y, xs, xe, doit)
int y, xs, xe, doit;
{
  struct canvas *cv, *cvlist, *cvlnext;
  struct viewport *vp;
  struct layer *oldflayer;
  int cost;

  debug3("CallRewrite %d %d %d\n", y, xs, xe);
  ASSERT(display);
  ASSERT(xe >= xs);

  vp = 0;
  for (cv = D_cvlist; cv; cv = cv->c_next)
    {
      if (y < cv->c_ys || y > cv->c_ye || xe < cv->c_xs || xs > cv->c_xe)
	continue;
      for (vp = cv->c_vplist; vp; vp = vp->v_next)
	if (y >= vp->v_ys && y <= vp->v_ye && xe >= vp->v_xs && xs <= vp->v_xe)
	  break;
      if (vp)
	break;
    }
  if (doit)
    {
      oldflayer = flayer;
      flayer = cv->c_layer;
      cvlist = flayer->l_cvlist;
      cvlnext = cv->c_lnext;
      flayer->l_cvlist = cv;
      cv->c_lnext = 0;
      Rewrite(y - vp->v_yoff, xs - vp->v_xoff, xe - vp->v_xoff, &D_rend, 1);
      flayer->l_cvlist = cvlist;
      cv->c_lnext = cvlnext;
      flayer = oldflayer;
      return 0;
    }
  if (cv == 0 || cv->c_layer == 0)
    return EXPENSIVE;	/* not found or nothing on it */
  if (xs < vp->v_xs || xe > vp->v_xe)
    return EXPENSIVE;	/* crosses viewport boundaries */
  if (y - vp->v_yoff < 0 || y - vp->v_yoff >= cv->c_layer->l_height)
    return EXPENSIVE;	/* line not on layer */
  if (xs - vp->v_xoff < 0 || xe - vp->v_xoff >= cv->c_layer->l_width)
    return EXPENSIVE;	/* line not on layer */
  oldflayer = flayer;
  flayer = cv->c_layer;
  debug3("Calling Rewrite %d %d %d\n", y - vp->v_yoff, xs - vp->v_xoff, xe - vp->v_xoff);
  cost = Rewrite(y - vp->v_yoff, xs - vp->v_xoff, xe - vp->v_xoff, &D_rend, 0);
  flayer = oldflayer;
  if (D_insert)
    cost += D_EIcost + D_IMcost;
  return cost;
}


void
GotoPos(x2, y2)
int x2, y2;
{
  register int dy, dx, x1, y1;
  register int costx, costy;
  register int m;
  register char *s;
  int CMcost;
  enum move_t xm = M_NONE, ym = M_NONE;

  if (!display)
    return;

  x1 = D_x;
  y1 = D_y;

  if (x1 == D_width)
    {
      if (D_CLP && D_AM)
        x1 = -1;	/* don't know how the terminal treats this */
      else
        x1--;
    }
  if (x2 == D_width)
    x2--;
  dx = x2 - x1;
  dy = y2 - y1;
  if (dy == 0 && dx == 0)
    return;
  debug2("GotoPos (%d,%d)", x1, y1);
  debug2(" -> (%d,%d)\n", x2, y2);
  if (!D_MS)		/* Safe to move ? */
    SetRendition(&mchar_null);
  if (y1 < 0			/* don't know the y position */
      || (y2 > D_bot && y1 <= D_bot)	/* have to cross border */
      || (y2 < D_top && y1 >= D_top))	/* of scrollregion ?    */
    {
    DoCM:
      if (D_HO && !x2 && !y2)
        PutStr(D_HO);
      else
        PutStr(tgoto(D_CM, x2, y2));
      D_x = x2;
      D_y = y2;
      return;
    }
  /* Calculate CMcost */
  if (D_HO && !x2 && !y2)
    s = D_HO;
  else
    s = tgoto(D_CM, x2, y2);
  CMcost = CalcCost(s);

  /* Calculate the cost to move the cursor to the right x position */
  costx = EXPENSIVE;
  if (x1 >= 0)	/* relativ x positioning only if we know where we are */
    {
      if (dx > 0)
	{
	  if (D_CRI && (dx > 1 || !D_ND))
	    {
	      costx = CalcCost(tgoto(D_CRI, 0, dx));
	      xm = M_CRI;
	    }
	  if ((m = D_NDcost * dx) < costx)
	    {
	      costx = m;
	      xm = M_RI;
	    }
	  /* Speedup: dx <= Rewrite() */
	  if (dx < costx && (m = CallRewrite(y1, x1, x2 - 1, 0)) < costx)
	    {
	      costx = m;
	      xm = M_RW;
	    }
	}
      else if (dx < 0)
	{
	  if (D_CLE && (dx < -1 || !D_BC))
	    {
	      costx = CalcCost(tgoto(D_CLE, 0, -dx));
	      xm = M_CLE;
	    }
	  if ((m = -dx * D_LEcost) < costx)
	    {
	      costx = m;
	      xm = M_LE;
	    }
	}
      else
	costx = 0;
    }
  /* Speedup: Rewrite() >= x2 */
  if (x2 + D_CRcost < costx && (m = (x2 ? CallRewrite(y1, 0, x2 - 1, 0) : 0) + D_CRcost) < costx)
    {
      costx = m;
      xm = M_CR;
    }

  /* Check if it is already cheaper to do CM */
  if (costx >= CMcost)
    goto DoCM;

  /* Calculate the cost to move the cursor to the right y position */
  costy = EXPENSIVE;
  if (dy > 0)
    {
      if (D_CDO && dy > 1)	/* DO & NL are always != 0 */
	{
	  costy = CalcCost(tgoto(D_CDO, 0, dy));
	  ym = M_CDO;
	}
      if ((m = dy * ((x2 == 0) ? D_NLcost : D_DOcost)) < costy)
	{
	  costy = m;
	  ym = M_DO;
	}
    }
  else if (dy < 0)
    {
      if (D_CUP && (dy < -1 || !D_UP))
	{
	  costy = CalcCost(tgoto(D_CUP, 0, -dy));
	  ym = M_CUP;
	}
      if ((m = -dy * D_UPcost) < costy)
	{
	  costy = m;
	  ym = M_UP;
	}
    }
  else
    costy = 0;

  /* Finally check if it is cheaper to do CM */
  if (costx + costy >= CMcost)
    goto DoCM;

  switch (xm)
    {
    case M_LE:
      while (dx++ < 0)
	PutStr(D_BC);
      break;
    case M_CLE:
      CPutStr(D_CLE, -dx);
      break;
    case M_RI:
      while (dx-- > 0)
	PutStr(D_ND);
      break;
    case M_CRI:
      CPutStr(D_CRI, dx);
      break;
    case M_CR:
      PutStr(D_CR);
      D_x = 0;
      x1 = 0;
      /* FALLTHROUGH */
    case M_RW:
      if (x1 < x2)
	(void) CallRewrite(y1, x1, x2 - 1, 1);
      break;
    default:
      break;
    }

  switch (ym)
    {
    case M_UP:
      while (dy++ < 0)
	PutStr(D_UP);
      break;
    case M_CUP:
      CPutStr(D_CUP, -dy);
      break;
    case M_DO:
      s =  (x2 == 0) ? D_NL : D_DO;
      while (dy-- > 0)
	PutStr(s);
      break;
    case M_CDO:
      CPutStr(D_CDO, dy);
      break;
    default:
      break;
    }
  D_x = x2;
  D_y = y2;
}

void
ClearDisplay()
{
  ASSERT(display);
  Clear(0, 0, 0, D_width - 1, D_width - 1, D_height - 1, 0);
}

void
Clear(x1, y1, xs, xe, x2, y2, uselayfn)
int x1, y1, xs, xe, x2, y2, uselayfn;
{
  int y, xxe;
  struct canvas *cv;
  struct viewport *vp;

  debug2("Clear %d,%d", x1, y1);
  debug2(" %d-%d", xs, xe);
  debug3(" %d,%d uselayfn=%d\n", x2, y2, uselayfn);
  ASSERT(display);
  if (x1 == D_width)
    x1--;
  if (x2 == D_width)
    x2--;
  if (D_UT)	/* Safe to erase ? */
    SetRendition(&mchar_null);
  if (D_lp_missing && y1 <= D_bot && xe >= D_width - 1)
    {
      if (y2 > D_bot || (y2 == D_bot && x2 >= D_width - 1))
	D_lp_missing = 0;
    }
  if (x2 == D_width - 1 && (xs == 0 || y1 == y2) && xe == D_width - 1 && y2 == D_height - 1)
    {
#ifdef AUTO_NUKE
      if (x1 == 0 && y1 == 0 && D_auto_nuke)
	NukePending();
#endif
      if (x1 == 0 && y1 == 0 && D_CL)
	{
	  PutStr(D_CL);
	  D_y = D_x = 0;
	  return;
	}
      /* 
       * Workaround a hp700/22 terminal bug. Do not use CD where CE
       * is also appropriate.
       */
      if (D_CD && (y1 < y2 || !D_CE))
	{
	  GotoPos(x1, y1);
	  PutStr(D_CD);
	  return;
	}
    }
  if (x1 == 0 && xs == 0 && (xe == D_width - 1 || y1 == y2) && y1 == 0 && D_CCD)
    {
      GotoPos(x1, y1);
      PutStr(D_CCD);
      return;
    }
  xxe = xe;
  for (y = y1; y <= y2; y++, x1 = xs)
    {
      if (y == y2)
	xxe = x2;
      if (x1 == 0 && D_CB && (xxe != D_width - 1 || (D_x == xxe && D_y == y)))
	{
	  GotoPos(xxe, y);
	  PutStr(D_CB);
	  continue;
	}
      if (xxe == D_width - 1 && D_CE)
	{
	  GotoPos(x1, y);
	  PutStr(D_CE);
	  continue;
	}
      if (uselayfn)
	{
	  vp = 0;
	  for (cv = D_cvlist; cv; cv = cv->c_next)
	    {
	      if (y < cv->c_ys || y > cv->c_ye || xxe < cv->c_xs || x1 > cv->c_xe)
		continue;
	      for (vp = cv->c_vplist; vp; vp = vp->v_next)
	        if (y >= vp->v_ys && y <= vp->v_ye && xxe >= vp->v_xs && x1 <= vp->v_xe)
		  break;
	      if (vp)
		break;
	    }
	  if (cv && cv->c_layer && x1 >= vp->v_xs && xxe <= vp->v_xe &&
              y - vp->v_yoff >= 0 && y - vp->v_yoff < cv->c_layer->l_height &&
              xxe - vp->v_xoff >= 0 && x1 - vp->v_xoff < cv->c_layer->l_width)
	    {
	      struct layer *oldflayer = flayer;
	      struct canvas *cvlist, *cvlnext;
	      flayer = cv->c_layer;
	      cvlist = flayer->l_cvlist;
	      cvlnext = cv->c_lnext;
	      flayer->l_cvlist = cv;
	      cv->c_lnext = 0;
              ClearLine(y - vp->v_yoff, x1 - vp->v_xoff, xxe - vp->v_xoff);
	      flayer->l_cvlist = cvlist;
	      cv->c_lnext = cvlnext;
	      flayer = oldflayer;
	      continue;
	    }
	}
      DisplayLine(&mline_null, &mline_blank, y, x1, xxe);
    }
}


/*
 * if cur_only > 0, we only redisplay current line, as a full refresh is
 * too expensive over a low baud line.
 */
void
Redisplay(cur_only)
int cur_only;
{
  register int i, stop;
  struct canvas *cv;

  ASSERT(display);

  /* XXX do em all? */
  InsertMode(0);
  ChangeScrollRegion(0, D_height - 1);
  KeypadMode(0);
  CursorkeysMode(0);
  CursorVisibility(0);
  SetRendition(&mchar_null);
  SetFlow(FLOW_NOW);

  ClearDisplay();
  stop = D_height;
  i = 0;
  if (cur_only > 0 && D_fore)
    {
      i = stop = D_fore->w_y;
      stop++;
    }
  else
    {
      debug("Signalling full refresh!\n");
      for (cv = D_cvlist; cv; cv = cv->c_next)
	{
	  CV_CALL(cv, RedisplayLine(-1, -1, -1, 1));
	  display = cv->c_display;	/* just in case! */
	}
    }
  RefreshArea(0, i, D_width - 1, stop - 1, 1);
  RefreshHStatus();

  CV_CALL(D_forecv, Restore();SetCursor());
}

void
RedisplayDisplays(cur_only)
int cur_only;
{
  struct display *olddisplay = display;
  for (display = displays; display; display = display->d_next)
    Redisplay(cur_only);
  display = olddisplay;
}


/* XXX: use oml! */
void
ScrollH(y, xs, xe, n, oml)
int y, xs, xe, n;
struct mline *oml;
{
  int i;

  if (n == 0)
    return;
  if (xe != D_width - 1)
    {
      RefreshLine(y, xs, xe, 0);
      /* UpdateLine(oml, y, xs, xe); */
      return;
    }
  GotoPos(xs, y);
  if (D_UT)
    SetRendition(&mchar_null);
  if (n > 0)
    {
      if (D_CDC && !(n == 1 && D_DC))
	CPutStr(D_CDC, n);
      else if (D_DC)
	{
	  for (i = n; i--; )
	    PutStr(D_DC);
	}
      else
	{
	  RefreshLine(y, xs, xe, 0);
	  /* UpdateLine(oml, y, xs, xe); */
	  return;
	}
    }
  else
    {
      if (!D_insert)
	{
	  if (D_CIC && !(n == -1 && D_IC))
	    CPutStr(D_CIC, -n);
	  else if (D_IC)
	    {
	      for (i = -n; i--; )
		PutStr(D_IC);
	    }
	  else if (D_IM)
	    {
	      InsertMode(1);
	      for (i = -n; i--; )
		INSERTCHAR(' ');
	    }
	  else
	    {
	      /* UpdateLine(oml, y, xs, xe); */
	      RefreshLine(y, xs, xe, 0);
	      return;
	    }
	}
      else
	{
	  for (i = -n; i--; )
	    INSERTCHAR(' ');
	}
    }
  if (D_lp_missing && y == D_bot)
    {
      if (n > 0)
        WriteLP(D_width - 1 - n, y);
      D_lp_missing = 0;
    }
}

void
ScrollV(xs, ys, xe, ye, n)
int xs, ys, xe, ye, n;
{
  int i;
  int up;
  int oldtop, oldbot;
  int alok, dlok, aldlfaster;
  int missy = 0;

  ASSERT(display);
  if (n == 0)
    return;
  if (n >= ye - ys + 1 || -n >= ye - ys + 1)
    {
      Clear(xs, ys, xs, xe, xe, ye, 0);
      return;
    }
  if (xs > D_vpxmin || xe < D_vpxmax)
    {
      RefreshArea(xs, ys, xe, ye, 0);
      return;
    }

  if (D_lp_missing)
    {
      if (D_bot > ye || D_bot < ys)
	missy = D_bot;
      else
	{
	  missy = D_bot - n;
          if (missy > ye || missy < ys)
	    D_lp_missing = 0;
	}
    }

  up = 1;
  if (n < 0)
    {
      up = 0;
      n = -n;
    }
  if (n >= ye - ys + 1)
    n = ye - ys + 1;

  oldtop = D_top;
  oldbot = D_bot;
  if (ys < D_top || D_bot != ye)
    ChangeScrollRegion(ys, ye);
  alok = (D_AL || D_CAL || (ys >= D_top && ye == D_bot &&  up));
  dlok = (D_DL || D_CDL || (ys >= D_top && ye == D_bot && !up));
  if (D_top != ys && !(alok && dlok))
    ChangeScrollRegion(ys, ye);

  if (D_lp_missing && 
      (oldbot != D_bot ||
       (oldbot == D_bot && up && D_top == ys && D_bot == ye)))
    {
      WriteLP(D_width - 1, oldbot);
      if (oldbot == D_bot)		/* have scrolled */
	{
	  if (--n == 0)
	    {
/* XXX
	      ChangeScrollRegion(oldtop, oldbot);
*/
	      return;
	    }
	}
    }

  aldlfaster = (n > 1 && ys >= D_top && ye == D_bot && ((up && D_CDL) || (!up && D_CAL)));

  if (D_UT)
    SetRendition(&mchar_null);
  if ((up || D_SR) && D_top == ys && D_bot == ye && !aldlfaster)
    {
      if (up)
	{
	  GotoPos(0, ye);
	  while (n-- > 0)
	    PutStr(D_NL);		/* was SF, I think NL is faster */
	}
      else
	{
	  GotoPos(0, ys);
	  while (n-- > 0)
	    PutStr(D_SR);
	}
    }
  else if (alok && dlok)
    {
      if (up || ye != D_bot)
	{
          GotoPos(0, up ? ys : ye+1-n);
          if (D_CDL && !(n == 1 && D_DL))
	    CPutStr(D_CDL, n);
	  else
	    for(i = n; i--; )
	      PutStr(D_DL);
	}
      if (!up || ye != D_bot)
	{
          GotoPos(0, up ? ye+1-n : ys);
          if (D_CAL && !(n == 1 && D_AL))
	    CPutStr(D_CAL, n);
	  else
	    for(i = n; i--; )
	      PutStr(D_AL);
	}
    }
  else
    {
      RefreshArea(xs, ys, xe, ye, 0);
      return;
    }
  if (D_lp_missing && missy != D_bot)
    WriteLP(D_width - 1, missy);
/* XXX
  ChangeScrollRegion(oldtop, oldbot);
  if (D_lp_missing && missy != D_bot)
    WriteLP(D_width - 1, missy);
*/
}

void
SetAttr(new)
register int new;
{
  register int i, j, old, typ;

  if (!display || (old = D_rend.attr) == new)
    return;
#if defined(TERMINFO) && defined(USE_SGR)
  if (D_SA)
    {
      char *tparm();
      SetFont(ASCII);
      ospeed = D_dospeed;
      tputs(tparm(D_SA, new & A_SO, new & A_US, new & A_RV, new & A_BL,
			new & A_DI, new & A_BD, 0         , 0         ,
			0), 1, PutChar);
      D_rend.attr = new;
      D_atyp = 0;
# ifdef COLOR
      if (D_CAF || D_CAB)
	D_rend.color = 0;
# endif
      return;
    }
#endif
  typ = D_atyp;
  if ((new & old) != old)
    {
      if ((typ & ATYP_U))
        PutStr(D_UE);
      if ((typ & ATYP_S))
        PutStr(D_SE);
      if ((typ & ATYP_M))
	{
          PutStr(D_ME);
#ifdef COLOR
	  /* ansi attrib handling: \E[m resets color, too */
	  if (D_CAF || D_CAB)
	    D_rend.color = 0;
#endif
	}
      old = 0;
      typ = 0;
    }
  old ^= new;
  for (i = 0, j = 1; old && i < NATTR; i++, j <<= 1)
    {
      if ((old & j) == 0)
	continue;
      old ^= j;
      if (D_attrtab[i])
	{
	  PutStr(D_attrtab[i]);
	  typ |= D_attrtyp[i];
	}
    }
  D_rend.attr = new;
  D_atyp = typ;
}

#ifdef FONT
void
SetFont(new)
int new;
{
  if (!display || D_rend.font == new)
    return;
  D_rend.font = new;
#ifdef KANJI
  if ((new == KANJI || new == KANA) && D_kanji)
    return;	/* all done in RAW_PUTCHAR */
#endif
  if (D_xtable && D_xtable[(int)(unsigned char)new] &&
      D_xtable[(int)(unsigned char)new][256])
    {
      PutStr(D_xtable[(int)(unsigned char)new][256]);
      return;
    }

  if (!D_CG0 && new != '0')
    new = ASCII;

  if (new == ASCII)
    PutStr(D_CE0);
#ifdef KANJI
  else if (new < ' ')
    {
      AddStr("\033$");
      AddChar(new + '@');
    }
#endif
  else
    CPutStr(D_CS0, new);
}
#endif

#ifdef COLOR
void
SetColor(new)
int new;
{
  int of, ob, f, b;

  if (!display || D_rend.color == new)
    return;
  of = D_rend.color & 0xf;
  ob = (D_rend.color >> 4) & 0xf;
  f = new & 0xf;
  b = (new >> 4) & 0xf;

  if (!D_CAX && (D_CAF || D_CAB) && ((f == 0 && f != of) || (b == 0 && b != ob)))
    {
      int oattr;

      oattr = D_rend.attr;
      AddStr("\033[m");
      D_rend.attr = 0;
      D_rend.color = 0;
      of = ob = 0;
      SetAttr(oattr);
    }
  if (D_CAF || D_CAB)
    {
      if (f != of)
	CPutStr(D_CAF, 9 - f);
      if (b != ob)
	CPutStr(D_CAB, 9 - b);
    }
  D_rend.color = new;
}
#endif

void
SetRendition(mc)
struct mchar *mc;
{
  if (!display)
    return;
  if (D_rend.attr != mc->attr)
    SetAttr(mc->attr);
#ifdef COLOR
  if (D_rend.color != mc->color)
    SetColor(mc->color);
#endif
#ifdef FONT
  if (D_rend.font != mc->font)
    SetFont(mc->font);
#endif
}

void
SetRenditionMline(ml, x)
struct mline *ml;
int x;
{
  if (!display)
    return;
  if (D_rend.attr != ml->attr[x])
    SetAttr(ml->attr[x]);
#ifdef COLOR
  if (D_rend.color != ml->color[x])
    SetColor(ml->color[x]);
#endif
#ifdef FONT
  if (D_rend.font != ml->font[x])
    SetFont(ml->font[x]);
#endif
}

void
MakeStatus(msg)
char *msg;
{
  register char *s, *t;
  register int max, ti;

  if (!display)
    return;
  
  if (!D_tcinited)
    {
      debug("tc not inited, just writing msg\n");
      if (D_processinputdata)
	return;		/* XXX: better */
      AddStr(msg);
      AddStr("\r\n");
      Flush();
      return;
    }
  if (!use_hardstatus || !D_HS)
    {
      max = D_width;
      if (D_CLP == 0)
	max--;
    }
  else
    max = D_WS;
  if (D_status)
    {
      /* same message? */
      if (strcmp(msg, D_status_lastmsg) == 0)
	{
	  debug("same message - increase timeout");
	  SetTimeout(&D_statusev, MsgWait * 1000);
	  return;
	}
      if (!D_status_bell)
	{
	  ti = time((time_t *)0) - D_status_time;
	  if (ti < MsgMinWait)
	    sleep(MsgMinWait - ti);
	}
      RemoveStatus();
    }
  for (s = t = msg; *s && t - msg < max; ++s)
    if (*s == BELL)
      PutStr(D_BL);
    else if ((unsigned char)*s >= ' ' && *s != 0177)
      *t++ = *s;
  *t = '\0';
  if (t == msg)
    return;
  if (t - msg >= D_status_buflen)
    {
      char *buf;
      if (D_status_lastmsg)
	buf = realloc(D_status_lastmsg, t - msg + 1);
      else
	buf = malloc(t - msg + 1);
      if (buf)
	{
	  D_status_lastmsg = buf;
	  D_status_buflen = t - msg + 1;
	}
    }
  if (t - msg < D_status_buflen)
    strcpy(D_status_lastmsg, msg);
  D_status_len = t - msg;
  D_status_lastx = D_x;
  D_status_lasty = D_y;
  if (!use_hardstatus || D_has_hstatus == HSTATUS_IGNORE || D_has_hstatus == HSTATUS_MESSAGE)
    {
      if (D_status_delayed != -1 && t - msg < D_status_buflen)
	{
	  D_status_delayed = 1;	/* not yet... */
	  D_status = 0;
	  return;
	}
      D_status = STATUS_ON_WIN;
      debug1("using STATLINE %d\n", STATLINE);
      GotoPos(0, STATLINE);
      SetRendition(&mchar_so);
      InsertMode(0);
      AddStr(msg);
      if (D_status_len < max)
	{
	  /* Wayne Davison: add extra space for readability */
	  D_status_len++;
	  SetRendition(&mchar_null);
	  AddChar(' ');
	  if (D_status_len < max)
	    {
	      D_status_len++;
	      AddChar(' ');
	      AddChar('\b');
	    }
	  AddChar('\b');
	}
      D_x = -1;
    }
  else
    {
      D_status = STATUS_ON_HS;
      ShowHStatus(msg);
    }
  D_status_delayed = 0;
  Flush();
  (void) time(&D_status_time);
  SetTimeout(&D_statusev, MsgWait * 1000);
  evenq(&D_statusev);
#ifdef HAVE_BRAILLE
  RefreshBraille();	/* let user see multiple Msg()s */
#endif
}

void
RemoveStatus()
{
  struct display *olddisplay;
  struct layer *oldflayer;
  int where;

  if (!display)
    return;
  if (!(where = D_status))
    return;
  
  D_status = 0;
  D_status_bell = 0;
  evdeq(&D_statusev);
  olddisplay = display;
  oldflayer = flayer;
  if (where == STATUS_ON_WIN)
    {
      GotoPos(0, STATLINE);
      RefreshLine(STATLINE, 0, D_status_len - 1, 0);
      GotoPos(D_status_lastx, D_status_lasty);
    }
  else
    RefreshHStatus();
  flayer = D_forecv->c_layer;
  if (flayer)
    SetCursor();
  display = olddisplay;
  flayer = oldflayer;
}

/* refresh the display's hstatus line */
void
ShowHStatus(str)
char *str;
{
  int l, i, ox, oy;

  if (D_status == STATUS_ON_WIN && D_has_hstatus == HSTATUS_LASTLINE && STATLINE == D_height-1)
    return;	/* sorry, in use */

  if (D_HS && D_has_hstatus == HSTATUS_HS)
    {
      if (!D_hstatus && (str == 0 || *str == 0))
	return;
      debug("ShowHStatus: using HS\n");
      SetRendition(&mchar_null);
      InsertMode(0);
      if (D_hstatus)
	PutStr(D_DS);
      D_hstatus = 0;
      if (str == 0 || *str == 0)
	return;
      CPutStr(D_TS, 0);
      if (strlen(str) > D_WS)
	AddStrn(str, D_WS);
      else
	AddStr(str);
      PutStr(D_FS);
      D_hstatus = 1;
    }
  else if (D_has_hstatus == HSTATUS_LASTLINE)
    {
      debug("ShowHStatus: using last line\n");
      ox = D_x;
      oy = D_y;
      str = str ? str : "";
      l = strlen(str);
      if (l > D_width)
	l = D_width;
      GotoPos(0, D_height - 1);
      SetRendition(captionalways || D_cvlist == 0 || D_cvlist->c_next ? &mchar_null: &mchar_so);
      for (i = 0; i < l; i++)
	PUTCHARLP(str[i]);
      if (!captionalways && D_cvlist && !D_cvlist->c_next)
        while (l++ < D_width)
	  PUTCHARLP(' ');
      if (l < D_width)
        Clear(l, D_height - 1, l, D_width - 1, D_width - 1, D_height - 1, 0);
      if (ox != -1 && oy != -1)
	GotoPos(ox, oy);
      D_hstatus = *str ? 1 : 0;
      SetRendition(&mchar_null);
    }
  else if (str && *str && D_has_hstatus == HSTATUS_MESSAGE)
    {
      debug("ShowHStatus: using message\n");
      Msg(0, "%s", str);
    }
}


/*
 *  Refreshes the harstatus of the fore window. Shouldn't be here...
 */
void
RefreshHStatus()
{
  char *buf;

  evdeq(&D_hstatusev);
  if (D_status == STATUS_ON_HS)
    return;
  buf = MakeWinMsgEv(hstatusstring, D_fore, '%', &D_hstatusev);
  if (buf && *buf)
    {
      ShowHStatus(buf);
      if (D_has_hstatus != HSTATUS_IGNORE && D_hstatusev.timeout.tv_sec)
        evenq(&D_hstatusev);
    }
  else
    ShowHStatus((char *)0);
}

/*********************************************************************/
/*
 *  Here come the routines that refresh an arbitrary part of the screen.
 */

void
RefreshArea(xs, ys, xe, ye, isblank)
int xs, ys, xe, ye, isblank;
{
  int y;
  ASSERT(display);
  debug2("Refresh Area: %d,%d", xs, ys);
  debug3(" - %d,%d (isblank=%d)\n", xe, ye, isblank);
  if (!isblank && xs == 0 && xe == D_width - 1 && ye == D_height - 1 && (ys == 0 || D_CD))
    {
      Clear(xs, ys, xs, xe, xe, ye, 0);
      isblank = 1;
    }
  for (y = ys; y <= ye; y++)
    RefreshLine(y, xs, xe, isblank);
}

void
RefreshLine(y, from, to, isblank)
int y, from, to, isblank;
{
  struct viewport *vp, *lvp;
  struct canvas *cv, *lcv, *cvlist, *cvlnext;
  struct layer *oldflayer;
  int xx, yy;
  char *buf;
  struct win *p;

  ASSERT(display);

  debug2("RefreshLine %d %d", y, from);
  debug2(" %d %d\n", to, isblank);

  if (D_status == STATUS_ON_WIN && y == STATLINE)
    return;	/* can't refresh status */

  if (isblank == 0 && D_CE && to == D_width - 1 && from < to)
    {
      GotoPos(from, y);
      if (D_UT)
	SetRendition(&mchar_null);
      PutStr(D_CE);
      isblank = 1;
    }
  while (from <= to)
    {
      lcv = 0;
      lvp = 0;
      for (cv = display->d_cvlist; cv; cv = cv->c_next)
	{
	  if (y < cv->c_ys || y > cv->c_ye || to < cv->c_xs || from > cv->c_xe)
	    continue;
	  debug2("- canvas hit: %d %d", cv->c_xs, cv->c_ys);
	  debug2("  %d %d\n", cv->c_xe, cv->c_ye);
	  for (vp = cv->c_vplist; vp; vp = vp->v_next)
	    {
	      debug2("  - vp: %d %d", vp->v_xs, vp->v_ys);
	      debug2("  %d %d\n", vp->v_xe, vp->v_ye);
	      /* find leftmost overlapping vp */
	      if (y >= vp->v_ys && y <= vp->v_ye && from <= vp->v_xe && to >= vp->v_xs && (lvp == 0 || lvp->v_xs > vp->v_xs))
		{
		  lcv = cv;
		  lvp = vp;
		}
	    }
	}
      if (lvp == 0)
	break;
      if (from < lvp->v_xs)
	{
	  if (!isblank)
	    DisplayLine(&mline_null, &mline_blank, y, from, lvp->v_xs - 1);
	  from = lvp->v_xs;
	}

      /* call RedisplayLine on canvas lcv viewport lvp */
      yy = y - lvp->v_yoff;
      xx = to < lvp->v_xe ? to : lvp->v_xe;

      if (lcv->c_layer && yy == lcv->c_layer->l_height)
	{
	  GotoPos(from, y);
	  SetRendition(&mchar_blank);
	  while (from <= lvp->v_xe && from - lvp->v_xoff < lcv->c_layer->l_width)
	    {
	      PUTCHARLP('-');
	      from++;
	    }
	  if (from >= lvp->v_xe + 1)
	    continue;
	}
      if (lcv->c_layer == 0 || yy >= lcv->c_layer->l_height || from - lvp->v_xoff >= lcv->c_layer->l_width)
	{
	  if (!isblank)
	    DisplayLine(&mline_null, &mline_blank, y, from, lvp->v_xe);
	  from = lvp->v_xe + 1;
	  continue;
	}

      if (xx - lvp->v_xoff >= lcv->c_layer->l_width)
	xx = lcv->c_layer->l_width + lvp->v_xoff - 1;
      oldflayer = flayer;
      flayer = lcv->c_layer;
      cvlist = flayer->l_cvlist;
      cvlnext = lcv->c_lnext;
      flayer->l_cvlist = lcv;
      lcv->c_lnext = 0;
      RedisplayLine(yy, from - lvp->v_xoff, xx - lvp->v_xoff, isblank);
      flayer->l_cvlist = cvlist;
      lcv->c_lnext = cvlnext;
      flayer = oldflayer;

      from = xx + 1;
    }
  if (from > to)
    return;		/* all done */

  if (y == D_height - 1 && D_has_hstatus == HSTATUS_LASTLINE)
    {
      RefreshHStatus();
      return;
    }

  for (cv = display->d_cvlist; cv; cv = cv->c_next)
    if (y == cv->c_ye + 1)
      break;
  if (cv == 0)
    {
      if (!isblank)
	DisplayLine(&mline_null, &mline_blank, y, from, to);
      return;
    }

  p = Layer2Window(cv->c_layer);
  buf = MakeWinMsgEv(captionstring, p, '%', &cv->c_captev);
  if (cv->c_captev.timeout.tv_sec)
    evenq(&cv->c_captev);
  xx = strlen(buf);
  GotoPos(from, y);
  SetRendition(&mchar_so);
  while (from <= to && from < xx)
    {
      PUTCHARLP(buf[from]);
      from++;
    }
  while (from++ <= to)
    PUTCHARLP(' ');
}

/*********************************************************************/

/* clear lp_missing by writing the char on the screen. The
 * position must be safe.
 */
static void
WriteLP(x2, y2)
int x2, y2;
{
  struct mchar oldrend;

  ASSERT(display);
  ASSERT(D_lp_missing);
  oldrend = D_rend;
#ifdef KANJI
  if (D_lpchar.font == KANJI && (D_mbcs = D_lp_mbcs) != 0 && x2 > 0)
    x2--;
#endif
  GotoPos(x2, y2);
  SetRendition(&D_lpchar);
  PUTCHAR(D_lpchar.image);
  D_lp_missing = 0;
  SetRendition(&oldrend);
}

void
DisplayLine(oml, ml, y, from, to)
struct mline *oml, *ml;
int from, to, y;
{
  register int x;
  int last2flag = 0, delete_lp = 0;

  ASSERT(display);
  ASSERT(y >= 0 && y < D_height);
  ASSERT(from >= 0 && from < D_width);
  ASSERT(to >= 0 && to < D_width);
  if (!D_CLP && y == D_bot && to == D_width - 1)
    {
      if (D_lp_missing || !cmp_mline(oml, ml, to))
	{
	  if ((D_IC || D_IM) && from < to)
	    {
	      to -= 2;
	      last2flag = 1;
	      D_lp_missing = 0;
	    }
	  else
	    {
	      to--;
	      delete_lp = (D_CE || D_DC || D_CDC);
	      D_lp_missing = !cmp_mchar_mline(&mchar_blank, ml, to);
	      copy_mline2mchar(&D_lpchar, ml, to);
	    }
	}
      else
	to--;
    }
#ifdef KANJI
  if (D_mbcs)
    {
      /* finish kanji (can happen after a wrap) */
      SetRenditionMline(ml, from);
      PUTCHAR(ml->image[from]);
      from++;
    }
#endif
  for (x = from; x <= to; x++)
    {
#if 0	/* no longer needed */
      if (x || D_x != D_width || D_y != y - 1)
#endif
        {
	  if (x < to || x != D_width - 1 || ml->image[x + 1])
	    if (cmp_mline(oml, ml, x))
	      continue;
	  GotoPos(x, y);
        }
#ifdef KANJI
      if (badkanji(ml->font, x))
	{
	  x--;
	  debug1("DisplayLine badkanji - x now %d\n", x);
	  GotoPos(x, y);
	}
      if (ml->font[x] == KANJI && x == to)
	break;	/* don't start new kanji */
#endif
      SetRenditionMline(ml, x);
      PUTCHAR(ml->image[x]);
#ifdef KANJI
      if (ml->font[x] == KANJI)
	PUTCHAR(ml->image[++x]);
#endif
    }
#if 0	/* not needed any longer */
  /* compare != 0 because ' ' can happen when clipping occures */
  if (to == D_width - 1 && y < D_height - 1 && D_x == D_width && ml->image[to + 1])
    GotoPos(0, y + 1);
#endif
  if (last2flag)
    {
      GotoPos(x, y);
      SetRenditionMline(ml, x + 1);
      PUTCHAR(ml->image[x + 1]);
      GotoPos(x, y);
      SetRenditionMline(ml, x);
      INSERTCHAR(ml->image[x]);
    }
  else if (delete_lp)
    {
      if (D_UT)
	SetRendition(&mchar_null);
      if (D_DC)
	PutStr(D_DC);
      else if (D_CDC)
	CPutStr(D_CDC, 1);
      else if (D_CE)
	PutStr(D_CE);
    }
}

void
InsChar(c, x, xe, y, oml)
struct mchar *c;
int x, xe, y;
struct mline *oml;
{
  GotoPos(x, y);
  if (y == D_bot && !D_CLP)
    {
      if (x == D_width - 1)
        {
          D_lpchar = *c;
          return;
        }
      if (xe == D_width - 1)
        D_lp_missing = 0;
    }
  if (x == xe)
    {
      if (xe != D_width - 1)
        InsertMode(0);
      SetRendition(c);
      RAW_PUTCHAR(c->image);
      return;
    }
  if (!(D_IC || D_CIC || D_IM) || xe != D_width - 1)
    {
      RefreshLine(y, x, xe, 0);
      GotoPos(x + 1, y);
      /* UpdateLine(oml, y, x, xe); */
      return;
    }
  InsertMode(1);
  if (!D_insert)
    {
      if (D_IC)
        PutStr(D_IC);
      else
        CPutStr(D_CIC, 1);
    }
  SetRendition(c);
  RAW_PUTCHAR(c->image);
}

void
WrapChar(c, x, y, xs, ys, xe, ye, ins)
struct mchar *c;
int x, y;
int xs, ys, xe, ye;
int ins;
{
  debug("WrapChar:");
  debug2("  x %d  y %d", x, y);
  debug2("  Dx %d  Dy %d", D_x, D_y);
  debug2("  xs %d  ys %d", xs, ys);
  debug3("  xe %d  ye %d ins %d\n", xe, ye, ins);
  if (xs != 0 || x != D_width || !D_AM)
    {
      if (y == ye)
	ScrollV(xs, ys, xe, ye, 1);
      else if (y < D_height - 1)
	y++;
      GotoPos(xs, y);
      if (ins)
	{
	  InsChar(c, xs, xe, y, 0);
	  return;
	}
      SetRendition(c);
      RAW_PUTCHAR(c->image);
      return;
    }
  if (y == ye)		/* we have to scroll */
    {
      debug("- scrolling\n");
      ChangeScrollRegion(ys, ye);
      if (D_bot != y)
	{
	  debug("- have to call ScrollV\n");
	  ScrollV(xs, ys, xe, ye, 1);
	  y--;
	}
    }
  else if (y == D_bot)
    ChangeScrollRegion(ys, ye);		/* remove unusable region */
  if (D_x != D_width || D_y != y)
    {
      if (D_CLP && y >= 0)		/* don't even try if !LP */
        RefreshLine(y, D_width - 1, D_width - 1, 0);
      debug2("- refresh last char -> x,y now %d,%d\n", D_x, D_y);
      if (D_x != D_width || D_y != y)	/* sorry, no bonus */
	{
	  if (y == ye)
	    ScrollV(xs, ys, xe, ye, 1);
	  GotoPos(xs, y == ye || y == D_height - 1 ? y : y + 1);
	}
    }
  debug("- writeing new char");
  if (y != ye && y < D_height - 1)
    y++;
  if (ins != D_insert)
    InsertMode(ins);
  if (ins && !D_insert)
    {
      InsChar(c, 0, xe, y, 0);
      debug2(" -> done with insert (%d,%d)\n", D_x, D_y);
      return;
    }
  SetRendition(c);
  D_y = y;
  D_x = 0;
  RAW_PUTCHAR(c->image);
  debug2(" -> done (%d,%d)\n", D_x, D_y);
}

int
ResizeDisplay(wi, he)
int wi, he;
{
  ASSERT(display);
  debug2("ResizeDisplay: to (%d,%d).\n", wi, he);
  if (D_width == wi && D_height == he)
    {
      debug("ResizeDisplay: No change\n");
      return 0;
    }
  if (D_width != wi && (D_height == he || !D_CWS) && D_CZ0 && (wi == Z0width || wi == Z1width))
    {
      debug("ResizeDisplay: using Z0/Z1\n");
      PutStr(wi == Z0width ? D_CZ0 : D_CZ1);
      ChangeScreenSize(wi, D_height, 0);
      return (he == D_height) ? 0 : -1;
    }
  if (D_CWS)
    {
      debug("ResizeDisplay: using WS\n");
      PutStr(tgoto(D_CWS, wi, he));
      ChangeScreenSize(wi, he, 0);
      return 0;
    }
  return -1;
}

void
ChangeScrollRegion(newtop, newbot)
int newtop, newbot;
{
  if (display == 0)
    return;
  if (newtop == -1)
    newtop = 0;
  if (newbot == -1)
    newbot = D_height - 1;
  if (D_CS == 0)
    {
      D_top = 0;
      D_bot = D_height - 1;
      return;
    }
  if (D_top == newtop && D_bot == newbot)
    return;
  debug2("ChangeScrollRegion: (%d - %d)\n", newtop, newbot);
  PutStr(tgoto(D_CS, newbot, newtop));
  D_top = newtop;
  D_bot = newbot;
  D_y = D_x = -1;		/* Just in case... */
}


/*
 *  Output buffering routines
 */

void
AddStr(str)
char *str;
{
  register char c;

  ASSERT(display);

  while ((c = *str++))
    AddChar(c);
}

void
AddStrn(str, n)
char *str;
int n;
{
  register char c;

  ASSERT(display);
  while ((c = *str++) && n-- > 0)
    AddChar(c);
  while (n-- > 0)
    AddChar(' ');
}

void
Flush()
{
  register int l;
  register char *p;

  ASSERT(display);
  l = D_obufp - D_obuf;
  debug1("Flush(): %d\n", l);
  ASSERT(l + D_obuffree == D_obuflen);
  if (l == 0)
    return;
  if (D_userfd < 0)
    {
      D_obuffree += l;
      D_obufp = D_obuf;
      return;
    }
  p = D_obuf;
  if (fcntl(D_userfd, F_SETFL, 0))
    debug1("Warning: BLOCK fcntl failed: %d\n", errno);
  while (l)
    {
      register int wr;
      wr = write(D_userfd, p, l);
      if (wr <= 0) 
	{
	  if (errno == EINTR) 
	    continue;
	  debug1("Writing to display: %d\n", errno);
	  wr = l;
	}
      D_obuffree += wr;
      p += wr;
      l -= wr;
    }
  D_obuffree += l;
  D_obufp = D_obuf;
  if (D_nonblock > 1)
    D_nonblock = 1;	/* reenable flow control for WriteString */
  if (fcntl(D_userfd, F_SETFL, FNBLOCK))
    debug1("Warning: NBLOCK fcntl failed: %d\n", errno);
}

void
freetty()
{
  if (D_userfd >= 0)
    close(D_userfd);
  debug1("did freetty %d\n", D_userfd);
  D_userfd = -1;
  D_obufp = 0;
  D_obuffree = 0;
  if (D_obuf)
    free(D_obuf);
  D_obuf = 0;
  D_obuflen = 0;
  D_obuflenmax = -D_obufmax;
}

/*
 *  Asynchronous output routines by
 *  Tim MacKenzie (tym@dibbler.cs.monash.edu.au)
 */

void
Resize_obuf()
{
  register int ind;

  ASSERT(display);
  if (D_obuflen && D_obuf)
    {
      ind  = D_obufp - D_obuf;
      D_obuflen += GRAIN;
      D_obuffree += GRAIN;
      D_obuf = realloc(D_obuf, D_obuflen);
    }
  else
    {
      ind  = 0;
      D_obuflen = GRAIN;
      D_obuffree = GRAIN;
      D_obuf = malloc(D_obuflen);
    }
  if (!D_obuf)
    Panic(0, "Out of memory");
  D_obufp = D_obuf + ind;
  D_obuflenmax = D_obuflen - D_obufmax;
  debug1("ResizeObuf: resized to %d\n", D_obuflen);
}

#ifdef AUTO_NUKE
void
NukePending()
{/* Nuke pending output in current display, clear screen */
  register int len;
  int oldtop = D_top, oldbot = D_bot;
  struct mchar oldrend;
  int oldkeypad = D_keypad, oldcursorkeys = D_cursorkeys;
  int oldcurvis = D_curvis;

  oldrend = D_rend;
  len = D_obufp - D_obuf;
  debug1("NukePending: nuking %d chars\n", len);
  
  /* Throw away any output that we can... */
# ifdef POSIX
  tcflush(D_userfd, TCOFLUSH);
# else
#  ifdef TCFLSH
  (void) ioctl(D_userfd, TCFLSH, (char *) 1);
#  endif
# endif

  D_obufp = D_obuf;
  D_obuffree += len;
  D_top = D_bot = -1;
  PutStr(D_TI);
  PutStr(D_IS);
  /* Turn off all attributes. (Tim MacKenzie) */
  if (D_ME)
    PutStr(D_ME);
  else
    {
#ifdef COLOR
      if (D_CAF)
	AddStr("\033[m");	/* why is D_ME not set? */
#endif
      PutStr(D_SE);
      PutStr(D_UE);
    }
  /* Check for toggle */
  if (D_IM && strcmp(D_IM, D_EI))
    PutStr(D_EI);
  D_insert = 0;
  /* Check for toggle */
#ifdef MAPKEYS
  if (D_KS && strcmp(D_KS, D_KE))
    PutStr(D_KS);
  if (D_CCS && strcmp(D_CCS, D_CCE))
    PutStr(D_CCS);
#else
  if (D_KS && strcmp(D_KS, D_KE))
    PutStr(D_KE);
  D_keypad = 0;
  if (D_CCS && strcmp(D_CCS, D_CCE))
    PutStr(D_CCE);
  D_cursorkeys = 0;
#endif
  PutStr(D_CE0);
  D_rend = mchar_null;
  D_atyp = 0;
  PutStr(D_DS);
  D_hstatus = 0;
  PutStr(D_VE);
  D_curvis = 0;
  ChangeScrollRegion(oldtop, oldbot);
  SetRendition(&oldrend);
  KeypadMode(oldkeypad);
  CursorkeysMode(oldcursorkeys);
  CursorVisibility(oldcurvis);
  if (D_CWS)
    {
      debug("ResizeDisplay: using WS\n");
      PutStr(tgoto(D_CWS, D_width, D_height));
    }
  else if (D_CZ0 && (D_width == Z0width || D_width == Z1width))
    {
      debug("ResizeDisplay: using Z0/Z1\n");
      PutStr(D_width == Z0width ? D_CZ0 : D_CZ1);
    }
}
#endif /* AUTO_NUKE */

#ifdef KANJI
int
badkanji(f, x)
char *f;
int x;
{
  int i, j;

  f += x;
  if (*f-- != KANJI)
    return 0;
  for (j = 0, i = x - 1; i >= 0; i--, j ^= 1)
    if (*f-- != KANJI)
      break;
  return j;
}
#endif

static void
disp_writeev_fn(ev, data)
struct event *ev;
char *data;
{
  int len, size = OUTPUT_BLOCK_SIZE;

  display = (struct display *)data;
  len = D_obufp - D_obuf;
  if (len < size)
    size = len;
  ASSERT(len >= 0);
  size = write(D_userfd, D_obuf, size);
  if (size >= 0) 
    {
      len -= size;
      if (len)
	{
	  bcopy(D_obuf + size, D_obuf, len);
	  debug2("ASYNC: wrote %d - remaining %d\n", size, len);
	}
      /* Great, reenable flow control for WriteString now. */
      if ((D_nonblock > 1) && (len < D_obufmax/2))
	D_nonblock = 1;
      D_obufp -= size;
      D_obuffree += size;
    } 
  else
    {
      if (errno != EINTR)
# ifdef EWOULDBLOCK
	if (errno != EWOULDBLOCK)
# endif
	  Msg(errno, "Error writing output to display");
    }
}

static void
disp_readev_fn(ev, data)
struct event *ev;
char *data;
{
  int size;
  char buf[IOSIZE];
  struct canvas *cv;

  display = (struct display *)data;

  /* Hmmmm... a bit ugly... */
  if (D_forecv)
    for (cv = D_forecv->c_layer->l_cvlist; cv; cv = cv->c_lnext)
      {
        display = cv->c_display;
        if (D_status == STATUS_ON_WIN)
          RemoveStatus();
      }

  display = (struct display *)data;
  if (D_fore == 0)
    size = IOSIZE;
  else
    {
#ifdef PSEUDOS
      if (W_UWP(D_fore))
	size = sizeof(D_fore->w_pwin->p_inbuf) - D_fore->w_pwin->p_inlen;
      else
#endif
	size = sizeof(D_fore->w_inbuf) - D_fore->w_inlen;
    }

  if (size > IOSIZE)
    size = IOSIZE;
  if (size <= 0)
    size = 1;     /* Always allow one char for command keys */

  size = read(D_userfd, buf, size);
  if (size < 0)
    {
      if (errno == EINTR)
	return;
      debug1("Read error: %d - SigHup()ing!\n", errno);
      SigHup(SIGARG);
      sleep(1);
      return;
    }
  else if (size == 0)
    {
      debug("Found EOF - SigHup()ing!\n");
      SigHup(SIGARG);
      sleep(1);
      return;
    }
  (*D_processinput)(buf, size);
}

static void
disp_status_fn(ev, data)
struct event *ev;
char *data;
{
  display = (struct display *)data;
  if (D_status)
    RemoveStatus();
}

static void
disp_hstatus_fn(ev, data)
struct event *ev;
char *data;
{
  display = (struct display *)data;
  RefreshHStatus();
}

static void
cv_winid_fn(ev, data)
struct event *ev;
char *data;
{
  int ox, oy;
  struct canvas *cv = (struct canvas *)data;

  display = cv->c_display;
  ox = D_x;
  oy = D_y;
  if (cv->c_ye + 1 < D_height)
    RefreshLine(cv->c_ye + 1, 0, D_width - 1, 0);
  if (ox != -1 && oy != -1)
    GotoPos(ox, oy);
}

#ifdef MAPKEYS
static void
disp_map_fn(ev, data)
struct event *ev;
char *data;
{
  char *p;
  int l;
  display = (struct display *)data;
  debug("Flushing map sequence\n");
  if (!(l = D_seql))
    return;
  p = D_seqp - l;
  D_seqp = D_kmaps[0].seq;
  D_seql = 0;
  ProcessInput2(p, l);
}
#endif
