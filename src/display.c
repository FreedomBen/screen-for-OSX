/* Copyright (c) 1991
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Noteworthy contributors to screen's design and implementation:
 *	Wayne Davison (davison@borland.com)
 *	Patrick Wolfe (pat@kai.com, kailand!pat)
 *	Bart Schaefer (schaefer@cse.ogi.edu)
 *	Nathan Glasser (nathan@brokaw.lcs.mit.edu)
 *	Larry W. Virden (lvirden@cas.org)
 *	Howard Chu (hyc@hanauma.jpl.nasa.gov)
 *	Tim MacKenzie (tym@dibbler.cs.monash.edu.au)
 *	Markku Jarvinen (mta@{cc,cs,ee}.tut.fi)
 *	Marc Boucher (marc@CAM.ORG)
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


extern char *tgoto __P((char *, int, int));

extern struct display *display, *displays;
extern struct win *windows;
extern int  use_hardstatus;
extern int  MsgMinWait;
extern int  Z0width, Z1width;
extern char *blank, *null;

/*
 * tputs needs this to calculate the padding
 */
#ifndef NEED_OSPEED
extern
#endif /* NEED_OSPEED */
short ospeed;

#ifndef MULTI
struct display TheDisplay;
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
  DisplayLine(null, null, null, blank, null, null, y, xs, xe);
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
  ChangeScrollRegion(0, d_height - 1);
  KeypadMode(0);
  CursorkeysMode(0);
  SetAttrFont(0, ASCII);
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
#ifdef MULTI
  if ((display = (struct display *)malloc(sizeof(*display))) == 0)
    return 0;
  bzero((char *) display, sizeof(*display));
#else
  if (displays)
    return 0;
  display = &TheDisplay;
#endif
  display->_d_next = displays;
  displays = display;
  d_flow = 1;
  d_userfd = fd;
  d_OldMode = *Mode;
  Resize_obuf();  /* Allocate memory for buffer */
  d_obufmax = OBUF_MAX;
  d_obufp = d_obuf;
  d_userpid = pid;
#ifdef POSIX
  d_dospeed = (short) cfgetospeed(&d_OldMode.tio);
#else
# ifndef TERMIO
  d_dospeed = (short) d_OldMode.m_ttyb.sg_ospeed;
# endif
#endif
  debug1("New displays ospeed = %d\n", d_dospeed);
  strcpy(d_usertty, utty);
  strcpy(d_termname, term);
  strncpy(d_user, uname, sizeof(d_user) - 1);
  d_lay = &BlankLayer;
  d_layfn = BlankLayer.l_layfn;
  return display;
}

void
FreeDisplay()
{
  struct display *d, **dp;

#ifdef MULTI
  for (dp = &displays; (d = *dp) ; dp = &d->_d_next)
    if (d == display)
      break;
  ASSERT(d);
  if (d_status_lastmsg)
    free(d_status_lastmsg);
# ifdef COPY_PASTE
  if (d_copybuffer)
    free(d_copybuffer);
# endif
  if (d_obuf)
    free(d_obuf);
  *dp = display->_d_next;
  free(display);
#else
  ASSERT(display == displays);
  ASSERT(display == &TheDisplay);
  displays = 0;
#endif
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
  d_top = d_bot = -1;
  PutStr(TI);
  PutStr(IS);
  /* Check for toggle */
  if (IM && strcmp(IM, EI))
    PutStr(EI);
  d_insert = 0;
  /* Check for toggle */
  if (KS && strcmp(KS, KE))
    PutStr(KE);
  d_keypad = 0;
  if (CCS && strcmp(CCS, CCE))
    PutStr(CCE);
  d_cursorkeys = 0;
  PutStr(CE0);
  d_font = ASCII;
  if (adapt == 0)
    ResizeDisplay(d_defwidth, d_defheight);
  ChangeScrollRegion(0, d_height - 1);
  d_x = d_y = 0;
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
  ResizeDisplay(d_defwidth, d_defheight);
  DefRestore();
  SetAttrFont(0, ASCII);
  d_x = d_y = -1;
  GotoPos(0, d_height - 1);
  AddChar('\n');
  PutStr(TE);
  Flush();
}


