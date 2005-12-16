/* Copyright (c) 1991
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
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

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef sun	/* we want to know about TIOCPKT. */
# include <sys/ioctl.h>
#endif
#include "config.h"
#include "screen.h"
#include "extern.h"

extern char *getenv(), *tgetstr(), *tgoto();
#ifndef __STDC__
extern char *malloc();
#endif

extern struct win *windows;	/* linked list of all windows */
extern struct win *fore;
extern int  force_vt;
extern int  all_norefresh;	/* => display */
extern char Esc, MetaEsc;	/* => display */

int maxwidth;			/* maximum of all widths so far */

int Z0width, Z1width;		/* widths for Z0/Z1 switching */

struct display *display;

static struct win *curr;	/* window we are working on */
static int rows, cols;		/* window size of the curr window */

int default_wrap = 1;		/* default: wrap on */
int default_monitor = 0; 

int visual_bell = 0;
int use_hardstatus = 1;

char *blank;			/* line filled with spaces */
char *null;			/* line filled with '\0' */
char *OldImage, *OldAttr, *OldFont;	/* temporary buffers */

time_t time();

static void WinProcess __P((char **, int *));
static void WinRedisplayLine __P((int, int, int, int));
static void WinClearLine __P((int, int, int));
static int  WinRewrite __P((int, int, int, int));
static void WinSetCursor __P(());
static int  WinResize __P((int, int));
static void WinRestore __P((void));
static int  Special __P((int));
static void DoESC __P((int, int ));
static void DoCSI __P((int, int ));
static void SetChar __P(());
static void StartString __P((enum string_t));
static void SaveChar __P((int ));
static void PrintChar __P((int ));
static void PrintFlush __P((void));
static void DesignateCharset __P((int, int ));
static void MapCharset __P((int));
static void SaveCursor __P((void));
static void RestoreCursor __P((void));
static void BackSpace __P((void));
static void Return __P((void));
static void LineFeed __P((int));
static void ReverseLineFeed __P((void));
static void InsertAChar __P((int));
static void InsertChar __P((int));
static void DeleteChar __P((int));
static void DeleteLine __P((int));
static void InsertLine __P((int));
static void ScrollUpMap __P((int));
static void ScrollDownMap __P((int));
static void Scroll __P((char *, int, int, char *));
static void ForwardTab __P((void));
static void BackwardTab __P((void));
static void ClearScreen __P((void));
static void ClearFromBOS __P((void));
static void ClearToEOS __P((void));
static void ClearFullLine __P((void));
static void ClearToEOL __P((void));
static void ClearFromBOL __P((void));
static void ClearInLine __P((int, int, int));
static void CursorRight __P(());
static void CursorUp __P(());
static void CursorDown __P(());
static void CursorLeft __P(());
static void ASetMode __P((int));
static void SelectRendition __P((void));
static void RestorePosAttrFont __P((void));
static void FillWithEs __P((void));
static void ChangeLine __P((char *, char *, char *, int, int, int ));
static void FindAKA __P((void));
static void Report __P((char *, int, int));


/*
 *  The window layer functions
 */

struct LayFuncs WinLf =
{
  WinProcess,
  0,
  WinRedisplayLine,
  WinClearLine,
  WinRewrite,
  WinSetCursor,
  WinResize,
  WinRestore
};

static void
WinProcess(bufpp, lenp)
char **bufpp;
int *lenp;
{
  int f, *ilen, l = *lenp;
  char *ibuf;
  
  fore = d_fore;
  if (W_UWP(fore))
    {
      /* we send the d_user input to our pseudowin */
      ibuf = fore->w_pwin->p_inbuf; ilen = &fore->w_pwin->p_inlen;
      f = sizeof(fore->w_pwin->p_inbuf) - *ilen;
    }
  else
    {
      /* we send the d_user input to the window */
      ibuf = fore->w_inbuf; ilen = &fore->w_inlen;
      f = sizeof(fore->w_inbuf) - *ilen;
    }
  if (l > f)
    l = f;
  if (l > 0)
    {
      bcopy(*bufpp, ibuf + *ilen, l);
      *ilen += l;
    }
  *bufpp += *lenp;
  *lenp = 0;
}

static void
WinRedisplayLine(y, from, to, isblank)
int y, from, to, isblank;
{
  if (y < 0)
    return;
  fore = d_fore;
  DisplayLine(isblank ? blank: null, null, null, fore->w_image[y],
              fore->w_attr[y], fore->w_font[y], y, from, to);
}

static int
WinRewrite(y, x1, x2, doit)
int y, x1, x2, doit;
{
  register int cost, dx;
  register char *p, *f, *i;

  fore = d_fore;
  dx = x2 - x1;
  if (doit)
    {
      i = fore->w_image[y] + x1;
      while (dx-- > 0)
	PUTCHAR(*i++);
      return(0);
    }
  p = fore->w_attr[y] + x1;
  f = fore->w_font[y] + x1;

  cost = dx = x2 - x1;
  if (d_insert)
    cost += d_EIcost + d_IMcost;
  while(dx-- > 0)
    {
      if (*p++ != d_attr || *f++ != d_font)
	return EXPENSIVE;
    }
  return cost;
}

static void
WinClearLine(y, xs, xe)
int y, xs, xe;
{
  fore = d_fore;
  DisplayLine(fore->w_image[y], fore->w_attr[y], fore->w_font[y],
	      blank, null, null, y, xs, xe);
}

static void
WinSetCursor()
{
  fore = d_fore;
  GotoPos(fore->w_x, fore->w_y);
}

static int
WinResize(wi, he)
int wi, he;
{
  fore = d_fore;
  if (fore)
    ChangeWindowSize(fore, wi, he);
  return 0;
}

static void
WinRestore()
{
  fore = d_fore;
  ChangeScrollRegion(fore->w_top, fore->w_bot);
  KeypadMode(fore->w_keypad);
  CursorkeysMode(fore->w_cursorkeys);
  SetFlow(fore->w_flow & FLOW_NOW);
  InsertMode(fore->w_insert);
  fore->w_active = 1;
}

/* 
 *  Activate - make fore window active
 *  norefresh = -1 forces a refresh, disregard all_norefresh then.
 */
