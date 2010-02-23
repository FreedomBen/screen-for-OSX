/* Copyright (c) 2008, 2009
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 *      Micah Cowan (micah@cowan.name)
 *      Sadrul Habib Chowdhury (sadrul@users.sourceforge.net)
 * Copyright (c) 1993-2002, 2003, 2005, 2006, 2007
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, see
 * http://www.gnu.org/licenses/, or contact Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 ****************************************************************
 */

#include <sys/types.h>

#include "config.h"

#include "screen.h"
#include "extern.h"

#include "list_generic.h"

char version[60];      /* initialised by main() */

extern struct layer *flayer;
extern struct display *display, *displays;
extern struct win *windows;
extern int maxwin;
extern char *noargs[];
extern struct mchar mchar_blank, mchar_so;
extern int renditions[];
extern unsigned char *blank;
extern struct win **wtab;
#ifdef MAPKEYS
extern struct term term[];
#endif

extern struct LayFuncs ListLf;

static void PadStr __P((char *, int, int, int));

extern char *wliststr;
extern char *wlisttit;

void
exit_with_usage(myname, message, arg)
char *myname, *message, *arg;
{
  printf("Use: %s [-opts] [cmd [args]]\n", myname);
  printf(" or: %s -r [host.tty]\n\nOptions:\n", myname);
  printf("-a            Force all capabilities into each window's termcap.\n");
  printf("-A -[r|R]     Adapt all windows to the new display width & height.\n");
  printf("-c file       Read configuration file instead of '.screenrc'.\n");
#ifdef REMOTE_DETACH
  printf("-d (-r)       Detach the elsewhere running screen (and reattach here).\n");
  printf("-dmS name     Start as daemon: Screen session in detached mode.\n");
  printf("-D (-r)       Detach and logout remote (and reattach here).\n");
  printf("-D -RR        Do whatever is needed to get a screen session.\n");
#endif
  printf("-e xy         Change command characters.\n");
  printf("-f            Flow control on, -fn = off, -fa = auto.\n");
  printf("-h lines      Set the size of the scrollback history buffer.\n");
  printf("-i            Interrupt output sooner when flow control is on.\n");
#if defined(LOGOUTOK) && defined(UTMPOK)
  printf("-l            Login mode on (update %s), -ln = off.\n", UTMPFILE);
#endif
  printf("-ls [match]   or -list. Do nothing, just list our SockDir [on possible matches].\n");
  printf("-L            Turn on output logging.\n");
  printf("-m            ignore $STY variable, do create a new screen session.\n");
  printf("-O            Choose optimal output rather than exact vt100 emulation.\n");
  printf("-p window     Preselect the named window if it exists.\n");
  printf("-q            Quiet startup. Exits with non-zero return code if unsuccessful.\n");
  printf("-r [session]  Reattach to a detached screen process.\n");
  printf("-R            Reattach if possible, otherwise start a new session.\n");
  printf("-s shell      Shell to execute rather than $SHELL.\n");
  printf("-S sockname   Name this session <pid>.sockname instead of <pid>.<tty>.<host>.\n");
  printf("-t title      Set title. (window's name).\n");
  printf("-T term       Use term as $TERM for windows, rather than \"screen\".\n");
#ifdef UTF8
  printf("-U            Tell screen to use UTF-8 encoding.\n");
#endif
  printf("-v            Print \"Screen version %s\".\n", version);
  printf("-wipe [match] Do nothing, just clean up SockDir [on possible matches].\n");
#ifdef MULTI
  printf("-x            Attach to a not detached screen. (Multi display mode).\n");
#endif /* MULTI */
  printf("-X            Execute <cmd> as a screen command in the specified session.\n");
  if (message && *message)
    {
      printf("\nError: ");
      printf(message, arg);
      printf("\n");
    }
  exit(1);
}

/*
**   Here come the help page routines
*/

extern struct comm comms[];
extern struct action ktab[];

static void HelpProcess __P((char **, int *));
static void HelpAbort __P((void));
static void HelpRedisplayLine __P((int, int, int, int));
static void add_key_to_buf __P((char *, int));
static void AddAction __P((struct action *, int, int));
static int  helppage __P((void));

struct helpdata
{
  char *class;
  struct action *ktabp;
  int	maxrow, grow, numcols, numrows, num_names;
  int	numskip, numpages;
  int	command_search, command_bindings;
  int   refgrow, refcommand_search;
  int   inter, mcom, mkey;
  int   nact[RC_LAST + 1];
};

#define MAXKLEN 256

static struct LayFuncs HelpLf =
{
  HelpProcess,
  HelpAbort,
  HelpRedisplayLine,
  DefClearLine,
  DefRewrite,
  DefResize,
  DefRestore
};


void
display_help(class, ktabp)
char *class;
struct action *ktabp;
{
  int i, n, key, mcom, mkey, l;
  struct helpdata *helpdata;
  int used[RC_LAST + 1];

  if (flayer->l_height < 6)
    {
      LMsg(0, "Window height too small for help page");
      return;
    }
  if (InitOverlayPage(sizeof(*helpdata), &HelpLf, 0))
    return;

  helpdata = (struct helpdata *)flayer->l_data;
  helpdata->class = class;
  helpdata->ktabp = ktabp;
  helpdata->num_names = helpdata->command_bindings = 0;
  helpdata->command_search = 0;
  for (n = 0; n <= RC_LAST; n++)
    used[n] = 0;
  mcom = 0;
  mkey = 0;
  for (key = 0; key < 256 + KMAP_KEYS; key++)
    {
      n = ktabp[key].nr;
      if (n == RC_ILLEGAL)
	continue;
      if (ktabp[key].args == noargs)
	{
	  used[n] += (key <= ' ' || key == 0x7f) ? 3 :
		     (key > 0x7f) ? 5 : 2;
	}
      else
	helpdata->command_bindings++;
    }
  for (n = i = 0; n <= RC_LAST; n++)
    if (used[n])
      {
	l = strlen(comms[n].name);
	if (l > mcom)
	  mcom = l;
	if (used[n] > mkey)
	  mkey = used[n];
	helpdata->nact[i++] = n;
      }
  debug1("help: %d commands bound to keys with no arguments\n", i);
  debug2("mcom: %d  mkey: %d\n", mcom, mkey);
  helpdata->num_names = i;