void
INSERTCHAR(c)
int c;
{
  ASSERT(display);
  if (!d_insert && d_x < d_width - 1)
    {
      if (IC || CIC)
	{
	  if (IC)
	    PutStr(IC);
	  else
	    CPutStr(CIC, 1);
	  RAW_PUTCHAR(c);
	  return;
	}
      InsertMode(1);
      if (!d_insert)
	{
          RefreshLine(d_y, d_x, d_width-1, 0);
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
  if (d_insert && d_x < d_width - 1)
    InsertMode(0);
  RAW_PUTCHAR(c);
}

void
PUTCHARLP(c)
int c;
{
  if (d_x < d_width - 1)
    {
      if (d_insert)
	InsertMode(0);
      RAW_PUTCHAR(c);
      return;
    }
  if (CLP || d_y != d_bot)
    {
      RAW_PUTCHAR(c);
      return;
    }
  d_lp_missing = 1;
  d_lp_image = c;
  d_lp_attr = d_attr;
  d_lp_font = d_font;
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
  if (d_font == '0')
    {
      AddChar(d_c0_tab[c]);
    }
  else
    AddChar(c);
  if (++d_x >= d_width)
    {
      if ((AM && !CLP) || d_x > d_width)
	{
	  d_x -= d_width;
	  if (d_y < d_height-1 && d_y != d_bot)
	    d_y++;
	}
    }
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
      ospeed = d_dospeed;
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
      ospeed = d_dospeed;
      tputs(tgoto(s, 0, c), 1, PutChar);
    }
}


/* Insert mode is a toggle on some terminals, so we need this hack:
 */
void
InsertMode(on)
int on;
{
  if (display && on != d_insert && IM)
    {
      d_insert = on;
      if (d_insert)
	PutStr(IM);
      else
	PutStr(EI);
    }
}

/* ...and maybe d_keypad application mode is a toggle, too:
 */
void
KeypadMode(on)
int on;
{
  if (display && d_keypad != on && KS)
    {
      d_keypad = on;
      if (d_keypad)
	PutStr(KS);
      else
	PutStr(KE);
    }
}

void
CursorkeysMode(on)
int on;
{
  if (display && d_cursorkeys != on && CCS)
    {
      d_cursorkeys = on;
      if (d_cursorkeys)
	PutStr(CCS);
      else
	PutStr(CCE);
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
  if (s)
    {
      StrCost = 0;
      ospeed = d_dospeed;
      tputs(s, 1, CountChars);
      return StrCost;
    }
  else
    return EXPENSIVE;
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

  x1 = d_x;
  y1 = d_y;

  if (x1 == d_width)
    if (CLP && AM)
      x1 = -1;		/* don't know how the terminal treats this */
    else
      x1--;
  if (x2 == d_width)
    x2--;
  dx = x2 - x1;
  dy = y2 - y1;
  if (dy == 0 && dx == 0)
    {
      return;
    }
  if (!MS && d_attr)	/* Safe to move in SO mode ? */
    SetAttr(0);
  if (y1 < 0			/* don't know the y position */
      || (y2 > d_bot && y1 <= d_bot)	/* have to cross border */
      || (y2 < d_top && y1 >= d_top))	/* of scrollregion ?    */
    {
    DoCM:
      if (HO && !x2 && !y2)
        PutStr(HO);
      else
        PutStr(tgoto(CM, x2, y2));
      d_x = x2;
      d_y = y2;
      return;
    }
  /* Calculate CMcost */
  if (HO && !x2 && !y2)
    s = HO;
  else
    s = tgoto(CM, x2, y2);
  CMcost = CalcCost(s);

  /* Calculate the cost to move the cursor to the right x position */
  costx = EXPENSIVE;
  if (x1 >= 0)	/* relativ x positioning only if we know where we are */
    {
      if (dx > 0)
	{
	  if (CRI && (dx > 1 || !ND))
	    {
	      costx = CalcCost(tgoto(CRI, 0, dx));
	      xm = M_CRI;
	    }
	  if ((m = d_NDcost * dx) < costx)
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
	  if (CLE && (dx < -1 || !BC))
	    {
	      costx = CalcCost(tgoto(CLE, 0, -dx));
	      xm = M_CLE;
	    }
	  if ((m = -dx * d_LEcost) < costx)
	    {
	      costx = m;
	      xm = M_LE;
	    }
	}
      else
	costx = 0;
    }
  /* Speedup: Rewrite() >= x2 */
  if (x2 + d_CRcost < costx && (m = (x2 ? Rewrite(y1, 0, x2, 0) : 0) + d_CRcost) < costx)
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
      if (CDO && dy > 1)	/* DO & NL are always != 0 */
	{
	  costy = CalcCost(tgoto(CDO, 0, dy));
	  ym = M_CDO;
	}
      if ((m = dy * ((x2 == 0) ? d_NLcost : d_DOcost)) < costy)
	{
	  costy = m;
	  ym = M_DO;
	}
    }
  else if (dy < 0)
    {
      if (CUP && (dy < -1 || !UP))
	{
	  costy = CalcCost(tgoto(CUP, 0, -dy));
	  ym = M_CUP;
	}
      if ((m = -dy * d_UPcost) < costy)
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
	PutStr(BC);
      break;
    case M_CLE:
      CPutStr(CLE, -dx);
      break;
    case M_RI:
      while (dx-- > 0)
	PutStr(ND);
      break;
    case M_CRI:
      CPutStr(CRI, dx);
      break;
    case M_CR:
      PutStr(CR);
      d_x = 0;
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
	PutStr(UP);
      break;
    case M_CUP:
      CPutStr(CUP, -dy);
      break;
    case M_DO:
      s =  (x2 == 0) ? NL : DO;
      while (dy-- > 0)
	PutStr(s);
      break;
    case M_CDO:
      CPutStr(CDO, dy);
      break;
    default:
      break;
    }
  d_x = x2;
  d_y = y2;
}

void
ClearDisplay()
{
  ASSERT(display);
  Clear(0, 0, d_width - 1, d_height - 1);
}

void
Clear(xs, ys, xe, ye)
int xs, ys, xe, ye;
{
  int y, xxe;

  ASSERT(display);
  if (xs == d_width)
    xs--;
  if (xe == d_width)
    xe--;
  if (d_lp_missing && ys <= d_bot)
    {
      if (ye > d_bot || (ye == d_bot && xe == d_width - 1))
	d_lp_missing = 0;
    }
  if (xe == d_width - 1 && ye == d_height - 1)
    {
#ifdef AUTO_NUKE
      if (xs == 0 && ys == 0 && d_auto_nuke)
	NukePending();
#endif
      if (xs == 0 && ys == 0 && CL)
	{
	  PutStr(CL);
	  d_y = d_x = 0;
	  return;
	}
      /* 
       * Workaround a hp700/22 terminal bug. Do not use CD where CE
       * is also appropriate.
       */
      if (CD && (ys < ye || !CE))
	{
	  GotoPos(xs, ys);
	  PutStr(CD);
	  return;
	}
    }
  xxe = d_width - 1;
  for (y = ys; y <= ye; y++, xs = 0)
    {
      if (y == ye)
	xxe = xe;
      if (xs == 0 && CB && (xxe != d_width - 1 || (d_x == xxe && d_y == y)))
	{
	  GotoPos(xxe, y);
	  PutStr(CB);
	  continue;
	}
      if (xxe == d_width - 1 && CE)
	{
	  GotoPos(xs, y);
	  PutStr(CE);
	  continue;
	}
      ClearLine(y, xs, xxe);
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
  stop = d_height;
  i = 0;
  if (cur_only > 0 && d_fore)
    {
      i = stop = d_fore->w_y;
      stop++;
    }
  else RedisplayLine(-1, 0, d_width - 1, 1);
  for (; i < stop; i++)
    RedisplayLine(i, 0, d_width - 1, 1);
  SetCursor();
  Restore();
}


void
ScrollRegion(ys, ye, n)
int ys, ye, n;
{
  int i;
  int up;
  int oldtop, oldbot;
  int alok, dlok, aldlfaster;
  int missy = 0;

  ASSERT(display);
  if (n == 0)
    return;
  if (ys == 0 && ye == d_height - 1 && 
      (n >= d_height || -n >= d_height))
    {
      ClearDisplay();
      return;
    }

  if (d_lp_missing)
    {
      if (d_bot > ye || d_bot < ys)
	missy = d_bot;
      else
	{
	  missy = d_bot - n;
          if (missy>ye || missy<ys)
	    d_lp_missing = 0;
	}
    }

  up = 1;
  if (n < 0)
    {
      up = 0;
      n = -n;
    }
  if (n >= ye-ys+1)
    n = ye-ys+1;

  oldtop = d_top;
  oldbot = d_bot;
  if (d_bot != ye)
    ChangeScrollRegion(ys, ye);
  alok = (AL || CAL || (ye == d_bot &&  up));
  dlok = (DL || CDL || (ye == d_bot && !up));
  if (d_top != ys && !(alok && dlok))
    ChangeScrollRegion(ys, ye);

  if (d_lp_missing && 
      (oldbot != d_bot ||
       (oldbot == d_bot && up && d_top == ys && d_bot == ye)))
    {
      FixLP(d_width - 1, oldbot);
      if (oldbot == d_bot)		/* have scrolled */
	{
	  if (--n == 0)
	    {
	      ChangeScrollRegion(oldtop, oldbot);
	      return;
	    }
	}
    }

  aldlfaster = (n > 1 && ye == d_bot && ((up && CDL) || (!up && CAL)));

  if ((up || SR) && d_top == ys && d_bot == ye && !aldlfaster)
    {
      if (up)
	{
	  GotoPos(0, ye);
	  while (n-- > 0)
	    PutStr(NL);		/* was SF, I think NL is faster */
	}
      else
	{
	  GotoPos(0, ys);
	  while (n-- > 0)
	    PutStr(SR);
	}
    }
  else if (alok && dlok)
    {
      if (up || ye != d_bot)
	{
          GotoPos(0, up ? ys : ye+1-n);
          if (CDL && !(n == 1 && DL))
	    CPutStr(CDL, n);
	  else
	    for(i = n; i--; )
	      PutStr(DL);
	}
      if (!up || ye != d_bot)
	{
          GotoPos(0, up ? ye+1-n : ys);
          if (CAL && !(n == 1 && AL))
	    CPutStr(CAL, n);
	  else
	    for(i = n; i--; )
	      PutStr(AL);
	}
    }
  else
    {
      Redisplay(0);
      return;
    }
  if (d_lp_missing && missy != d_bot)
    FixLP(d_width - 1, missy);
  ChangeScrollRegion(oldtop, oldbot);
  if (d_lp_missing && missy != d_bot)
    FixLP(d_width - 1, missy);
}

void
SetAttr(new)
register int new;
{
  register int i, old;

  if (!display || (old = d_attr) == new)
    return;
  d_attr = new;
  for (i = 1; i <= A_MAX; i <<= 1)
    {
      if ((old & i) && !(new & i))
	{
	  PutStr(UE);
	  PutStr(SE);
	  PutStr(ME);
	  if (new & A_DI)
	    PutStr(d_attrtab[ATTR_DI]);
	  if (new & A_US)
	    PutStr(d_attrtab[ATTR_US]);
	  if (new & A_BD)
	    PutStr(d_attrtab[ATTR_BD]);
	  if (new & A_RV)
	    PutStr(d_attrtab[ATTR_RV]);
	  if (new & A_SO)
	    PutStr(d_attrtab[ATTR_SO]);
	  if (new & A_BL)
	    PutStr(d_attrtab[ATTR_BL]);
	  return;
	}
    }
  if ((new & A_DI) && !(old & A_DI))
    PutStr(d_attrtab[ATTR_DI]);
  if ((new & A_US) && !(old & A_US))
    PutStr(d_attrtab[ATTR_US]);
  if ((new & A_BD) && !(old & A_BD))
    PutStr(d_attrtab[ATTR_BD]);
  if ((new & A_RV) && !(old & A_RV))
    PutStr(d_attrtab[ATTR_RV]);
  if ((new & A_SO) && !(old & A_SO))
    PutStr(d_attrtab[ATTR_SO]);
  if ((new & A_BL) && !(old & A_BL))
    PutStr(d_attrtab[ATTR_BL]);
}

void
SetFont(new)
int new;
{
  if (!display || d_font == new)
    return;
  d_font = new;
  if (new == ASCII)
    PutStr(CE0);
  else
    CPutStr(CS0, new);
}

void
SetAttrFont(newattr, newcharset)
int newattr, newcharset;
{
  SetAttr(newattr);
  SetFont(newcharset);
}

void
MakeStatus(msg)
char *msg;
{
  register char *s, *t;
  register int max, ti;

  if (!display)
    return;
  
  if (!d_tcinited)
    {
      debug("tc not inited, just writing msg\n");
      AddStr(msg);
      AddStr("\r\n");
      Flush();
      return;
    }
  if (!use_hardstatus || !HS)
    {
      max = d_width;
      if (CLP == 0)
	max--;
    }
  else
    max = WS;
  if (d_status)
    {
      if (!d_status_bell)
	{
	  ti = time((time_t *) 0) - d_status_time;
	  if (ti < MsgMinWait)
	    sleep(MsgMinWait - ti);
	}
      RemoveStatus();
    }
  for (s = t = msg; *s && t - msg < max; ++s)
    if (*s == BELL)
      PutStr(BL);
    else if (*s >= ' ' && *s <= '~')
      *t++ = *s;
  *t = '\0';
  if (t > msg)
    {
      if (t - msg >= d_status_buflen)
        {
          char *buf;
          if (d_status_lastmsg)
	    buf = realloc(d_status_lastmsg, t - msg + 1);
	  else
	    buf = malloc(t - msg + 1);
	  if (buf)
	    {
              d_status_lastmsg = buf;
              d_status_buflen = t - msg + 1;
            }
        }
      if (t - msg < d_status_buflen)
        strcpy(d_status_lastmsg, msg);
      d_status = 1;
      d_status_len = t - msg;
      d_status_lastx = d_x;
      d_status_lasty = d_y;
      if (!use_hardstatus || !HS)
	{
	  debug1("using STATLINE %d\n", STATLINE);
	  GotoPos(0, STATLINE);
          SetAttrFont(A_SO, ASCII);
	  InsertMode(0);
	  AddStr(msg);
          d_x = -1;
	}
      else
	{
	  debug("using HS\n");
          SetAttrFont(0, ASCII);
	  InsertMode(0);
	  CPutStr(TS, 0);
	  AddStr(msg);
	  PutStr(FS);
	}
      Flush();
      (void) time(&d_status_time);
    }
}

void
RemoveStatus()
{
  struct win *p;

  if (!display)
    return;
  if (!d_status)
    return;
  
  /*
   * now we need to find the window that caused an activity or bell
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
  d_status = 0;
  d_status_bell = 0;
  if (!use_hardstatus || !HS)
    {
      GotoPos(0, STATLINE);
      RefreshLine(STATLINE, 0, d_status_len - 1, 0);
      GotoPos(d_status_lastx, d_status_lasty);
    }
  else
    {
      SetAttrFont(0, ASCII);
      PutStr(DS);
    }
  SetCursor();
}

void
RefreshLine(y, from, to, isblank)
int y, from, to, isblank;
{
  ASSERT(display);
  debug2("RefreshLine %d %d", y, from);
  debug2(" %d %d\n", to, isblank);
  if (isblank == 0 && CE && to == d_width - 1)
    {
      GotoPos(from, y);
      PutStr(CE);
      isblank = 1;
    }
  RedisplayLine(y, from, to, isblank);
}

void
FixLP(x2, y2)
register int x2, y2;
{
  int oldattr = d_attr, oldfont = d_font;

  ASSERT(display);
  GotoPos(x2, y2);
  SetAttrFont(d_lp_attr, d_lp_font);
  PUTCHAR(d_lp_image);
  d_lp_missing = 0;
  SetAttrFont(oldattr, oldfont);
}

void
DisplayLine(os, oa, of, s, as, fs, y, from, to)
int from, to, y;
register char *os, *oa, *of, *s, *as, *fs;
{
  register int x;
  int last2flag = 0, delete_lp = 0;

  ASSERT(display);
  if (!CLP && y == d_bot && to == d_width - 1)
    if (d_lp_missing
	|| s[to] != os[to] || as[to] != oa[to] || of[to] != fs[to])
      {
	if ((IC || IM) && from < to)
	  {
	    to -= 2;
	    last2flag = 1;
	    d_lp_missing = 0;
	  }
	else
	  {
	    to--;
	    delete_lp = (CE || DC || CDC);
	    d_lp_missing = (s[to] != ' ' || as[to] || fs[to]);
	    d_lp_image = s[to];
	    d_lp_attr = as[to];
	    d_lp_font = fs[to];
	  }
      }
    else
      to--;
  for (x = from; x <= to; ++x)
    {
      if (x || d_x != d_width || d_y != y - 1)
        {
	  if (x < to || x != d_width - 1 || s[x + 1] == ' ')
	    if (s[x] == os[x] && as[x] == oa[x] && of[x] == fs[x])
	      continue;
	  GotoPos(x, y);
        }
      SetAttr(as[x]);
      SetFont(fs[x]);
      PUTCHAR(s[x]);
    }
  if (to == d_width - 1 && y < d_height - 1 && s[to + 1] == ' ')
    GotoPos(0, y + 1);
  if (last2flag)
    {
      GotoPos(x, y);
      SetAttr(as[x + 1]);
      SetFont(fs[x + 1]);
      PUTCHAR(s[x + 1]);
      GotoPos(x, y);
      SetAttr(as[x]);
      SetFont(fs[x]);
      INSERTCHAR(s[x]);
    }
  else if (delete_lp)
    {
      if (DC)
	PutStr(DC);
      else if (CDC)
	CPutStr(CDC, 1);
      else if (CE)
	PutStr(CE);
    }
}

void
SetLastPos(x,y)
int x,y;
{
  ASSERT(display);
  d_x = x;
  d_y = y;
}

int
ResizeDisplay(wi, he)
int wi, he;
{
  ASSERT(display);
  debug2("ResizeDisplay: to (%d,%d).\n", wi, he);
  if (d_width == wi && d_height == he)
    {
      debug("ResizeDisplay: No change\n");
      return 0;
    }
  if (CWS)
    {
      debug("ResizeDisplay: using WS\n");
      PutStr(tgoto(CWS, wi, he));
      ChangeScreenSize(wi, he, 0);
      return 0;
    }
  else if (CZ0 && (wi == Z0width || wi == Z1width))
    {
      debug("ResizeDisplay: using Z0/Z1\n");
      PutStr(wi == Z0width ? CZ0 : CZ1);
      ChangeScreenSize(wi, d_height, 0);
      return (he == d_height) ? 0 : -1;
    }
  return -1;
}

void
ChangeScrollRegion(newtop, newbot)
int newtop, newbot;
{
  if (display == 0)
    return;
  if (CS == 0)
    {
      d_top = 0;
      d_bot = d_height - 1;
      return;
    }
  if (d_top == newtop && d_bot == newbot)
    return;
  debug2("ChangeScrollRegion: (%d - %d)\n", newtop, newbot);
  PutStr(tgoto(CS, newbot, newtop));
  d_top = newtop;
  d_bot = newbot;
  d_y = d_x = -1;		/* Just in case... */
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
  debug3("Entering new layer  display %#x  d_fore %#x  oldlay %#x\n", 
  	 (unsigned int)display, (unsigned int)d_fore, (unsigned int)d_lay);
  if ((newlay = (struct layer *)malloc(sizeof(struct layer))) == 0)
    {
      Msg(0, "No memory for layer struct");
      return(-1);
    }
  data = 0;
  if (datasize)
    {
      if ((data = malloc(datasize)) == 0)
	{
	  free(newlay);
	  Msg(0, "No memory for layer data");
	  return(-1);
	}
      bzero(data, datasize);
    }
  newlay->l_layfn = lf;
  newlay->l_block = block | d_lay->l_block;
  newlay->l_data = data;
  newlay->l_next = d_lay;
  if (d_fore)
    {
      d_fore->w_lay = newlay;	/* XXX: CHECK */
      d_fore->w_active = 0;	/* XXX: CHECK */
    }
  d_lay = newlay;
  d_layfn = newlay->l_layfn;
  Restore();
  return(0);
}

void
ExitOverlayPage()
{
  struct layer *oldlay;

  debug3("Exiting layer  display %#x  fore %#x  d_lay %#x\n", 
         (unsigned int)display, (unsigned int)d_fore, (unsigned int)d_lay);
  oldlay = d_lay;
  if (oldlay->l_data)
    free(oldlay->l_data);
  d_lay = oldlay->l_next;
  d_layfn = d_lay->l_layfn;
  free(oldlay);
  if (d_fore)
    d_fore->w_lay = d_lay;	/* XXX: Is this necessary ? */
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
  l = d_obufp - d_obuf;
  debug1("Flush(): %d\n", l);
  ASSERT(l + d_obuffree == d_obuflen);
  if (l == 0)
    return;
  if (d_userfd < 0)
    {
      d_obuffree += l;
      d_obufp = d_obuf;
      return;
    }
  p = d_obuf;
  if (fcntl(d_userfd, F_SETFL, 0))
    debug1("Warning: DELAY fcntl failed: %d\n", errno);
  while (l)
    {
      register int wr;
      wr = write(d_userfd, p, l);
      if (wr <= 0) 
	{
	  if (errno == EINTR) 
	    continue;
	  debug1("Writing to display: %d\n", errno);
	  wr = l;
	}
      d_obuffree += wr;
      p += wr;
      l -= wr;
    }
  d_obuffree += l;
  d_obufp = d_obuf;
  if (fcntl(d_userfd, F_SETFL, FNDELAY))
    debug1("Warning: NDELAY fcntl failed: %d\n", errno);
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
  if (d_obuflen && d_obuf)
    {
      ind  = d_obufp - d_obuf;
      d_obuflen += GRAIN;
      d_obuffree += GRAIN;
      d_obuf = realloc(d_obuf, d_obuflen);
    }
  else
    {
      ind  = 0;
      d_obuflen = GRAIN;
      d_obuffree = GRAIN;
      d_obuf = malloc(d_obuflen);
    }
  if (!d_obuf)
    Panic(0, "Out of memory");
  d_obufp = d_obuf + ind;
  debug1("ResizeObuf: resized to %d\n", d_obuflen);
}

#ifdef AUTO_NUKE
void
NukePending()
{/* Nuke pending output in current display, clear screen */
  register int len;
  int oldfont = d_font, oldattr = d_attr, oldtop = d_top, oldbot = d_bot;
  int oldkeypad = d_keypad, oldcursorkeys = d_cursorkeys;

  len = d_obufp - d_obuf;
  debug1("NukePending: nuking %d chars\n", len);
  
  /* Throw away any output that we can... */
# ifdef POSIX
  tcflush(d_userfd, TCOFLUSH);
# else
#  ifdef TCFLUSH
  (void) ioctl(d_userfd, TCFLUSH, (char *) 1);
#  endif
# endif

  d_obufp = d_obuf;
  d_obuffree += len;
  d_top = d_bot = -1;
  PutStr(TI);
  PutStr(IS);
  /* Check for toggle */
  if (IM && strcmp(IM, EI))
    PutStr(EI);
  d_insert = 0;
  /* Check for toggle */
  if (KS && strcmp(KS, KE))
    PutStr(KE);
  d_keypad = 0;
  if (CCS && strcmp(CCS, CCE))
    PutStr(CCE);
  d_cursorkeys = 0;
  PutStr(CE0);
  d_font = ASCII;
  ChangeScrollRegion(oldtop, oldbot);
  SetAttrFont(oldattr, oldfont);
  KeypadMode(oldkeypad);
  CursorkeysMode(oldcursorkeys);
  if (CWS)
    {
      debug("ResizeDisplay: using WS\n");
      PutStr(tgoto(CWS, d_width, d_height));
    }
  else if (CZ0 && (d_width == Z0width || d_width == Z1width))
    {
      debug("ResizeDisplay: using Z0/Z1\n");
      PutStr(d_width == Z0width ? CZ0 : CZ1);
    }
}
#endif /* AUTO_NUKE */