void
Activate(norefresh)
int norefresh;
{
  debug1("Activate(%d)\n", norefresh);
  if (display == 0)
    return;
  RemoveStatus();
  if ((fore = d_fore))
    {
      ASSERT(fore->w_display == display);
      fore->w_active = d_layfn == &WinLf;
      if (fore->w_monitor != MON_OFF)
	fore->w_monitor = MON_ON;
      fore->w_bell = BELL_OFF;
    }
  if (ResizeDisplay(fore->w_width, fore->w_height))
    {
      debug2("Cannot resize from (%d,%d)", d_width, d_height);
      debug2(" to (%d,%d) -> resize window\n", fore->w_width, fore->w_height);
      DoResize(d_width, d_height);
    }
  Redisplay(norefresh + all_norefresh);
}

void
ResetWindow(p)
register struct win *p;
{
  register int i;

  p->w_wrap = default_wrap;
  p->w_origin = 0;
  p->w_insert = 0;
  p->w_vbwait = 0;
  p->w_keypad = 0;
  p->w_cursorkeys = 0;
  p->w_top = 0;
  p->w_bot = p->w_height - 1;
  p->w_saved = 0;
  p->w_Attr = 0;
  p->w_x = p->w_y = 0;
  p->w_state = LIT;
  p->w_StringType = NONE;
  p->w_ss = 0;
  p->w_Charset = G0;
  bzero(p->w_tabs, p->w_width);
  for (i = 8; i < p->w_width; i += 8)
    p->w_tabs[i] = 1;
  for (i = G0; i <= G3; i++)
    p->w_charsets[i] = ASCII;
}


/*
 *  Here comes the vt100 emulator
 */