  if (mkey > MAXKLEN)
    mkey = MAXKLEN;
  helpdata->numcols = flayer->l_width / (mcom + mkey + 1);
  if (helpdata->numcols == 0)
    {
      HelpAbort();
      LMsg(0, "Width too small");
      return;
    }
  helpdata->inter = (flayer->l_width - (mcom + mkey) * helpdata->numcols) / (helpdata->numcols + 1);
  if (helpdata->inter <= 0)
    helpdata->inter = 1;
  debug1("inter: %d\n", helpdata->inter);
  helpdata->mcom = mcom;
  helpdata->mkey = mkey;
  helpdata->numrows = (helpdata->num_names + helpdata->numcols - 1) / helpdata->numcols;
  debug1("Numrows: %d\n", helpdata->numrows);
  helpdata->numskip = flayer->l_height-5 - (2 + helpdata->numrows);
  while (helpdata->numskip < 0)
    helpdata->numskip += flayer->l_height-5;
  helpdata->numskip %= flayer->l_height-5;
  debug1("Numskip: %d\n", helpdata->numskip);
  if (helpdata->numskip > flayer->l_height/3 || helpdata->numskip > helpdata->command_bindings)
    helpdata->numskip = 1;
  helpdata->maxrow = 2 + helpdata->numrows + helpdata->numskip + helpdata->command_bindings;
  helpdata->grow = 0;

  helpdata->numpages = (helpdata->maxrow + flayer->l_height-6) / (flayer->l_height-5);
  flayer->l_x = 0;
  flayer->l_y = flayer->l_height - 1;
  helppage();
}

static void
HelpProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;

  while (!done && *plen > 0)
    {
      switch (**ppbuf)
	{
	case ' ':
	  if (helppage() == 0)
	    break;
	  /* FALLTHROUGH */
	case '\r':
	case '\n':
	  done = 1;
	  break;
	default:
	  break;
	}
      ++*ppbuf;
      --*plen;
    }
  if (done)
    HelpAbort();
}

static void
HelpAbort()
{
  LAY_CALL_UP(LRefreshAll(flayer, 0));
  ExitOverlayPage();
}


static int
helppage()
{
  struct helpdata *helpdata;
  int col, crow, n, key, x;
  char buf[MAXKLEN], Esc_buf[5], cbuf[256];
  struct action *ktabp;

  helpdata = (struct helpdata *)flayer->l_data;

  ktabp = helpdata->ktabp;
  if (helpdata->grow >= helpdata->maxrow)
    return -1;
  helpdata->refgrow = helpdata->grow;
  helpdata->refcommand_search = helpdata->command_search;

  /* Clear the help screen */
  LClearAll(flayer, 0);
  
  sprintf(cbuf,"Screen key bindings, page %d of %d.", helpdata->grow / (flayer->l_height-5) + 1, helpdata->numpages);
  centerline(cbuf, 0);
  crow = 2;

  *Esc_buf = '\0';
  *buf = '\0';
  /* XXX fix escape character */
  if (flayer->l_cvlist && flayer->l_cvlist->c_display)
    {
      add_key_to_buf(buf, flayer->l_cvlist->c_display->d_user->u_MetaEsc);
      add_key_to_buf(Esc_buf, flayer->l_cvlist->c_display->d_user->u_Esc);
    }
  else
    {
      strcpy(Esc_buf, "??");
      strcpy(buf, "??");
    }

  for (; crow < flayer->l_height - 3; crow++)
    {
      if (helpdata->grow < 1)
	{
	  if (ktabp == ktab)
	    sprintf(cbuf,"Command key:  %s   Literal %s:  %s", Esc_buf, Esc_buf, buf);
	  else
	    sprintf(cbuf,"Command class: '%.80s'", helpdata->class);
	  centerline(cbuf, crow);
	  helpdata->grow++;
	}
      else if (helpdata->grow >= 2 && helpdata->grow-2 < helpdata->numrows)
	{
	  x = 0;
	  for (col = 0; col < helpdata->numcols && (n = helpdata->numrows * col + (helpdata->grow-2)) < helpdata->num_names; col++)
	    {
	      x += helpdata->inter - !col;
	      n = helpdata->nact[n];
	      buf[0] = '\0';
	      for (key = 0; key < 256 + KMAP_KEYS; key++)
		if (ktabp[key].nr == n && ktabp[key].args == noargs && strlen(buf) < sizeof(buf) - 7)
		  {
		    strcat(buf, " ");
		    add_key_to_buf(buf, key);
		  }
	      PadStr(comms[n].name, helpdata->mcom, x, crow);
	      x += helpdata->mcom;
	      PadStr(buf, helpdata->mkey, x, crow);
	      x += helpdata->mkey;
	    }
	  helpdata->grow++;
	}
      else if (helpdata->grow-2-helpdata->numrows >= helpdata->numskip 
	       && helpdata->grow-2-helpdata->numrows-helpdata->numskip < helpdata->command_bindings)
        {
	  while ((n = ktabp[helpdata->command_search].nr) == RC_ILLEGAL
		 || ktabp[helpdata->command_search].args == noargs)
	    {
	      if (++helpdata->command_search >= 256 + KMAP_KEYS)
		return -1;
	    }
	  buf[0] = '\0';
	  add_key_to_buf(buf, helpdata->command_search);
	  PadStr(buf, 5, 0, crow);
	  AddAction(&ktabp[helpdata->command_search++], 5, crow);
	  helpdata->grow++;
	}
      else
	helpdata->grow++;
    }
  sprintf(cbuf,"[Press Space %s Return to end.]",
	 helpdata->grow < helpdata->maxrow ? "for next page;" : "or");
  centerline(cbuf, flayer->l_height - 2);
  LaySetCursor();
  return 0;
}

