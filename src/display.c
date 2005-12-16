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

static void CountChars __P((int));
static void PutChar __P((int));
static int  BlankResize __P((int, int));


extern struct win *windows;

extern int  use_hardstatus;
extern int  MsgMinWait;
extern int  Z0width, Z1width;
extern char *blank, *null;
extern struct mline mline_blank, mline_null;
extern struct mchar mchar_null, mchar_blank, mchar_so;

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
  DisplayLine(&mline_null, &mline_blank, y, xs, xe);
}

/*ARGSUSED*/
int
DefRewrite(y, xs, xe, doit)
int y, xs, xe, doit;
{
  return EXPENSIVE;
}

void
DefSetCursor()
{
  GotoPos(0, 0);
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
  InsertMode(0);
  ChangeScrollRegion(0, D_height - 1);
  KeypadMode(0);
  CursorkeysMode(0);
  CursorVisibility(0);
  SetRendition(&mchar_null);
  SetFlow(FLOW_NOW);
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
  DefSetCursor,
  BlankResize,
  DefRestore
};

struct layer BlankLayer =
{
  0,
  0,
  &BlankLf,
  0
};

/*ARGSUSED*/
static int
BlankResize(wi, he)
int wi, he;
{
  return 0;
}


/*
 *  Generate new display
 */

struct display *
MakeDisplay(uname, utty, term, fd, pid, Mode)
char *uname, *utty, *term;
int fd, pid;
struct mode *Mode;
{
  struct user **u;

  if (!*(u = FindUserPtr(uname)) && UserAdd(uname, (char *)0, u))
    return 0;	/* could not find or add user */

#ifdef MULTI
  if ((display = (struct display *)malloc(sizeof(*display))) == 0)
    return 0;
  bzero((char *) display, sizeof(*display));
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
  D_OldMode = *Mode;
  Resize_obuf();  /* Allocate memory for buffer */
  D_obufmax = defobuflimit;
#ifdef AUTO_NUKE
  D_auto_nuke = defautonuke;
#endif
  D_obufp = D_obuf;
  D_printfd = -1;
  D_userpid = pid;
#if defined(POSIX) || defined(TERMIO)
# ifdef POSIX
  switch (cfgetospeed(&D_OldMode.tio))
# else
  switch (D_OldMode.tio.c_cflag & CBAUD)
# endif
    {
#ifdef B0
    case B0: D_dospeed = 0; break;
#endif
#ifdef B50
    case B50: D_dospeed = 1; break;
#endif
#ifdef B75
    case B75: D_dospeed = 2; break;
#endif
#ifdef B110
    case B110: D_dospeed = 3; break;
#endif
#ifdef B134
    case B134: D_dospeed = 4; break;
#endif
#ifdef B150
    case B150: D_dospeed = 5; break;
#endif
#ifdef B200
    case B200: D_dospeed = 6; break;
#endif
#ifdef B300
    case B300: D_dospeed = 7; break;
#endif
#ifdef B600
    case B600: D_dospeed = 8; break;
#endif
#ifdef B1200
    case B1200: D_dospeed = 9; break;
#endif
#ifdef B1800
    case B1800: D_dospeed = 10; break;
#endif
#ifdef B2400
    case B2400: D_dospeed = 11; break;
#endif
#ifdef B4800
    case B4800: D_dospeed = 12; break;
#endif
#ifdef B9600
    case B9600: D_dospeed = 13; break;
#endif
#ifdef EXTA
    case EXTA: D_dospeed = 14; break;
#endif
#ifdef EXTB
    case EXTB: D_dospeed = 15; break;
#endif
#ifdef B57600
    case B57600: D_dospeed = 16; break;
#endif
#ifdef B115200
    case B115200: D_dospeed = 17; break;
#endif
    default: ;
    }
#else	/* POSIX || TERMIO */
  D_dospeed = (short) D_OldMode.m_ttyb.sg_ospeed;
#endif	/* POSIX || TERMIO */
  debug1("New displays ospeed = %d\n", D_dospeed);
  strncpy(D_usertty, utty, sizeof(D_usertty) - 1);
  D_usertty[sizeof(D_usertty) - 1] = 0;
  strncpy(D_termname, term, sizeof(D_termname) - 1);
  D_termname[sizeof(D_termname) - 1] = 0;
  D_user = *u;
  D_lay = &BlankLayer;
  D_layfn = BlankLayer.l_layfn;
  return display;
}