void
WriteString(wp, buf, len)
struct win *wp;
register char *buf;
register int len;
{
  register int c;

  if (!len)
    return;
  if (wp->w_logfp != NULL)
    if ((int)fwrite(buf, len, 1, wp->w_logfp) < 1)
      {
	Msg(errno, "Error writing logfile");
	fclose(wp->w_logfp);
	wp->w_logfp = NULL;
      }
  /*
   * SetCurr() here may prevent output, as it may set display = 0
   */
  SetCurr(wp);
  if (display)
    {
      if (d_status && !(use_hardstatus && HS))
	RemoveStatus();
    }
  else
    {
      if (curr->w_monitor == MON_ON || curr->w_monitor == MON_DONE)
	{
          debug2("ACTIVITY %d %d\n", curr->w_monitor, curr->w_bell);
          curr->w_monitor = MON_FOUND;
	}
    }
  do
    {
      c = (unsigned char)*buf++;
      if (c == '\177')
	continue;

      /* The next part is only for speedup */
      if (curr->w_state == LIT &&
          c >= ' ' && ((c & 0x80) == 0 || display == 0 || !CB8) &&
          !curr->w_insert && !curr->w_ss)
	{
	  register int currx;
	  register char *imp, *atp, *fop, at, fo;

	  currx = curr->w_x;
	  imp = curr->w_image[curr->w_y] + currx;
	  atp = curr->w_attr[curr->w_y] + currx;
	  fop = curr->w_font[curr->w_y] + currx;
	  at = curr->w_Attr;
	  fo = curr->w_charsets[curr->w_Charset];
	  if (display)
	    {
	      if (d_x != currx || d_y != curr->w_y)
		GotoPos(currx, curr->w_y);
	      if (at != d_attr)
		SetAttr(at);
	      if (fo != d_font)
		SetFont(fo);
	      if (d_insert)
		InsertMode(0);
	    }
	  while (currx < cols - 1)
	    {
	      if (display)
		AddChar(d_font != '0' ? c : d_c0_tab[c]);
	      *imp++ = c;
	      *atp++ = at;
	      *fop++ = fo;
	      currx++;
skip:	      if (--len == 0)
		break;
              c = (unsigned char)*buf++;
	      if (c == '\177')
		goto skip;
	      if (c < ' ' || ((c & 0x80) && display && CB8))
		break;
	    }
	  curr->w_x = currx;
	  if (display)
	    d_x = currx;
	  if (len == 0)
	    break;
	}
      /* end of speedup code */

      if ((c & 0x80) && display && CB8)
	{
	  FILE *logfp = wp->w_logfp;
	  char *cb8 = CB8;
	
	  wp->w_logfp = NULL;	/* a little hack */
	  CB8 = NULL;		/* dito */
	  WriteString(wp, cb8, strlen(cb8));
	  wp->w_logfp = logfp;
	  CB8 = cb8;
	  c &= 0x7f;
	}
    tryagain:
      switch (curr->w_state)
	{
	case PRIN:
	  switch (c)
	    {
	    case '\033':
	      curr->w_state = PRINESC;
	      break;
	    default:
	      PrintChar(c);
	    }
	  break;
	case PRINESC:
	  switch (c)
	    {
	    case '[':
	      curr->w_state = PRINCSI;
	      break;
	    default:
	      PrintChar('\033');
	      PrintChar(c);
	      curr->w_state = PRIN;
	    }
	  break;
	case PRINCSI:
	  switch (c)
	    {
	    case '4':
	      curr->w_state = PRIN4;
	      break;
	    default:
	      PrintChar('\033');
	      PrintChar('[');
	      PrintChar(c);
	      curr->w_state = PRIN;
	    }
	  break;
	case PRIN4:
	  switch (c)
	    {
	    case 'i':
	      curr->w_state = LIT;
	      PrintFlush();
	      break;
	    default:
	      PrintChar('\033');
	      PrintChar('[');
	      PrintChar('4');
	      PrintChar(c);
	      curr->w_state = PRIN;
	    }
	  break;
	case STRESC:
	  switch (c)
	    {
	    case '\\':
	      curr->w_state = LIT;
	      *(curr->w_stringp) = '\0';
	      switch (curr->w_StringType)
		{
		case PM:
		  if (!display)
		    break;
		  MakeStatus(curr->w_string);
		  if (d_status && !(use_hardstatus && HS) && len > 1)
		    {
		      curr->w_outlen = len - 1;
		      bcopy(buf, curr->w_outbuf, curr->w_outlen);
		      return;
		    }
		  break;
		case DCS:
		  if (display)
		    AddStr(curr->w_string);
		  break;
		case AKA:
		  if (curr->w_akapos == 0 && !*curr->w_string)
		    break;
		  strncpy(curr->w_cmd + curr->w_akapos, curr->w_string, 20);
		  if (!*curr->w_string)
		    curr->w_autoaka = curr->w_y + 1;
		  break;
		default:
		  break;
		}
	      break;
	    default:
	      curr->w_state = ASTR;
	      SaveChar('\033');
	      SaveChar(c);
	    }
	  break;
	case ASTR:
	  switch (c)
	    {
	    case '\0':
	      break;
	    case '\033':
	      curr->w_state = STRESC;
	      break;
	    default:
	      SaveChar(c);
	    }
	  break;
	case ESC:
	  switch (c)
	    {
	    case '[':
	      curr->w_NumArgs = 0;
	      curr->w_intermediate = 0;
	      bzero((char *) curr->w_args, MAXARGS * sizeof(int));
	      curr->w_state = CSI;
	      break;
	    case ']':
	      StartString(OSC);
	      break;
	    case '_':
	      StartString(APC);
	      break;
	    case 'P':
	      StartString(DCS);
	      break;
	    case '^':
	      StartString(PM);
	      break;
	    case '"':
	    case 'k':
	      StartString(AKA);
	      break;
	    default:
	      if (Special(c))
		break;
	      debug1("not special. c = %x\n", c);
	      if (c >= ' ' && c <= '/')
		curr->w_intermediate = curr->w_intermediate ? -1 : c;
	      else if (c >= '0' && c <= '~')
		{
		  DoESC(c, curr->w_intermediate);
		  curr->w_state = LIT;
		}
	      else
		{
		  curr->w_state = LIT;
		  goto tryagain;
		}
	    }
	  break;
	case CSI:
	  switch (c)
	    {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	      if (curr->w_NumArgs < MAXARGS)
		{
		  curr->w_args[curr->w_NumArgs] =
		    10 * curr->w_args[curr->w_NumArgs] + c - '0';
		}
	      break;
	    case ';':
	    case ':':
	      curr->w_NumArgs++;
	      break;
	    default:
	      if (Special(c))
		break;
	      if (c >= '@' && c <= '~')
		{
		  curr->w_NumArgs++;
		  DoCSI(c, curr->w_intermediate);
		  if (curr->w_state != PRIN)
		    curr->w_state = LIT;
		}
	      else if ((c >= ' ' && c <= '/') || (c >= '<' && c <= '?'))
		curr->w_intermediate = curr->w_intermediate ? -1 : c;
	      else
		{
		  curr->w_state = LIT;
		  goto tryagain;
		}
	    }
	  break;
	case LIT:
	default:
	  if (c < ' ')
	    {
	      if (c == '\033')
		{
		  curr->w_intermediate = 0;
		  curr->w_state = ESC;
		  if (display && d_lp_missing && (CIC || IC || IM))
		    {
		      ChangeLine(blank, null, null, d_bot,
				 cols - 2, cols - 1);
		      GotoPos(curr->w_x, curr->w_y);
		    }
		  if (curr->w_autoaka < 0)
		    curr->w_autoaka = 0;
		}
	      else
		Special(c);
	      break;
	    }
	  if (display)
	    {
	      if (d_attr != curr->w_Attr)
		SetAttr(curr->w_Attr);
	      if (d_font != curr->w_charsets[curr->w_ss ? curr->w_ss : curr->w_Charset])
		SetFont(curr->w_charsets[curr->w_ss ? curr->w_ss : curr->w_Charset]);
	    }
	  if (curr->w_x < cols - 1)
	    {
	      if (curr->w_insert)
		InsertAChar(c);
	      else
		{
		  if (display)
		    PUTCHAR(c);
		  SetChar(c);
		}
	      curr->w_x++;
	    }
	  else if (curr->w_x == cols - 1)
	    {
	      if (display && curr->w_wrap && (CLP || !force_vt || COP))
		{
		  RAW_PUTCHAR(c);	/* don't care about d_insert */
		  SetChar(c);
		  if (AM && !CLP)
		    LineFeed(0);	/* terminal auto-wrapped */
		  else
		    curr->w_x++;
		}
	      else
		{
		  if (display)
		    {
		      if (CLP || curr->w_y != d_bot)
			{
			  RAW_PUTCHAR(c);
			  GotoPos(curr->w_x, curr->w_y);
			}
		      else
			CheckLP(c);
		    }
		  SetChar(c);
		  if (curr->w_wrap)
		    curr->w_x++;
		}
	    }
	  else /* curr->w_x > cols - 1 */
	    {
              SetChar(0);		/* we wrapped */
	      if (curr->w_insert)
		{
		  LineFeed(2);		/* cr+lf, handle LP */
		  InsertAChar(c);
		}
	      else
		{
		  LineFeed(display == 0 || AM ? 0 : 2);
		  if (display)
		    PUTCHAR(c);
		  SetChar(c);
		}
	      curr->w_x = 1;
	    }
	  if (curr->w_ss)
	    {
	      SetFont(curr->w_charsets[curr->w_Charset]);
	      curr->w_ss = 0;
	    }
	  break;
	}
    } while (--len);
  curr->w_outlen = 0;
  if (curr->w_state == PRIN)
    PrintFlush();
}

static int
Special(c)
register int c;
{
  switch (c)
    {
    case '\b':
      BackSpace();
      return 1;
    case '\r':
      Return();
      return 1;
    case '\n':
      if (curr->w_autoaka)
	FindAKA();
      LineFeed(1);
      return 1;
    case '\007':
      if (display == 0)
	curr->w_bell = BELL_ON;
      else
	{
	  if (!visual_bell)
	    PutStr(BL);
	  else
	    {
	      if (!VB)
		curr->w_bell = BELL_VISUAL;
	      else
		PutStr(VB);
	    }
	}
      return 1;
    case '\t':
      ForwardTab();
      return 1;
    case '\017':		/* SI */
      MapCharset(G0);
      return 1;
    case '\016':		/* SO */
      MapCharset(G1);
      return 1;
    }
  return 0;
}