static void
AddAction(act, x, y)
struct action *act;
int x, y;
{
  char buf[256];
  int del, l;
  char *bp, *cp, **pp;
  int *lp, ll;
  int fr;
  struct mchar mchar_dol;

  mchar_dol = mchar_blank;
  mchar_dol.image = '$';

  fr = flayer->l_width - 1 - x;
  if (fr <= 0)
    return;
  l = strlen(comms[act->nr].name);
  
  if (l + 1 > fr)
    l = fr - 1;
  PadStr(comms[act->nr].name, l, x, y);
  x += l;
  fr -= l + 1;
  LPutChar(flayer, fr ? &mchar_blank : &mchar_dol, x++, y);

  pp = act->args;
  lp = act->argl;
  while (pp && (cp = *pp) != NULL)
    {
      del = 0;
      bp = buf;
      ll = *lp++;
      if (!ll || (index(cp, ' ') != NULL))
	{
	  if (index(cp, '\'') != NULL)
	    *bp++ = del = '"';
	  else
	    *bp++ = del = '\'';
	}
      while (ll-- && bp < buf + 250)
	bp += AddXChar(bp, *(unsigned char *)cp++);
      if (del)
	*bp++ = del;
      *bp = 0;
      if ((fr -= (bp - buf) + 1) < 0)
	{
	  fr += bp - buf;
	  if (fr > 0)
	    PadStr(buf, fr, x, y);
	  if (fr == 0)
	    LPutChar(flayer, &mchar_dol, x, y);
	  return;
	}
      PadStr(buf, strlen(buf), x, y);
      x += strlen(buf);
      pp++;
      if (*pp)
	LPutChar(flayer, fr ? &mchar_blank : &mchar_dol, x++, y);
    }
}

static void
add_key_to_buf(buf, key)
char *buf;
int key;
{
  buf += strlen(buf);
  if (key < 0)
    strcpy(buf, "unset");
  else if (key == ' ')
    strcpy(buf, "sp");
#ifdef MAPKEYS
  else if (key >= 256)
    {
      key = key - 256 + T_CAPS;
      buf[0] = ':';
      buf[1] = term[key].tcname[0];
      buf[2] = term[key].tcname[1];
      buf[3] = ':';
      buf[4] = 0;
    }
#endif
  else
    buf[AddXChar(buf, key)] = 0;
}


static void
HelpRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  if (y < 0)
    {
      struct helpdata *helpdata;

      helpdata = (struct helpdata *)flayer->l_data;
      helpdata->grow = helpdata->refgrow;
      helpdata->command_search = helpdata->refcommand_search;
      helppage();
      return;
    }
  if (y != 0 && y != flayer->l_height - 1)
    return;
  if (!isblank)
    LClearArea(flayer, xs, y, xe, y, 0, 0);
}


/*
**
**    here is all the copyright stuff 
**
*/

static void CopyrightProcess __P((char **, int *));
static void CopyrightRedisplayLine __P((int, int, int, int));
static void CopyrightAbort __P((void));
static void copypage __P((void));

struct copydata
{
  char	*cps, *savedcps;	/* position in the message */
  char	*refcps, *refsavedcps;	/* backup for redisplaying */
};

static struct LayFuncs CopyrightLf =
{
  CopyrightProcess,
  CopyrightAbort,
  CopyrightRedisplayLine,
  DefClearLine,
  DefRewrite,
  DefResize,
  DefRestore
};

static const char cpmsg[] = "\
\n\
Screen version %v\n\
\n\
Copyright (c) 2008, 2009 Juergen Weigert, Michael Schroeder, Micah Cowan, Sadrul Habib Chowdhury\n\
Copyright (c) 1993-2002, 2003, 2005, 2006, 2007 Juergen Weigert, Michael Schroeder\n\
Copyright (c) 1987 Oliver Laumann\n\
\n\
This program is free software; you can redistribute it and/or \
modify it under the terms of the GNU General Public License as published \
by the Free Software Foundation; either version 3, or (at your option) \
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License \
along with this program (see the file COPYING); if not, see \
http://www.gnu.org/licenses/, or contact Free Software Foundation, Inc., \
51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA.\n\
\n\
Send bugreports, fixes, enhancements, t-shirts, money, beer & pizza to \
screen@uni-erlangen.de\n";


static void
CopyrightProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;
  struct copydata *copydata;

  copydata = (struct copydata *)flayer->l_data;
  while (!done && *plen > 0)
    {
      switch (**ppbuf)
	{
	case ' ':
	  if (*copydata->cps)
	    {
	      copypage();
	      break;
	    }
	  /* FALLTHROUGH */
	case '\r':
	case '\n':
	  CopyrightAbort();
	  done = 1;
	  break;
	default:
	  break;
	}
      ++*ppbuf;
      --*plen;
    }
}

static void
CopyrightAbort()
{
  LAY_CALL_UP(LRefreshAll(flayer, 0));
  ExitOverlayPage();
}