void
FreeDisplay()
{
  struct win *p;
#ifdef MULTI
  struct display *d, **dp;
#endif

  FreeTransTable();
  freetty();
  if (D_tentry)
    free(D_tentry);
  D_tentry = 0;
  D_tcinited = 0;
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
  free((char *)display);
#else /* MULTI */
  ASSERT(display == displays);
  ASSERT(display == &TheDisplay);
  displays = 0;
#endif /* MULTI */
  for (p = windows; p; p = p->w_next)
    {
      if (p->w_display == display)
	p->w_display = 0;
      if (p->w_pdisplay == display)
	p->w_pdisplay = 0;
    }
  display = 0;
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
  /* Check for toggle */
#ifdef MAPKEYS
  PutStr(D_KS);
  PutStr(D_CCS);
#else
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
      DefRestore();
      SetRendition(&mchar_null);
#ifdef MAPKEYS
      PutStr(D_KE);
      PutStr(D_CCE);
#endif
      if (D_hstatus)
	PutStr(D_DS);
      D_x = D_y = -1;
      GotoPos(0, D_height - 1);
      AddChar('\n');
      PutStr(D_TE);
    }
  Flush();
}


void
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
}

/*
 * RAW_PUTCHAR() is for all text that will be displayed.
 * NOTE: charset Nr. 0 has a conversion table, but c1, c2, ... don't.
 */