static void
DoESC(c, intermediate)
int c, intermediate;
{
  debug2("DoESC: %x - inter = %x\n", c, intermediate);
  switch (intermediate)
    {
    case 0:
      switch (c)
	{
	case 'E':
	  LineFeed(2);
	  break;
	case 'D':
	  LineFeed(1);
	  break;
	case 'M':
	  ReverseLineFeed();
	  break;
	case 'H':
	  curr->w_tabs[curr->w_x] = 1;
	  break;
	case 'Z':		/* jph: Identify as VT100 */
	  Report("\033[?%d;%dc", 1, 2);
	  break;
	case '7':
	  SaveCursor();
	  break;
	case '8':
	  RestoreCursor();
	  break;
	case 'c':
	  ClearScreen();
	  ResetWindow(curr);
	  SetAttrFont(0, ASCII);
	  InsertMode(0);
	  KeypadMode(0);
	  CursorkeysMode(0);
	  ChangeScrollRegion(0, rows - 1);
	  break;
	case '=':
	  KeypadMode(curr->w_keypad = 1);
#ifndef TIOCPKT
	  NewAutoFlow(curr, 0);
#endif /* !TIOCPKT */
	  break;
	case '>':
	  KeypadMode(curr->w_keypad = 0);
#ifndef TIOCPKT
	  NewAutoFlow(curr, 1);
#endif /* !TIOCPKT */
	  break;
	case 'n':		/* LS2 */
	  MapCharset(G2);
	  break;
	case 'o':		/* LS3 */
	  MapCharset(G3);
	  break;
	case 'N':		/* SS2 */
	  if (curr->w_charsets[curr->w_Charset] != curr->w_charsets[G2])
	    curr->w_ss = G2;
	  else
	    curr->w_ss = 0;
	  break;
	case 'O':		/* SS3 */
	  if (curr->w_charsets[curr->w_Charset] != curr->w_charsets[G3])
	    curr->w_ss = G3;
	  else
	    curr->w_ss = 0;
	  break;
	}
      break;
    case '#':
      switch (c)
	{
	case '8':
	  FillWithEs();
	  break;
	}
      break;
    case '(':
      DesignateCharset(c, G0);
      break;
    case ')':
      DesignateCharset(c, G1);
      break;
    case '*':
      DesignateCharset(c, G2);
      break;
    case '+':
      DesignateCharset(c, G3);
      break;
    }
}

static void
DoCSI(c, intermediate)
int c, intermediate;
{
  register int i, a1 = curr->w_args[0], a2 = curr->w_args[1];

  if (curr->w_NumArgs > MAXARGS)
    curr->w_NumArgs = MAXARGS;
  switch (intermediate)
    {
    case 0:
      switch (c)
	{
	case 'H':
	case 'f':
	  if (a1 < 1)
	    a1 = 1;
	  if (curr->w_origin)
	    a1 += curr->w_top;
	  if (a1 > rows)
	    a1 = rows;
	  if (a2 < 1)
	    a2 = 1;
	  if (a2 > cols)
	    a2 = cols;
	  GotoPos(--a2, --a1);
	  curr->w_x = a2;
	  curr->w_y = a1;
	  if (curr->w_autoaka)
	    curr->w_autoaka = a1 + 1;
	  break;
	case 'J':
	  if (a1 < 0 || a1 > 2)
	    a1 = 0;
	  switch (a1)
	    {
	    case 0:
	      ClearToEOS();
	      break;
	    case 1:
	      ClearFromBOS();
	      break;
	    case 2:
	      ClearScreen();
	      GotoPos(curr->w_x, curr->w_y);
	      break;
	    }
	  break;
	case 'K':
	  if (a1 < 0 || a1 > 2)
	    a1 %= 3;
	  switch (a1)
	    {
	    case 0:
	      ClearToEOL();
	      break;
	    case 1:
	      ClearFromBOL();
	      break;
	    case 2:
	      ClearFullLine();
	      break;
	    }
	  break;
	case 'A':
	  CursorUp(a1 ? a1 : 1);
	  break;
	case 'B':
	  CursorDown(a1 ? a1 : 1);
	  break;
	case 'C':
	  CursorRight(a1 ? a1 : 1);
	  break;
	case 'D':
	  CursorLeft(a1 ? a1 : 1);
	  break;
	case 'm':
	  SelectRendition();
	  break;
	case 'g':
	  if (a1 == 0)
	    curr->w_tabs[curr->w_x] = 0;
	  else if (a1 == 3)
	    bzero(curr->w_tabs, cols);
	  break;
	case 'r':
	  if (!a1)
	    a1 = 1;
	  if (!a2)
	    a2 = rows;
	  if (a1 < 1 || a2 > rows || a1 >= a2)
	    break;
	  curr->w_top = a1 - 1;
	  curr->w_bot = a2 - 1;
	  ChangeScrollRegion(curr->w_top, curr->w_bot);
	  if (curr->w_origin)
	    {
	      GotoPos(0, curr->w_top);
	      curr->w_y = curr->w_top;
	      curr->w_x = 0;
	    }
	  else
	    {
	      GotoPos(0, 0);
	      curr->w_y = curr->w_x = 0;
	    }
	  break;
	case 's':
	  SaveCursor();
	  break;
	case 't':
	  if (a1 != 8)
	    break;
	  a1 = curr->w_args[2];
	  if (a1 < 1)
	    a1 = curr->w_width;
	  if (a2 < 1)
	    a2 = curr->w_height;
	  if (CWS == NULL)
	    {
	      a2 = curr->w_height;
	      if (CZ0 == NULL || (a1 != Z0width && a1 != Z1width))
	        a1 = curr->w_width;
 	    }
	  if (a1 == curr->w_width && a2 == curr->w_height)
	    break;
          ChangeWindowSize(curr, a1, a2);
	  SetCurr(curr);
	  if (display)
	    Activate(0);
	  break;
	case 'u':
	  RestoreCursor();
	  break;
	case 'I':
	  if (!a1)
	    a1 = 1;
	  while (a1--)
	    ForwardTab();
	  break;
	case 'Z':
	  if (!a1)
	    a1 = 1;
	  while (a1--)
	    BackwardTab();
	  break;
	case 'L':
	  InsertLine(a1 ? a1 : 1);
	  break;
	case 'M':
	  DeleteLine(a1 ? a1 : 1);
	  break;
	case 'P':
	  DeleteChar(a1 ? a1 : 1);
	  break;
	case '@':
	  InsertChar(a1 ? a1 : 1);
	  break;
	case 'h':
	  ASetMode(1);
	  break;
	case 'l':
	  ASetMode(0);
	  break;
	case 'i':
	  if (PO && a1 == 5)
	    {
	      curr->w_stringp = curr->w_string;
	      curr->w_state = PRIN;
	    }
	  break;
	case 'n':
	  if (a1 == 6)		/* Report cursor position */
	    Report("\033[%d;%dR", curr->w_y + 1, curr->w_x + 1);
	  break;
	case 'c':		/* Identify as VT100 */
	  Report("\033[?%d;%dc", 1, 2);
	  break;
	}
      break;
    case '?':
      debug2("\\E[?%d%c\n",a1,c);
      if (c != 'h' && c != 'l')
	break;
      i = (c == 'h');
      switch (a1)
	{
	case 1:
	  CursorkeysMode(curr->w_cursorkeys = i);
#ifndef TIOCPKT
	  NewAutoFlow(curr, !i);
#endif /* !TIOCPKT */
	  break;
	case 3:
	  i = (i ? Z0width : Z1width);
	  if ((CZ0 || CWS) && curr->w_width != i)
	    {
              ChangeWindowSize(curr, i, curr->w_height);
	      SetCurr(curr);
	      if (display)
		Activate(0);
	    }
	  break;
	case 5:
	  if (i)
	    curr->w_vbwait = 1;
	  else
	    {
	      if (curr->w_vbwait)
		PutStr(VB);
	      curr->w_vbwait = 0;
	    }
	  break;
	case 6:
	  if ((curr->w_origin = i) != 0)
	    {
	      GotoPos(0, curr->w_top);
	      curr->w_y = curr->w_top;
	      curr->w_x = 0;
	    }
	  else
	    {
	      GotoPos(0, 0);
	      curr->w_y = curr->w_x = 0;
	    }
	  break;
	case 7:
	  curr->w_wrap = i;
	  break;
	case 35:
	  debug1("Cursor %svisible\n", i ? "in" : "");
	  curr->w_cursor_invisible = i;
	  break;
	}
      break;
    }
}