void
display_copyright()
{
  struct copydata *copydata;

  if (flayer->l_width < 10 || flayer->l_height < 5)
    {
      LMsg(0, "Window size too small for copyright page");
      return;
    }
  if (InitOverlayPage(sizeof(*copydata), &CopyrightLf, 0))
    return;
  copydata = (struct copydata *)flayer->l_data;
  copydata->cps = (char *)cpmsg;
  copydata->savedcps = 0;
  flayer->l_x = 0;
  flayer->l_y = flayer->l_height - 1;
  copypage();
}

static void
copypage()
{
  register char *cps;
  char *ws;
  int x, y, l;
  char cbuf[80];
  struct copydata *copydata;

  ASSERT(flayer);
  copydata = (struct copydata *)flayer->l_data;

  LClearAll(flayer, 0);
  x = y = 0;
  cps = copydata->cps;
  copydata->refcps = cps;
  copydata->refsavedcps = copydata->savedcps;
  while (*cps && y < flayer->l_height - 3)
    {
      ws = cps;
      while (*cps == ' ')
	cps++;
      if (strncmp(cps, "%v", 2) == 0)
	{
	  copydata->savedcps = cps + 2;
	  cps = version;
	  continue;
	}
      while (*cps && *cps != ' ' && *cps != '\n')
	cps++;
      l = cps - ws;
      cps = ws;
      if (l > flayer->l_width - 1)
	l = flayer->l_width - 1;
      if (x && x + l >= flayer->l_width - 2)
	{
	  x = 0;
	  y++;
	  continue;
	}
      if (x)
	{
	  LPutChar(flayer, &mchar_blank, x, y);
	  x++;
	}
      if (l)
	LPutStr(flayer, ws, l, &mchar_blank, x, y);
      x += l;
      cps += l;
      if (*cps == 0 && copydata->savedcps)
	{
	  cps = copydata->savedcps;
	  copydata->savedcps = 0;
	}
      if (*cps == '\n')
	{
	  x = 0;
	  y++;
	}
      if (*cps == ' ' || *cps == '\n')
	cps++;
    }
  while (*cps == '\n')
    cps++;
  sprintf(cbuf,"[Press Space %s Return to end.]",
	 *cps ? "for next page;" : "or");
  centerline(cbuf, flayer->l_height - 2);
  copydata->cps = cps;
  LaySetCursor();
}

static void
CopyrightRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  ASSERT(flayer);
  if (y < 0)
    {
      struct copydata *copydata;

      copydata = (struct copydata *)flayer->l_data;
      copydata->cps = copydata->refcps;
      copydata->savedcps = copydata->refsavedcps;
      copypage();
      return;
    }
  if (y != 0 && y != flayer->l_height - 1)
    return;
  if (isblank)
    return;
  LClearArea(flayer, xs, y, xe, y, 0, 0);
}


/*
**
**    here is the windowlist
**
*/

struct wlistdata;

static void WListProcess __P((char **, int *));
static void WListAbort __P((void));
static void WListRedisplayLine __P((int, int, int, int));
static void wlistpage __P((void));
static void WListLine __P((int, int, int, int));
static void WListLines __P((int, int));
static void WListMove __P((int, int));
static void WListUpdate __P((struct win *));
static int  WListNormalize __P((void));
static int  WListResize __P((int, int));
static int  WListNext __P((struct wlistdata *, int, int));

struct wlistdata {
  int pos;
  int ypos;
  int npos;
  int numwin;
  int first;
  int last;
  int start;
  int order;
  struct win *group;
  int nested;
  int *list;
};

#if 0
static struct LayFuncs WListLf =
{
  WListProcess,
  WListAbort,
  WListRedisplayLine,
  DefClearLine,
  DefRewrite,
  WListResize,
  DefRestore
};
#endif

#define WTAB_GROUP_MATCHES(i) (group == wtab[i]->w_group)

#if 0
static int
WListResize(wi, he)
int wi, he;
{
  struct wlistdata *wlistdata;
  if (wi < 10 || he < 5)
    return -1;
  wlistdata = (struct wlistdata *)flayer->l_data;
  flayer->l_width = wi;
  flayer->l_height = he;
  wlistdata->numwin = he - 3;
  if (wlistdata->ypos >= wlistdata->numwin)
    wlistdata->ypos = wlistdata->numwin - 1;
  flayer->l_y = he - 1;
  return 0;
}

static void
WListAbort()
{
  struct wlistdata * wlistdata = (struct wlistdata *)flayer->l_data;
  if (wlistdata->list)
    Free(wlistdata->list);
  LAY_CALL_UP(LRefreshAll(flayer, 0));
  ExitOverlayPage();
}