void
RAW_PUTCHAR(c)
int c;
{
  ASSERT(display);
#ifdef KANJI
  if (D_rend.font == KANJI)
    {
      int t = c;
      if (D_mbcs == 0)
	{
	  D_mbcs = c;
	  return;
	}
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
#endif
  if (D_xtable && D_xtable[(int)(unsigned char)D_rend.font] && D_xtable[(int)(unsigned char)D_rend.font][(int)(unsigned char)c])
    AddStr(D_xtable[(int)(unsigned char)D_rend.font][(int)(unsigned char)c]);
  else
    AddChar(D_rend.font != '0' ? c : D_c0_tab[(int)(unsigned char)c]);

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

static void
PutChar(c)
int c;
{
  /* this PutChar for ESC-sequences only (AddChar is a macro) */
  AddChar(c);
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
      if (D_insert)
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
      if (D_keypad)
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
      if (D_cursorkeys)
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
static void
CountChars(c)
int c;
{
  StrCost++;
}

int
CalcCost(s)
register char *s;
{
  ASSERT(display);
  if (!s)
    return EXPENSIVE;
  StrCost = 0;
  ospeed = D_dospeed;
  tputs(s, 1, CountChars);
  return StrCost;
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
    if (D_CLP && D_AM)
      x1 = -1;		/* don't know how the terminal treats this */
    else
      x1--;
  if (x2 == D_width)
    x2--;
  dx = x2 - x1;
  dy = y2 - y1;
  if (dy == 0 && dx == 0)
    return;
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
	  if (dx < costx && (m = Rewrite(y1, x1, x2, 0)) < costx)
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
  if (x2 + D_CRcost < costx && (m = (x2 ? Rewrite(y1, 0, x2, 0) : 0) + D_CRcost) < costx)
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
	(void) Rewrite(y1, x1, x2, 1);
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
        ClearLine(y, x1, xxe);
      else
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

  ASSERT(display);
  DefRestore();
  ClearDisplay();
  stop = D_height;
  i = 0;
  if (cur_only > 0 && D_fore)
    {
      i = stop = D_fore->w_y;
      stop++;
    }
  else
    RedisplayLine(-1, 0, D_width - 1, 1);
  for (; i < stop; i++)
    RedisplayLine(i, 0, D_width - 1, 1);
  RefreshStatus();
  Restore();
  SetCursor();
}


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
        FixLP(D_width - 1 - n, y);
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
  if (xs != 0 || xe != D_width - 1)
    {
      Redisplay(0);
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
  if (D_bot != ye)
    ChangeScrollRegion(ys, ye);
  alok = (D_AL || D_CAL || (ye == D_bot &&  up));
  dlok = (D_DL || D_CDL || (ye == D_bot && !up));
  if (D_top != ys && !(alok && dlok))
    ChangeScrollRegion(ys, ye);

  if (D_lp_missing && 
      (oldbot != D_bot ||
       (oldbot == D_bot && up && D_top == ys && D_bot == ye)))
    {
      FixLP(D_width - 1, oldbot);
      if (oldbot == D_bot)		/* have scrolled */
	{
	  if (--n == 0)
	    {
	      ChangeScrollRegion(oldtop, oldbot);
	      return;
	    }
	}
    }

  aldlfaster = (n > 1 && ye == D_bot && ((up && D_CDL) || (!up && D_CAL)));

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
      Redisplay(0);
      return;
    }
  if (D_lp_missing && missy != D_bot)
    FixLP(D_width - 1, missy);
  ChangeScrollRegion(oldtop, oldbot);
  if (D_lp_missing && missy != D_bot)
    FixLP(D_width - 1, missy);
}

void
SetAttr(new)
register int new;
{
  register int i, j, old, typ;

  if (!display || (old = D_rend.attr) == new)
    return;
#if defined(TERMINFO) && defined(USE_SGR)
  debug1("USE_SGR defined, sa is %s\n", D_SA ? D_SA : "undefined");
  if (D_SA)
    {
      char *tparm();
      SetFont(ASCII);
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

  if (!D_CAX && ((f == 0 && f != of) || (b == 0 && b != ob)))
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
  if (D_rend.font != mc->font)
    SetFont(mc->font);
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
  if (D_rend.font != ml->font[x])
    SetFont(ml->font[x]);
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
      if (!D_status_bell)
	{
	  ti = time((time_t *) 0) - D_status_time;
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
  if (t > msg)
    {
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
      D_status = 1;
      D_status_len = t - msg;
      D_status_lastx = D_x;
      D_status_lasty = D_y;
      if (!use_hardstatus || !D_HS)
	{
	  debug1("using STATLINE %d\n", STATLINE);
	  GotoPos(0, STATLINE);
          SetRendition(&mchar_so);
	  InsertMode(0);
	  AddStr(msg);
          D_x = -1;
	}
      else
	{
	  debug("using HS\n");
          SetRendition(&mchar_null);
	  InsertMode(0);
	  if (D_hstatus)
	    PutStr(D_DS);
	  CPutStr(D_TS, 0);
	  AddStr(msg);
	  PutStr(D_FS);
	  D_hstatus = 1;
	}
      Flush();
      (void) time(&D_status_time);
    }
}

void
RemoveStatus()
{
  struct win *p;

  if (!display)
    return;
  if (!D_status)
    return;
  
  /*
   * UGLY HACK ALERT - this should NOT be in display.c
   * We need to find the window that caused an activity or bell
   * message, to reenable this function there.
   */
  for (p = windows; p; p = p->w_next)
    { 
      if (p->w_display != display)
	continue;
      if (p->w_monitor == MON_MSG)
	{
	  debug1("RemoveStatus clearing monitor win %d\n", p->w_number);
	  p->w_monitor = MON_DONE;
	}
      if (p->w_bell == BELL_MSG)
	{
	  debug1("RemoveStatus clearing bell win %d\n", p->w_number);
	  p->w_bell = BELL_DONE;
	}
    }
  D_status = 0;
  D_status_bell = 0;
  if (!use_hardstatus || !D_HS)
    {
      GotoPos(0, STATLINE);
      RefreshLine(STATLINE, 0, D_status_len - 1, 0);
      GotoPos(D_status_lastx, D_status_lasty);
    }
  else
    {
      /*
      SetRendition(&mchar_null);
      if (D_hstatus)
        PutStr(D_DS);
      */
      RefreshStatus();
    }
  SetCursor();
}

/*
 *  Refreshes the harstatus of the _window_. Shouldn't be here...
 */

void
RefreshStatus()
{
  char *buf;

  if (D_HS)
    {
      SetRendition(&mchar_null);
      if (D_hstatus)
        PutStr(D_DS);
      if (D_fore && D_fore->w_hstatus)
	{
	  buf = MakeWinMsg(D_fore->w_hstatus, D_fore, '\005');
	  CPutStr(D_TS, 0);
	  if (strlen(buf) > D_WS)
	    AddStrn(buf, D_WS);
	  else
	    AddStr(buf);
	  PutStr(D_FS);
	  D_hstatus = 1;
	}
    }
  else if (D_fore && D_fore->w_hstatus)
    {
      buf = MakeWinMsg(D_fore->w_hstatus, D_fore, '\005');
      Msg(0, "%s", buf);
    }
}

void
RefreshLine(y, from, to, isblank)
int y, from, to, isblank;
{
  ASSERT(display);
  debug2("RefreshLine %d %d", y, from);
  debug2(" %d %d\n", to, isblank);
  if (isblank == 0 && D_CE && to == D_width - 1)
    {
      GotoPos(from, y);
      if (D_UT)
	SetRendition(&mchar_null);
      PutStr(D_CE);
      isblank = 1;
    }
  RedisplayLine(y, from, to, isblank);
}

void
FixLP(x2, y2)
register int x2, y2;
{
  struct mchar oldrend;

  ASSERT(display);
  oldrend = D_rend;
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
  for (x = from; x <= to; x++)
    {
      if (x || D_x != D_width || D_y != y - 1)
        {
	  if (x < to || x != D_width - 1 || ml->image[x + 1] == ' ')
	    if (cmp_mline(oml, ml, x))
	      continue;
	  GotoPos(x, y);
        }
#ifdef KANJI
      if (badkanji(ml->font, x))
	{
	  x--;
	  GotoPos(x, y);
	}
#endif
      SetRenditionMline(ml, x);
      PUTCHAR(ml->image[x]);
#ifdef KANJI
      if (ml->font[x] == KANJI)
	PUTCHAR(ml->image[++x]);
#endif
    }
  if (to == D_width - 1 && y < D_height - 1 && ml->image[to + 1] == ' ')
    GotoPos(0, y + 1);
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
	{
	  SetRendition(&mchar_null);
	}
      if (D_DC)
	PutStr(D_DC);
      else if (D_CDC)
	CPutStr(D_CDC, 1);
      else if (D_CE)
	PutStr(D_CE);
    }
}

void
SetLastPos(x,y)
int x,y;
{
  ASSERT(display);
  D_x = x;
  D_y = y;
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
  if (D_CWS)
    {
      debug("ResizeDisplay: using WS\n");
      PutStr(tgoto(D_CWS, wi, he));
      ChangeScreenSize(wi, he, 0);
      return 0;
    }
  else if (D_CZ0 && (wi == Z0width || wi == Z1width))
    {
      debug("ResizeDisplay: using Z0/Z1\n");
      PutStr(wi == Z0width ? D_CZ0 : D_CZ1);
      ChangeScreenSize(wi, D_height, 0);
      return (he == D_height) ? 0 : -1;
    }
  return -1;
}

void
ChangeScrollRegion(newtop, newbot)
int newtop, newbot;
{
  if (display == 0)
    return;
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
 *  Layer creation / removal
 */

int
InitOverlayPage(datasize, lf, block)
int datasize;
struct LayFuncs *lf;
int block;
{
  char *data;
  struct layer *newlay;

  RemoveStatus();
  debug3("Entering new layer  display %#x  D_fore %#x  oldlay %#x\n", 
  	 (unsigned int)display, (unsigned int)D_fore, (unsigned int)D_lay);
  if ((newlay = (struct layer *)malloc(sizeof(struct layer))) == 0)
    {
      Msg(0, "No memory for layer struct");
      return -1;
    }
  data = 0;
  if (datasize)
    {
      if ((data = malloc(datasize)) == 0)
	{
	  free((char *)newlay);
	  Msg(0, "No memory for layer data");
	  return -1;
	}
      bzero(data, datasize);
    }
  newlay->l_layfn = lf;
  newlay->l_block = block | D_lay->l_block;
  newlay->l_data = data;
  newlay->l_next = D_lay;
  if (D_fore)
    {
      D_fore->w_lay = newlay;	/* XXX: CHECK */
      D_fore->w_active = 0;	/* XXX: CHECK */
    }
  D_lay = newlay;
  D_layfn = newlay->l_layfn;
  Restore();
  return 0;
}

void
ExitOverlayPage()
{
  struct layer *oldlay;

  debug3("Exiting layer  display %#x  fore %#x  D_lay %#x\n", 
         (unsigned int)display, (unsigned int)D_fore, (unsigned int)D_lay);
  oldlay = D_lay;
  if (oldlay->l_data)
    free(oldlay->l_data);
  D_lay = oldlay->l_next;
  D_layfn = D_lay->l_layfn;
  free((char *)oldlay);
  if (D_fore)
    D_fore->w_lay = D_lay;	/* XXX: Is this necessary ? */
  Restore();
  SetCursor();
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
      PutStr(D_SE);
      PutStr(D_UE);
    }
  /* FIXME: reset color! */
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
  if (f[x] != KANJI)
    return 0;
  for (i = j = 0; i < x; i++)
    if (*f++ == KANJI)
      j ^= 1;
  return j;
}
#endif