static void
SetChar(c)
register int c;
{
  register struct win *p = curr;

  p->w_image[p->w_y][p->w_x] = c;
  p->w_attr[p->w_y][p->w_x] = p->w_Attr;
  p->w_font[p->w_y][p->w_x] = p->w_charsets[p->w_ss ? p->w_ss : p->w_Charset];
}

static void
StartString(type)
enum string_t type;
{
  curr->w_StringType = type;
  curr->w_stringp = curr->w_string;
  curr->w_state = ASTR;
}

static void
SaveChar(c)
int c;
{
  if (curr->w_stringp >= curr->w_string + MAXSTR - 1)
    curr->w_state = LIT;
  else
    *(curr->w_stringp)++ = c;
}

static void
PrintChar(c)
int c;
{
  if (curr->w_stringp >= curr->w_string + MAXSTR - 1)
    PrintFlush();
  *(curr->w_stringp)++ = c;
}

static void
PrintFlush()
{
  if (curr->w_stringp > curr->w_string)
    {
      PutStr(PO);
      AddStrn(curr->w_string, curr->w_stringp - curr->w_string);
      PutStr(PF);
      Flush();
      curr->w_stringp = curr->w_string;
    }
}


void
NewAutoFlow(win, on)
struct win *win;
int on;
{
  debug1("NewAutoFlow: %d\n", on);
  SetCurr(win);
  if (win->w_flow & FLOW_AUTOFLAG)
    win->w_flow = FLOW_AUTOFLAG | (FLOW_AUTO|FLOW_NOW) * on;
  else
    win->w_flow = (win->w_flow & ~FLOW_AUTO) | FLOW_AUTO * on;
  if (display)
    SetFlow(win->w_flow & FLOW_NOW);
}

static void
DesignateCharset(c, n)
int c, n;
{
  curr->w_ss = 0;
  if (c == 'B')
    c = ASCII;
  if (curr->w_charsets[n] != c)
    {
      curr->w_charsets[n] = c;
      if (curr->w_Charset == n)
	SetFont(c);
    }
}

static void
MapCharset(n)
int n;
{
  curr->w_ss = 0;
  if (curr->w_Charset != n)
    {
      curr->w_Charset = n;
      SetFont(curr->w_charsets[n]);
    }
}

static void
SaveCursor()
{
  curr->w_saved = 1;
  curr->w_Saved_x = curr->w_x;
  curr->w_Saved_y = curr->w_y;
  curr->w_SavedAttr = curr->w_Attr;
  curr->w_SavedCharset = curr->w_Charset;
  bcopy((char *) curr->w_charsets, (char *) curr->w_SavedCharsets,
	4 * sizeof(int));
}

static void
RestoreCursor()
{
  if (curr->w_saved)
    {
      GotoPos(curr->w_Saved_x, curr->w_Saved_y);
      curr->w_x = curr->w_Saved_x;
      curr->w_y = curr->w_Saved_y;
      curr->w_Attr = curr->w_SavedAttr;
      SetAttr(curr->w_Attr);
      bcopy((char *) curr->w_SavedCharsets, (char *) curr->w_charsets,
	    4 * sizeof(int));
      curr->w_Charset = curr->w_SavedCharset;
      SetFont(curr->w_charsets[curr->w_Charset]);
    }
}

static void
BackSpace()
{
  if (curr->w_x > 0)
    {
      curr->w_x--;
    }
  else if (curr->w_wrap && curr->w_y > 0)
    {
      curr->w_x = cols - 1;
      curr->w_y--;
    }
  if (display)
    GotoPos(curr->w_x, curr->w_y);
}

static void
Return()
{
  if (curr->w_x > 0)
    {
      curr->w_x = 0;
      if (display)
        GotoPos(curr->w_x, curr->w_y);
    }
}