static void
WListProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;
  struct wlistdata *wlistdata;
  struct display *olddisplay = display;
  int h;
  struct win *group;

  ASSERT(flayer);
  wlistdata = (struct wlistdata *)flayer->l_data;
  group = wlistdata->group;
  h = wlistdata->numwin;
  while (!done && *plen > 0)
    {
      if ((unsigned char)**ppbuf >= '0' && (unsigned char)**ppbuf <= '9')
	{
	  int n = (unsigned char)**ppbuf - '0';
	  int d = 0;
	  if (n < maxwin && wtab[n] && WTAB_GROUP_MATCHES(n))
	    {
	      int i;
	      for (d = -wlistdata->npos, i = WListNext(wlistdata, -1, 0); i != n; i = WListNext(wlistdata, i, 1), d++)
		;
	    }
	  if (d)
	    WListMove(d, -1);
	}
      switch ((unsigned char)**ppbuf)
	{
	case 0220:	/* up */
	case 16:	/* ^P like emacs */
	case 'k':
	  WListMove(-1, -1);
	  break;
	case 0216:	/* down */
	case 14:	/* ^N like emacs */
	case 'j':
	  WListMove(1, -1);
	  break;
	case '\025':
	  WListMove(-(h / 2), wlistdata->ypos);
	  break;
	case '\004':
	  WListMove(h / 2, wlistdata->ypos);
	  break;
	case 0002:
	case 'b':
	  WListMove(-h, -1);
	  break;
	case 0006:
	case 'f':
	  WListMove(h, -1);
	  break;
	case 0201:	/* home */
	  WListMove(-wlistdata->pos, -1);
	  break;
	case 0205:	/* end */
	  WListMove(maxwin, -1);
	  break;
	case 'a':
	  /* All-window view */
	  wlistdata->group = 0;
	  wlistdata->nested = WLIST_NESTED;
	  wlistpage();
	  break;
	case 'g':
	  /* Toggle nested view */
	  wlistdata->nested ^= WLIST_NESTED;
	  wlistpage();
	  break;
	case 'm':
	  /* Toggle MRU view */
	  wlistdata->order ^= 1;
	  wlistpage();
	  break;
	case '\r':
	case '\n':
	case ' ':
	  h = wlistdata->pos;
	  if (h == maxwin && Layer2Window(flayer) && Layer2Window(flayer)->w_type == W_TYPE_GROUP)
	    break;
	  if (display && h != maxwin && wtab[h] && (wtab[h]->w_type == W_TYPE_GROUP) && Layer2Window(flayer) == wtab[h])
	    {
	      wlistdata->group = wtab[h];
	      wlistdata->pos = wtab[h]->w_number;
	      wlistpage();
	      break;
	    }
	  done = 1;
	  if (!display || h == maxwin || !wtab[h] || wtab[h] == D_fore || (flayer->l_cvlist && flayer->l_cvlist->c_lnext))
	    WListAbort();
#ifdef MULTIUSER
	  else if (AclCheckPermWin(D_user, ACL_READ, wtab[h]))
	    WListAbort();
#endif
	  else
	    {
	      if (wlistdata->list)
		Free(wlistdata->list);
	      ExitOverlayPage();	/* no need to redisplay */
	    }
	  /* restore display, don't switch wrong user */
	  display = olddisplay;
	  if (h != maxwin)
	    SwitchWindow(h);
	  break;
	case 0222: /* Mouse event */
	  break;
	case 0033:
	case 0007:
	  h = wlistdata->start;
	  if (h == -1 && Layer2Window(flayer) && Layer2Window(flayer)->w_type == W_TYPE_GROUP)
	    {
	      struct win *p = Layer2Window(flayer);
	      if (wlistdata->group != p)
		{
		  wlistdata->group = p;
		  wlistpage();
		}
	      break;
	    }
	  WListAbort();
	  display = olddisplay;
	  if (h >= 0 && wtab[h])
	    SwitchWindow(h);
	  else if (h == -2)
	    {
	      struct win *p = FindNiceWindow(display ? D_other : (struct win *)0, 0);
	      if (p)
		SwitchWindow(p->w_number);
	    }
	  done = 1;
	  break;
	case '\010':   /* ctrl-h */
	case 0177:
	  if (!wlistdata->group)
	    break;
	  wlistdata->pos = wlistdata->group->w_number;
	  wlistdata->group = wlistdata->group->w_group;
	  if (wlistdata->group)
	    wlistdata->pos = wlistdata->group->w_number;
	  wlistpage();
	  break;
	default:
	  break;
	}
      ++*ppbuf;
      --*plen;
    }
}

static void
WListLine(y, i, pos, isblank)
int y, i;
int pos;
int isblank;
{
  char *str;
  int n;
  int yoff, xoff = 0;
  struct wlistdata *wlistdata;
  struct win *group;
  struct mchar mchar_rend = mchar_blank;
  struct mchar *mchar = (struct mchar *)0;

  if (i == maxwin)
    return;
  wlistdata = (struct wlistdata *)flayer->l_data;
  if (wlistdata->nested && wtab[i])
    for (group = wtab[i]->w_group, xoff = 0; group != wlistdata->group;
	  group = group->w_group, xoff += 2)
      ;
  yoff = wlistdata->group ? 3 : 2;
  display = Layer2Window(flayer) ? 0 : flayer->l_cvlist ? flayer->l_cvlist->c_display : 0;
  str = MakeWinMsgEv(wliststr, wtab[i], '%', flayer->l_width - xoff, (struct event *)0, 0);
  n = strlen(str);
  if (i != pos && isblank)
    while (n && str[n - 1] == ' ')
      n--;
  if (i == pos)
    mchar = &mchar_so;
  else if (wtab[i]->w_monitor == MON_DONE && renditions[REND_MONITOR] != -1)
    {
      mchar = &mchar_rend;
      ApplyAttrColor(renditions[REND_MONITOR], mchar);
    }
  else if ((wtab[i]->w_bell == BELL_DONE || wtab[i]->w_bell == BELL_FOUND) && renditions[REND_BELL] != -1)
    {
      mchar = &mchar_rend;
      ApplyAttrColor(renditions[REND_BELL], mchar);
    }
  else
    mchar = &mchar_blank;
  LPutWinMsg(flayer, str, (i == pos || !isblank) ? flayer->l_width : n, mchar, xoff, y + yoff);
  if (xoff)
    LPutWinMsg(flayer, "", xoff, mchar, 0, y + yoff);
  if (mchar == &mchar_so)
    flayer->l_y = y + yoff;
#if 0
  LPutStr(flayer, str, n, i == pos ? &mchar_so : &mchar_blank, 0, y + yoff);
  if (i == pos || !isblank)
    while(n < flayer->l_width)
      LPutChar(flayer, i == pos ? &mchar_so : &mchar_blank, n++, y + yoff);
#endif
  return;
}