static void
LineFeed(out_mode)
int out_mode;
{
  /* out_mode: 0=cr+lf no-output, 1=lf, 2=cr+lf */
  if (out_mode != 1)
    curr->w_x = 0;
  if (curr->w_y != curr->w_bot)		/* Don't scroll */
    {
      if (curr->w_y < rows-1)
	curr->w_y++;
      if (out_mode && display)
	GotoPos(curr->w_x, curr->w_y);
      return;
    }
  ScrollUpMap(1);
  if (curr->w_autoaka > 1)
    curr->w_autoaka--;
  if (out_mode && display)
    {
      ScrollRegion(curr->w_top, curr->w_bot, 1);
      GotoPos(curr->w_x, curr->w_y);
    }
}

static void
ReverseLineFeed()
{
  if (curr->w_y == curr->w_top)
    {
      ScrollDownMap(1);
      if (!display)
	return;
      ScrollRegion(curr->w_top, curr->w_bot, -1);
      GotoPos(curr->w_x, curr->w_y);
    }
  else if (curr->w_y > 0)
    CursorUp(1);
}

static void
InsertAChar(c)
int c;
{
  register int y = curr->w_y, x = curr->w_x;

  if (x == cols)
    x--;
  bcopy(curr->w_image[y], OldImage, cols);
  bcopy(curr->w_attr[y], OldAttr, cols);
  bcopy(curr->w_font[y], OldFont, cols);
  bcopy(curr->w_image[y] + x, curr->w_image[y] + x + 1, cols - x - 1);
  bcopy(curr->w_attr[y] + x, curr->w_attr[y] + x + 1, cols - x - 1);
  bcopy(curr->w_font[y] + x, curr->w_font[y] + x + 1, cols - x - 1);
  SetChar(c);
  if (!display)
    return;
  if (CIC || IC || IM)
    {
      InsertMode(curr->w_insert);
      INSERTCHAR(c);
      if (y == d_bot)
	d_lp_missing = 0;
    }
  else
    {
      ChangeLine(OldImage, OldAttr, OldFont, y, x, cols - 1);
      GotoPos(++x, y);
    }
}

static void
InsertChar(n)
int n;
{
  register int i, y = curr->w_y, x = curr->w_x;

  if (n <= 0)
    return;
  /* Hack to be compatible with the old screen versions */
  if (curr->w_insert)
    return;
  if (x == cols)
    --x;
  bcopy(curr->w_image[y], OldImage, cols);
  bcopy(curr->w_attr[y], OldAttr, cols);
  bcopy(curr->w_font[y], OldFont, cols);
  if (n > cols - x)
    n = cols - x;
  bcopy(curr->w_image[y] + x, curr->w_image[y] + x + n, cols - x - n);
  bcopy(curr->w_attr[y] + x, curr->w_attr[y] + x + n, cols - x - n);
  bcopy(curr->w_font[y] + x, curr->w_font[y] + x + n, cols - x - n);
  ClearInLine(y, x, x + n - 1);
  if (!display)
    return;
  if (IC || CIC || IM)
    {
      if (y == d_bot)
	d_lp_missing = 0;
      if (!d_insert)
	{
	  if (n == 1 && IC)
	    {
	      PutStr(IC);
	      return;
            }
	  if (CIC)
	    {
	      CPutStr(CIC, n);
	      return;
            }
	}
      InsertMode(1);
      for (i = n; i--; )
	INSERTCHAR(' ');
      GotoPos(x, y);
    }
  else
    {
      ChangeLine(OldImage, OldAttr, OldFont, y, x, cols - 1);
      GotoPos(x, y);
    }
}

static void
DeleteChar(n)
int n;
{
  register int i, y = curr->w_y, x = curr->w_x;

  if (x == cols)
    --x;
  bcopy(curr->w_image[y], OldImage, cols);
  bcopy(curr->w_attr[y], OldAttr, cols);
  bcopy(curr->w_font[y], OldFont, cols);
  if (n > cols - x)
    n = cols - x;
  bcopy(curr->w_image[y] + x + n, curr->w_image[y] + x, cols - x - n);
  bcopy(curr->w_attr[y] + x + n, curr->w_attr[y] + x, cols - x - n);
  bcopy(curr->w_font[y] + x + n, curr->w_font[y] + x, cols - x - n);
  ClearInLine(y, cols - n, cols - 1);
  if (!display)
    return;
  if (CDC && !(n == 1 && DC))
    {
      CPutStr(CDC, n);
      if (d_lp_missing && y == d_bot)
	{
	  FixLP(cols - 1 - n, y);
          GotoPos(x, y);
	}
    }
  else if (DC)
    {
      for (i = n; i; i--)
	PutStr(DC);
      if (d_lp_missing && y == d_bot)
	{
	  FixLP(cols - 1 - n, y);
          GotoPos(x, y);
	}
    }
  else
    {
      ChangeLine(OldImage, OldAttr, OldFont, y, x, cols - 1);
      GotoPos(x, y);
    }
}

static void
DeleteLine(n)
int n;
{
  register int old = curr->w_top;
  
  if (curr->w_y < curr->w_top || curr->w_y > curr->w_bot)
    return;
  if (n > curr->w_bot - curr->w_y + 1)
    n = curr->w_bot - curr->w_y + 1;
  curr->w_top = curr->w_y;
  ScrollUpMap(n);
  curr->w_top = old;
  if (!display)
    return;
  ScrollRegion(curr->w_y, curr->w_bot, n);
  GotoPos(curr->w_x, curr->w_y);
}

static void
InsertLine(n)
int n;
{
  register int old = curr->w_top;

  if (curr->w_y < curr->w_top || curr->w_y > curr->w_bot)
    return;
  if (n > curr->w_bot - curr->w_y + 1)
    n = curr->w_bot - curr->w_y + 1;
  curr->w_top = curr->w_y;
  ScrollDownMap(n);
  curr->w_top = old;
  if (!display)
    return;
  ScrollRegion(curr->w_y, curr->w_bot, -n);
  GotoPos(curr->w_x, curr->w_y);
}