static int
WListNext(wlistdata, old, delta)
struct wlistdata *wlistdata;
int old, delta;
{
  int i;

  if (old == maxwin)
    return maxwin;
  if (old == -1)
    old = 0;
  else
    {
      for (i = 0; i < maxwin && wlistdata->list[i] != -1; i++)
	if (wlistdata->list[i] == old)
	  break;
      if (i < maxwin && wlistdata->list[i] != -1)
	old = i;
    }

  old += delta;
  if (old < 0 || old >= maxwin || wlistdata->list[old] == -1)
    old -= delta;
  return wlistdata->list[old];
}

static void
WListLines(up, oldpos)
int up, oldpos;
{
  struct wlistdata *wlistdata;
  int ypos, pos;
  int y, i, oldi;

  wlistdata = (struct wlistdata *)flayer->l_data;
  ypos = wlistdata->ypos;
  pos = wlistdata->pos;

  i = WListNext(wlistdata, pos, -ypos);
  for (y = 0; y < wlistdata->numwin; y++)
    {
      if (i == maxwin || !wtab[i])
	return;
      if (y == 0)
	wlistdata->first = i;
      wlistdata->last = i;
      if (((i == oldpos || i == pos) && pos != oldpos) || (up > 0 && y >= wlistdata->numwin - up) || (up < 0 && y < -up))
	WListLine(y, i, pos, i != oldpos);
      if (i == pos)
	wlistdata->ypos = y;
      oldi = i;
      i = WListNext(wlistdata, i, 1);
      if (i == maxwin || i == oldi)
	break;
    }
}

static int
WListNormalize()
{
  struct wlistdata *wlistdata;
  int i, oldi, n;
  int ypos, pos;

  wlistdata = (struct wlistdata *)flayer->l_data;
  ypos = wlistdata->ypos;
  pos = wlistdata->pos;
  if (ypos < 0)
    ypos = 0;
  if (ypos >= wlistdata->numwin)
    ypos = wlistdata->numwin - 1;
  for (n = 0, oldi = maxwin, i = pos; i != maxwin && i != oldi && n < wlistdata->numwin; oldi = i, i = WListNext(wlistdata, i, 1))
    n++;
  if (ypos < wlistdata->numwin - n)
    ypos = wlistdata->numwin - n;
  for (n = 0, oldi = maxwin, i = WListNext(wlistdata, -1, 0); i != maxwin && i != oldi && i != pos; oldi = i, i = WListNext(wlistdata, i, 1))
    n++;
  if (ypos > n)
    ypos = n;
  wlistdata->ypos = ypos;
  wlistdata->npos = n;
  return ypos;
}

static void
WListMove(num, ypos)
int num;
int ypos;
{
  struct wlistdata *wlistdata;
  int oldpos, oldypos, oldnpos;
  int pos, up;

  wlistdata = (struct wlistdata *)flayer->l_data;
  oldpos = wlistdata->pos;
  oldypos = wlistdata->ypos;
  oldnpos = wlistdata->npos;
  wlistdata->ypos = ypos == -1 ? oldypos + num : ypos;
  pos = WListNext(wlistdata, oldpos, num);
  wlistdata->pos = pos;
  ypos = WListNormalize();
  up = wlistdata->npos - ypos - (oldnpos - oldypos);
  if (up)
    {
      LScrollV(flayer, up, 2, 2 + wlistdata->numwin - 1, 0);
      WListLines(up, oldpos);
      LaySetCursor();
      return;
    }
  if (pos == oldpos)
    return;
  WListLine(oldypos, oldpos, pos, 0);
  WListLine(ypos, pos, pos, 1);
  LaySetCursor();
}

static void
WListRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  ASSERT(flayer);
  if (y < 0)
    {
      wlistpage();
      return;
    }
  if (y != 0 && y != flayer->l_height - 1)
    return;
  if (!isblank)
    LClearArea(flayer, xs, y, xe, y, 0, 0);
}

static int
WListOrder(wlistdata, ind, start, group)
struct wlistdata *wlistdata;
int ind, start;
struct win *group;
{
  int i;

  if (ind >= maxwin)
    return ind;
  if (ind == 0)
    for (i = 0; i < maxwin; i++)
      wlistdata->list[i] = -1;

  if (wlistdata->order == WLIST_MRU)
    {
      if (start == -1)
	start = windows->w_number;
    }
  else
    {
      if (start == -1)
	start = 0;
      while (start < maxwin && !wtab[start])
	start++;
    }

  if (start >= maxwin || !wtab[start])
    return ind;

  if (!WTAB_GROUP_MATCHES(start))
    {
      while (start < maxwin && (!wtab[start] || !WTAB_GROUP_MATCHES(start)))
	if (wlistdata->order != WLIST_MRU)
	  start++;
	else if (wtab[start]->w_next)
	  start = wtab[start]->w_next->w_number;
	else
	  start = maxwin;
      if (start >= maxwin || !wtab[start])
	return ind;
    }

  wlistdata->list[ind++] = start;
  if (wlistdata->nested && wtab[start]->w_type == W_TYPE_GROUP)
    ind = WListOrder(wlistdata, ind, -1, wtab[start]);

  if (wlistdata->order != WLIST_MRU)
    start++;
  else if (wtab[start]->w_next)
    start = wtab[start]->w_next->w_number;
  else
    return ind;
  return WListOrder(wlistdata, ind, start, group);
}
#endif

void
display_wlist(onblank, order, group)
int onblank;
int order;
struct win *group;
{
  display_windows(onblank, order, group);
  return;
#if 0
  struct win *p;
  struct wlistdata *wlistdata;