static void
ScrollUpMap(n)
int n;
{
  char tmp[256 * sizeof(char *)];
  register int ii, i, cnt1, cnt2;
  register char **ppi, **ppa, **ppf;

  i = curr->w_top + n;
  cnt1 = n * sizeof(char *);
  cnt2 = (curr->w_bot - i + 1) * sizeof(char *);
  ppi = curr->w_image + i;
  ppa = curr->w_attr + i;
  ppf = curr->w_font + i;
#ifdef COPY_PASTE
  for(ii = curr->w_top; ii < i; ii++)
     AddLineToHist(curr, &curr->w_image[ii], &curr->w_attr[ii], &curr->w_font[ii]);
#endif
  for (i = n; i; --i)
    {
      bclear(*--ppi, cols + 1);
      bzero(*--ppa, cols + 1);
      bzero(*--ppf, cols + 1);
    }
  Scroll((char *) ppi, cnt1, cnt2, tmp);
  Scroll((char *) ppa, cnt1, cnt2, tmp);
  Scroll((char *) ppf, cnt1, cnt2, tmp);
}

static void
ScrollDownMap(n)
int n;
{
  char tmp[256 * sizeof(char *)];
  register int i, cnt1, cnt2;
  register char **ppi, **ppa, **ppf;

  i = curr->w_top;
  cnt1 = (curr->w_bot - i - n + 1) * sizeof(char *);
  cnt2 = n * sizeof(char *);
  Scroll((char *) (ppi = curr->w_image + i), cnt1, cnt2, tmp);
  Scroll((char *) (ppa = curr->w_attr + i), cnt1, cnt2, tmp);
  Scroll((char *) (ppf = curr->w_font + i), cnt1, cnt2, tmp);
  for (i = n; i; --i)
    {
      bclear(*ppi++, cols + 1);
      bzero(*ppa++, cols + 1);
      bzero(*ppf++, cols + 1);
    }
}

static void
Scroll(cp, cnt1, cnt2, tmp)
char *cp, *tmp;
int cnt1, cnt2;
{
  if (!cnt1 || !cnt2)
    return;
  if (cnt1 <= cnt2)
    {
      bcopy(cp, tmp, cnt1);
      bcopy(cp + cnt1, cp, cnt2);
      bcopy(tmp, cp + cnt2, cnt1);
    }
  else
    {
      bcopy(cp + cnt1, tmp, cnt2);
      bcopy(cp, cp + cnt2, cnt1);
      bcopy(tmp, cp, cnt2);
    }
}

static void
ForwardTab()
{
  register int x = curr->w_x;

  if (x == cols)
    {
      LineFeed(2);
      x = 0;
    }
  if (curr->w_tabs[x] && x < cols - 1)
    x++;
  while (x < cols - 1 && !curr->w_tabs[x])
    x++;
  GotoPos(x, curr->w_y);
  curr->w_x = x;
}

static void
BackwardTab()
{
  register int x = curr->w_x;

  if (curr->w_tabs[x] && x > 0)
    x--;
  while (x > 0 && !curr->w_tabs[x])
    x--;
  GotoPos(x, curr->w_y);
  curr->w_x = x;
}

static void
ClearScreen()
{
  register int i;
  register char **ppi = curr->w_image, **ppa = curr->w_attr, **ppf = curr->w_font;

  for (i = 0; i < rows; ++i)
    {
#ifdef COPY_PASTE
      AddLineToHist(curr, ppi, ppa, ppf);
#endif
      bclear(*ppi++, cols + 1);
      bzero(*ppa++, cols + 1);
      bzero(*ppf++, cols + 1);
    }
  if (display)
    ClearDisplay();
}

static void
ClearFromBOS()
{
  register int n, y = curr->w_y, x = curr->w_x;

  if (display)
    Clear(0, 0, x, y);
  for (n = 0; n < y; ++n)
    ClearInLine(n, 0, cols - 1);
  ClearInLine(y, 0, x);
  RestorePosAttrFont();
}

static void
ClearToEOS()
{
  register int n, y = curr->w_y, x = curr->w_x;

  if (x == 0 && y == 0)
    {
      ClearScreen();
      return;
    }
  if (display)
    Clear(x, y, d_width - 1, d_height - 1);
  ClearInLine(y, x, cols - 1);
  for (n = y + 1; n < rows; n++)
    ClearInLine(n, 0, cols - 1);
  RestorePosAttrFont();
}

static void
ClearFullLine()
{
  register int y = curr->w_y;

  if (display)
    Clear(0, y, d_width - 1, y);
  ClearInLine(y, 0, cols - 1);
  RestorePosAttrFont();
}

static void
ClearToEOL()
{
  register int y = curr->w_y, x = curr->w_x;

  if (display)
    Clear(x, y, d_width - 1, y);
  ClearInLine(y, x, cols - 1);
  RestorePosAttrFont();
}

static void
ClearFromBOL()
{
  register int y = curr->w_y, x = curr->w_x;

  if (display)
    Clear(0, y, x, y);
  ClearInLine(y, 0, x);
  RestorePosAttrFont();
}

static void
ClearInLine(y, x1, x2)
int y, x1, x2;
{
  register int n;

  if (x1 == cols)
    x1--;
  if (x2 == cols - 1)
    x2++;
  if ((n = x2 - x1 + 1) != 0)
    {
      bclear(curr->w_image[y] + x1, n);
      bzero(curr->w_attr[y] + x1, n);
      bzero(curr->w_font[y] + x1, n);
    }
}

static void
CursorRight(n)
register int n;
{
  register int x = curr->w_x;

  if (x == cols)
    {
      LineFeed(2);
      x = 0;
    }
  if ((curr->w_x += n) >= cols)
    curr->w_x = cols - 1;
  GotoPos(curr->w_x, curr->w_y);
}

static void
CursorUp(n)
register int n;
{
  if (curr->w_y < curr->w_top)		/* if above scrolling rgn, */
    {
      if ((curr->w_y -= n) < 0)		/* ignore its limits      */
         curr->w_y = 0;
    }
  else
    if ((curr->w_y -= n) < curr->w_top)
      curr->w_y = curr->w_top;
  GotoPos(curr->w_x, curr->w_y);
}

static void
CursorDown(n)
register int n;
{
  if (curr->w_y > curr->w_bot)		/* if below scrolling rgn, */
    {
      if ((curr->w_y += n) > rows - 1)	/* ignore its limits      */
        curr->w_y = rows - 1;
    }
  else
    if ((curr->w_y += n) > curr->w_bot)
      curr->w_y = curr->w_bot;
  GotoPos(curr->w_x, curr->w_y);
}

static void
CursorLeft(n)
register int n;
{
  if ((curr->w_x -= n) < 0)
    curr->w_x = 0;
  GotoPos(curr->w_x, curr->w_y);
}