  if (flayer->l_width < 10 || flayer->l_height < 6)
    {
      LMsg(0, "Window size too small for window list page");
      return;
    }
  if (onblank)
    {
      debug3("flayer %x %d %x\n", flayer, flayer->l_width, flayer->l_height);
      if (!display)
	{
	  LMsg(0, "windowlist -b: display required");
	  return;
	}
      p = D_fore;
      if (p)
	{
	  SetForeWindow((struct win *)0);
          if (p->w_group)
	    {
	      D_fore = p->w_group;
	      flayer->l_data = (char *)p->w_group;
	    }
	  Activate(0);
	}
      if (flayer->l_width < 10 || flayer->l_height < 6)
	{
	  LMsg(0, "Window size too small for window list page");
	  return;
	}
    }
  else
    p = Layer2Window(flayer);
  if (!group && p)
    group = p->w_group;
  if (InitOverlayPage(sizeof(*wlistdata), &WListLf, 0))
    return;
  wlistdata = (struct wlistdata *)flayer->l_data;
  wlistdata->list = calloc(maxwin, sizeof(int));
  flayer->l_mode = 1;
  flayer->l_x = 0;
  flayer->l_y = flayer->l_height - 1;
  wlistdata->start = onblank && p ? p->w_number : -1;
  wlistdata->order = (order & 0x1);
  wlistdata->group = group;
  wlistdata->pos = p ? p->w_number : WListNext(wlistdata, -1, 0);
  wlistdata->ypos = wlistdata->npos = 0;
  wlistdata->numwin = flayer->l_height - (group ? 4 : 3);
  wlistdata->nested = (order & WLIST_NESTED);
  wlistpage();
#endif
}

#if 0
static void
wlistpage()
{
  struct wlistdata *wlistdata;
  char *str;
  int pos;
  struct win *group;

  wlistdata = (struct wlistdata *)flayer->l_data;
  group = wlistdata->group;

  LClearAll(flayer, 0);
  if (wlistdata->start >= 0 && wtab[wlistdata->start] == 0)
    wlistdata->start = -2;

  WListOrder(wlistdata, 0, -1, group);
  pos = wlistdata->pos;
  if (pos == maxwin || !wtab[pos] || !WTAB_GROUP_MATCHES(pos))
    {
      if (wlistdata->order == WLIST_MRU)
        pos = WListNext(wlistdata, -1, wlistdata->npos);
      else
	{
          /* find new position */
	  if (pos < maxwin)
	    while(++pos < maxwin)
	      if (wtab[pos] && WTAB_GROUP_MATCHES(pos))
	        break;
	  if (pos == maxwin)
	    while (--pos >= 0)
	      if (wtab[pos] && WTAB_GROUP_MATCHES(pos))
		break;
	  if (pos == -1)
	    pos = maxwin;
	}
    }
  wlistdata->pos = pos;

  display = 0;
  str = MakeWinMsgEv(wlisttit, (struct win *)0, '%', flayer->l_width, (struct event *)0, 0);
  if (wlistdata->group)
    {
      LPutWinMsg(flayer, "Group: ", 7, &mchar_blank, 0, 0);
      LPutWinMsg(flayer, wlistdata->group->w_title, strlen(wlistdata->group->w_title), &mchar_blank, 7, 0);
      LPutWinMsg(flayer, str, strlen(str), &mchar_blank, 0, 1);
    }
  else
    {
      LPutWinMsg(flayer, str, strlen(str), &mchar_blank, 0, 0);
    }
  WListNormalize();
  WListLines(wlistdata->numwin, -1);
  LaySetCursor();
}

static void
WListUpdate(p)
struct win *p;
{
  struct wlistdata *wlistdata;
  int i, n, y;

  if (p == 0)
    {
      wlistpage();
      return;
    }
  wlistdata = (struct wlistdata *)flayer->l_data;
  n = p->w_number;
  if (wlistdata->order == WLIST_NUM && (n < wlistdata->first || n > wlistdata->last))
    return;
  i = wlistdata->first;
  for (y = 0; y < wlistdata->numwin; y++)
    {
      if (i == n)
	break;
      i = WListNext(wlistdata, i, 1);
    }
  if (y == wlistdata->numwin)
    return;
  WListLine(y, i, wlistdata->pos, 0);
  LaySetCursor();
}
#endif

/*
**
**    The bindkey help page
**
*/

#ifdef MAPKEYS
extern struct kmap_ext *kmap_exts;
extern int kmap_extn;
extern struct action dmtab[];
extern struct action mmtab[];


static void BindkeyProcess __P((char **, int *));
static void BindkeyAbort __P((void));
static void BindkeyRedisplayLine __P((int, int, int, int));
static void bindkeypage __P((void));

struct bindkeydata
{
  char *title;
  struct action *tab;
  int pos;
  int last;
  int page;
  int pages;
};

static struct LayFuncs BindkeyLf =
{
  BindkeyProcess,
  BindkeyAbort,
  BindkeyRedisplayLine,
  DefClearLine,
  DefRewrite,
  DefResize,
  DefRestore
};


void
display_bindkey(title, tab)
char *title;
struct action *tab;
{
  struct bindkeydata *bindkeydata;
  int i, n;

  if (flayer->l_height < 6)
    {
      LMsg(0, "Window height too small for bindkey page");
      return;
    }
  if (InitOverlayPage(sizeof(*bindkeydata), &BindkeyLf, 0))
    return;

  bindkeydata = (struct bindkeydata *)flayer->l_data;
  bindkeydata->title = title;
  bindkeydata->tab = tab;
  
  n = 0;
  for (i = 0; i < KMAP_KEYS+KMAP_AKEYS+kmap_extn; i++)
    {
      if (tab[i].nr != RC_ILLEGAL)
	n++;
    }
  bindkeydata->pos = 0;
  bindkeydata->page = 1;
  bindkeydata->pages = (n + flayer->l_height - 6) / (flayer->l_height - 5);
  if (bindkeydata->pages == 0)
    bindkeydata->pages = 1;
  flayer->l_x = 0;
  flayer->l_y = flayer->l_height - 1;
  bindkeypage();
}