static void
ASetMode(on)
int on;
{
  register int i;

  for (i = 0; i < curr->w_NumArgs; ++i)
    {
      switch (curr->w_args[i])
	{
	case 4:
	  curr->w_insert = on;
	  InsertMode(on);
	  break;
	}
    }
}

static void
SelectRendition()
{
  register int i = 0, a = curr->w_Attr;

  do
    {
      switch (curr->w_args[i])
	{
	case 0:
	  a = 0;
	  break;
	case 1:
	  a |= A_BD;
	  break;
	case 2:
	  a |= A_DI;
	  break;
	case 3:
	  a |= A_SO;
	  break;
	case 4:
	  a |= A_US;
	  break;
	case 5:
	  a |= A_BL;
	  break;
	case 7:
	  a |= A_RV;
	  break;
	case 22:
	  a &= ~(A_BD | A_SO | A_DI);
	  break;
	case 23:
	  a &= ~A_SO;
	  break;
	case 24:
	  a &= ~A_US;
	  break;
	case 25:
	  a &= ~A_BL;
	  break;
	case 27:
	  a &= ~A_RV;
	  break;
	}
    } while (++i < curr->w_NumArgs);
  SetAttr(curr->w_Attr = a);
}

static void
FillWithEs()
{
  register int i;
  register char *p, *ep;

  curr->w_y = curr->w_x = 0;
  for (i = 0; i < rows; ++i)
    {
      bzero(curr->w_attr[i], cols);
      bzero(curr->w_font[i], cols);
      p = curr->w_image[i];
      ep = p + cols;
      while (p < ep)
	*p++ = 'E';
    }
  if (display)
    Redisplay(0);
}


static void
ChangeLine(os, oa, of, y, from, to)
int from, to, y;
char *os, *oa, *of;
{
  ASSERT(display);
  DisplayLine(os, oa, of, curr->w_image[y], curr->w_attr[y],
	      curr->w_font[y], y, from, to);
  RestorePosAttrFont();
}

void
CheckLP(n_ch)
char n_ch;
{
  register int y, x;
  register char n_at, n_fo, o_ch, o_at, o_fo;

  ASSERT(display);
  x = cols - 1;
  y = d_bot;
  o_ch = curr->w_image[y][x];
  o_at = curr->w_attr[y][x];
  o_fo = curr->w_font[y][x];

  n_at = curr->w_Attr;
  n_fo = curr->w_charsets[curr->w_Charset];

  d_lp_image = n_ch;
  d_lp_attr = n_at;
  d_lp_font = n_fo;
  d_lp_missing = 0;
  if (n_ch == o_ch && n_at == o_at && n_fo == o_fo)
    return;
  if (n_ch != ' ' || n_at || n_fo)
    d_lp_missing = 1;
  if (o_ch != ' ' || o_at || o_fo)
    {
      if (DC)
	PutStr(DC);
      else if (CDC)
	CPutStr(CDC, 1);
      else if (CE)
	PutStr(CE);
      else
	d_lp_missing = 1;
    }
}

static void
FindAKA()
{
  register char *cp, *line;
  register struct win *wp = curr;
  register int len = strlen(wp->w_cmd);
  int y;

  y = (wp->w_autoaka > 0 && wp->w_autoaka <= wp->w_height) ? wp->w_autoaka - 1 : wp->w_y;
  cols = wp->w_width;
 try_line:
  cp = line = wp->w_image[y];
  if (wp->w_autoaka > 0 &&  *wp->w_cmd != '\0')
    {
      for (;;)
	{
	  if (cp - line >= cols - len)
	    {
	      if (++y == wp->w_autoaka && y < rows)
		goto try_line;
	      return;
	    }
	  if (strncmp(cp, wp->w_cmd, len) == 0)
	    break;
	  cp++;
	}
      cp += len;
    }
  for (len = cols - (cp - line); len && *cp == ' '; len--, cp++)
    ;
  if (len)
    {
      if (wp->w_autoaka > 0 && (*cp == '!' || *cp == '%' || *cp == '^'))
	wp->w_autoaka = -1;
      else
	wp->w_autoaka = 0;
      line = wp->w_cmd + wp->w_akapos;
      while (len && *cp != ' ')
	{
	  if ((*line++ = *cp++) == '/')
	    line = wp->w_cmd + wp->w_akapos;
	  len--;
	}
      *line = '\0';
    }
  else
    wp->w_autoaka = 0;
}

void
MakeBlankLine(p, n)
register char *p;
register int n;
{
  while (n--)
    *p++ = ' ';
}

void
SetCurr(wp)
struct win *wp;
{
  curr = wp;
  if (curr == 0)
    return;
  cols = curr->w_width;
  rows = curr->w_height;
  display = curr->w_active ? curr->w_display : 0;
}

static void
RestorePosAttrFont()
{
  GotoPos(curr->w_x, curr->w_y);
  SetAttr(curr->w_Attr);
  SetFont(curr->w_charsets[curr->w_Charset]);
}

/* Send a terminal report as if it were typed. */ 
static void
Report(fmt, n1, n2)
char *fmt;
int n1, n2;
{
  register int len;
  char rbuf[40];

  sprintf(rbuf, fmt, n1, n2);
  len = strlen(rbuf);

  if ((unsigned)(curr->w_inlen + len) <= sizeof(curr->w_inbuf))
    {
      bcopy(rbuf, curr->w_inbuf + curr->w_inlen, len);
      curr->w_inlen += len;
    }
}

#ifdef COPY_PASTE
void
AddLineToHist(wp, pi, pa, pf)
struct win *wp;
char **pi, **pa, **pf;
{
  register char *q;

  if (wp->w_histheight == 0)
    return;
  q = *pi; *pi = wp->w_ihist[wp->w_histidx]; wp->w_ihist[wp->w_histidx] = q;
  q = *pa; *pa = wp->w_ahist[wp->w_histidx]; wp->w_ahist[wp->w_histidx] = q;
  q = *pf; *pf = wp->w_fhist[wp->w_histidx]; wp->w_fhist[wp->w_histidx] = q;
  if (++wp->w_histidx >= wp->w_histheight)
    wp->w_histidx = 0;
}
#endif