static void
BindkeyAbort()
{
  LAY_CALL_UP(LRefreshAll(flayer, 0));
  ExitOverlayPage();
}

static void
bindkeypage()
{
  struct bindkeydata *bindkeydata;
  struct kmap_ext *kme;
  char tbuf[256];
  int del, i, y, sl;
  struct action *act;
  char *xch, *s, *p;

  bindkeydata = (struct bindkeydata *)flayer->l_data;

  LClearAll(flayer, 0);

  sprintf(tbuf, "%s key bindings, page %d of %d.", bindkeydata->title, bindkeydata->page, bindkeydata->pages);
  centerline(tbuf, 0);
  y = 2;
  for (i = bindkeydata->pos; i < KMAP_KEYS+KMAP_AKEYS+kmap_extn && y < flayer->l_height - 3; i++)
    {
      p = tbuf;
      xch = "   ";
      if (i < KMAP_KEYS)
	{
	  act = &bindkeydata->tab[i];
	  if (act->nr == RC_ILLEGAL)
	    continue;
	  del = *p++ = ':';
	  s = term[i + T_CAPS].tcname;
	  sl = s ? strlen(s) : 0;
	}
      else if (i < KMAP_KEYS+KMAP_AKEYS)
	{
	  act = &bindkeydata->tab[i];
	  if (act->nr == RC_ILLEGAL)
	    continue;
	  del = *p++ = ':';
	  s = term[i + (T_CAPS - T_OCAPS + T_CURSOR)].tcname;
	  sl = s ? strlen(s) : 0;
	  xch = "[A]";
	}
      else
	{
	  kme = kmap_exts + (i - (KMAP_KEYS+KMAP_AKEYS));
	  del = 0;
	  s = kme->str;
	  sl = kme->fl & ~KMAP_NOTIMEOUT;
	  if ((kme->fl & KMAP_NOTIMEOUT) != 0)
	    xch = "[T]";
	  act = bindkeydata->tab == dmtab ? &kme->dm : bindkeydata->tab == mmtab ? &kme->mm : &kme->um;
	  if (act->nr == RC_ILLEGAL)
	    continue;
	}
      while (sl-- > 0)
	p += AddXChar(p, *(unsigned char *)s++);
      if (del)
	*p++ = del;
      *p++ = ' ';
      while (p < tbuf + 15)
	*p++ = ' ';
      sprintf(p, "%s -> ", xch);
      p += 7;
      if (p - tbuf > flayer->l_width - 1)
	{
	  tbuf[flayer->l_width - 2] = '$';
	  tbuf[flayer->l_width - 1] = 0;
	}
      PadStr(tbuf, strlen(tbuf), 0, y);
      AddAction(act, strlen(tbuf), y);
      y++;
    }
  y++;
  bindkeydata->last = i;
  sprintf(tbuf,"[Press Space %s Return to end.]", bindkeydata->page < bindkeydata->pages ? "for next page;" : "or");
  centerline(tbuf, flayer->l_height - 2);
  LaySetCursor();
}
 
static void
BindkeyProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;
  struct bindkeydata *bindkeydata;

  bindkeydata = (struct bindkeydata *)flayer->l_data;
  while (!done && *plen > 0)
    {
      switch (**ppbuf)
	{
	case ' ':
	  if (bindkeydata->page < bindkeydata->pages)
	    {
	      bindkeydata->pos = bindkeydata->last;
	      bindkeydata->page++;
	      bindkeypage();
	      break;
	    }
	  /* FALLTHROUGH */
	case '\r':
	case '\n':
	  done = 1;
	  break;
	default:
	  break;
	}
      ++*ppbuf;
      --*plen;
    }
  if (done)
    BindkeyAbort();
}

static void
BindkeyRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  if (y < 0)
    {
      bindkeypage();
      return;
    }
  if (y != 0 && y != flayer->l_height - 1)
    return;
  if (!isblank)
    LClearArea(flayer, xs, y, xe, y, 0, 0);
}

#endif /* MAPKEYS */


/*
**
**    The zmodem active page
**
*/

#ifdef ZMODEM

static void ZmodemRedisplayLine __P((int, int, int, int));
static int  ZmodemResize __P((int, int));

static struct LayFuncs ZmodemLf =
{
  DefProcess,
  0,
  ZmodemRedisplayLine,
  DefClearLine,
  DefRewrite,
  ZmodemResize,
  DefRestore
};

/*ARGSUSED*/
static int
ZmodemResize(wi, he)
int wi, he; 
{
  flayer->l_width = wi;
  flayer->l_height = he;
  flayer->l_x = flayer->l_width > 32 ? 32 : 0;
  return 0;
}

static void
ZmodemRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  DefRedisplayLine(y, xs, xe, isblank);
  if (y == 0 && xs == 0)
    LPutStr(flayer, "Zmodem active on another display", flayer->l_width > 32 ? 32 : flayer->l_width, &mchar_blank, 0, 0);
}

void
ZmodemPage()
{
  if (InitOverlayPage(1, &ZmodemLf, 1))
    return;
  LRefreshAll(flayer, 0);
  flayer->l_x = flayer->l_width > 32 ? 32 : 0;
  flayer->l_y = 0;
}

#endif



static void
PadStr(str, n, x, y)
char *str;
int n, x, y;
{
  int l;

  l = strlen(str);
  if (l > n)
    l = n;
  LPutStr(flayer, str, l, &mchar_blank, x, y);
  if (l < n)
    LPutStr(flayer, (char *)blank, n - l, &mchar_blank, x + l, y);
}
