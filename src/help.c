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
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <sys/types.h>

#include "config.h"

#include "screen.h"
#include "extern.h"

char version[40];      /* initialised by main() */

extern struct display *display;
extern char *noargs[];


void
exit_with_usage(myname)
char *myname;
{
  printf("Use: %s [-opts] [cmd [args]]\n", myname);
  printf(" or: %s -r [host.tty]\n\nOptions:\n", myname);
  printf("-a           Force all capabilities into each window's termcap.\n");
  printf("-A -[r|R]    Adapt all windows to the new display width & height.\n");
  printf("-c file      Read configuration file instead of '.screenrc'.\n");
#ifdef REMOTE_DETACH
  printf("-d (-r)      Detach the elsewhere running screen (and reattach here).\n");
  printf("-D (-r)      Detach and logout remote (and reattach here).\n");
#endif
  printf("-e xy        Change command characters.\n");
  printf("-f           Flow control on, -fn = off, -fa = auto.\n");
  printf("-h lines     Set the size of the scrollback history buffer.\n");
  printf("-i           Interrupt output sooner when flow control is on.\n");
#if defined(LOGOUTOK) && defined(UTMPOK)
  printf("-l           Login mode on (update %s), -ln = off.\n", UTMPFILE);
#endif
  printf("-list        or -ls. Do nothing, just list our SockDir.\n");
  printf("-L           Terminal's last character can be safely updated.\n");
  printf("-m           ignore $STY variable, do create a new screen session.\n");
  printf("-O           Choose optimal output rather than exact vt100 emulation.\n");
  printf("-q           Quiet startup. Exits with non-zero return code if unsuccessful.\n");
  printf("-r           Reattach to a detached screen process.\n");
  printf("-R           Reattach if possible, otherwise start a new session.\n");
  printf("-s shell     Shell to execute rather than $SHELL.\n");
  printf("-S sockname  Name this session <pid>.sockname instead of <pid>.<tty>.<host>.\n");
  printf("-t title     Set title. (window's name).\n");
  printf("-T term      Use term as $TERM for windows, rather than \"screen\".\n");
  printf("-v           Print \"Screen version %s\".\n", version);
  printf("-wipe        Do nothing, just clean up SockDir.\n");
#ifdef MULTI
  printf("-x           Attach to a not detached screen. (Multi display mode).\n");
#endif /* MULTI */
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
static void HelpSetCursor __P((void));
static void add_key_to_buf __P((char *, int));
static int  AddAction __P((struct action *, int));
static int  helppage __P((void));

struct helpdata
{
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
  HelpSetCursor,
  DefResize,
  DefRestore
};


void
display_help()
{
  int i, n, key, mcom, mkey, l;
  struct helpdata *helpdata;
  int used[RC_LAST + 1];

  if (D_height < 6)
    {
      Msg(0, "Window height too small for help page");
      return;
    }
  if (InitOverlayPage(sizeof(*helpdata), &HelpLf, 0))
    return;

  helpdata = (struct helpdata *)D_lay->l_data;
  helpdata->num_names = helpdata->command_bindings = 0;
  helpdata->command_search = 0;
  for (n = 0; n <= RC_LAST; n++)
    used[n] = 0;
  mcom = 0;
  mkey = 0;
  for (key = 0; key < 256; key++)
    {
      n = ktab[key].nr;
      if (n == RC_ILLEGAL)
	continue;
      if (ktab[key].args == noargs)
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
  helpdata->numcols = (D_width - !D_CLP)/(mcom + mkey + 1);
  if (helpdata->numcols == 0)
    {
      HelpAbort();
      Msg(0, "Width too small");
      return;
    }
  helpdata->inter = (D_width - !D_CLP - (mcom + mkey) * helpdata->numcols) / (helpdata->numcols + 1);
  if (helpdata->inter <= 0)
    helpdata->inter = 1;
  debug1("inter: %d\n", helpdata->inter);
  helpdata->mcom = mcom;
  helpdata->mkey = mkey;
  helpdata->numrows = (helpdata->num_names + helpdata->numcols - 1) / helpdata->numcols;
  debug1("Numrows: %d\n", helpdata->numrows);
  helpdata->numskip = D_height-5 - (2 + helpdata->numrows);
  while (helpdata->numskip < 0)
    helpdata->numskip += D_height-5;
  helpdata->numskip %= D_height-5;
  debug1("Numskip: %d\n", helpdata->numskip);
  if (helpdata->numskip > D_height/3 || helpdata->numskip > helpdata->command_bindings)
    helpdata->numskip = 1;
  helpdata->maxrow = 2 + helpdata->numrows + helpdata->numskip + helpdata->command_bindings;
  helpdata->grow = 0;

  helpdata->numpages = (helpdata->maxrow + D_height-6) / (D_height-5);
  helppage();
}

static void
HelpSetCursor()
{
  GotoPos(0, D_height - 1);
}

static void
HelpProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;

  GotoPos(0, D_height-1);
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
  LAY_CALL_UP(Activate(0));
  ExitOverlayPage();
}


static int
helppage()
{
  struct helpdata *helpdata;
  int col, crow, n, key;
  char buf[MAXKLEN], Esc_buf[5], cbuf[256];

  helpdata = (struct helpdata *)D_lay->l_data;

  if (helpdata->grow >= helpdata->maxrow)
    { 
      return(-1);
    }
  helpdata->refgrow = helpdata->grow;
  helpdata->refcommand_search = helpdata->command_search;

  /* Clear the help screen */
  SetAttrFont(0, ASCII);
  ClearDisplay();
  
  sprintf(cbuf,"Screen key bindings, page %d of %d.", helpdata->grow / (D_height-5) + 1, helpdata->numpages);
  centerline(cbuf);
  AddChar('\n');
  crow = 2;

  *Esc_buf = '\0';
  add_key_to_buf(Esc_buf, D_user->u_Esc);

  for (; crow < D_height - 3; crow++)
    {
      if (helpdata->grow < 1)
        {
   	  *buf = '\0';
          add_key_to_buf(buf, D_user->u_MetaEsc);
          sprintf(cbuf,"Command key:  %s   Literal %s:  %s", Esc_buf, Esc_buf, buf);
          centerline(cbuf);
	  helpdata->grow++;
        }
      else if (helpdata->grow >= 2 && helpdata->grow-2 < helpdata->numrows)
	{
	  for (col = 0; col < helpdata->numcols && (n = helpdata->numrows * col + (helpdata->grow-2)) < helpdata->num_names; col++)
	    {
	      AddStrn("", helpdata->inter - !col);
	      n = helpdata->nact[n];
	      debug1("help: searching key %d\n", n);
	      buf[0] = '\0';
	      for (key = 0; key < 256; key++)
		if (ktab[key].nr == n && ktab[key].args == noargs)
		  {
		    strcat(buf, " ");
		    add_key_to_buf(buf, key);
		  }
	      AddStrn(comms[n].name, helpdata->mcom);
	      AddStrn(buf, helpdata->mkey);
	    }
	  AddStr("\r\n");
          helpdata->grow++;
        }
      else if (helpdata->grow-2-helpdata->numrows >= helpdata->numskip 
               && helpdata->grow-2-helpdata->numrows-helpdata->numskip < helpdata->command_bindings)
        {
	  while ((n = ktab[helpdata->command_search].nr) == RC_ILLEGAL
		 || ktab[helpdata->command_search].args == noargs)
	    {
	      if (++helpdata->command_search >= 256)
		return -1;
	    }
	  buf[0] = '\0';
	  add_key_to_buf(buf, helpdata->command_search);
	  AddStrn(buf, 4);
	  AddAction(&ktab[helpdata->command_search++], D_width - 5);
	  AddStr("\r\n");
	  helpdata->grow++;
	}
      else
	{
          AddChar('\n');
	  helpdata->grow++;
	}
    }
  AddChar('\n');
  sprintf(cbuf,"[Press Space %s Return to end.]",
	 helpdata->grow < helpdata->maxrow ? "for next page;" : "or");
  centerline(cbuf);
  SetLastPos(0, D_height-1);
  return(0);
}

static int
AddAction(act, fr)
struct action *act;
int fr;
{
  char buf[256];
  int del, l;
  char *bp, *cp, **pp;

  if (fr <= 0)
    return 0;
  l = strlen(comms[act->nr].name);
  
  if (l + 1 > fr)
    l = fr - 1;
  AddStrn(comms[act->nr].name, l);
  fr -= l + 1;
  AddChar(fr ? ' ' : '$');

  pp = act->args;
  while (pp && (cp = *pp) != NULL)
    {
      del = 0;
      bp = buf;
      if (!*cp || (index(cp, ' ') != NULL))
	{
	  if (index(cp, '\'') != NULL)
	    *bp++ = del = '"';
	  else
	    *bp++ = del = '\'';
	}
      while (*cp && bp < buf + 250)
        bp += AddXChar(bp, *cp++);
      if (del)
	*bp++ = del;
      *bp = 0;
      if ((fr -= (bp - buf) + 1) < 0)
	{
	  fr += bp - buf;
	  if (fr > 0)
	    AddStrn(buf, fr);
	  if (fr == 0)
	    AddChar('$');
	  return 0;
	}
      AddStr(buf);
      pp++;
      if (*pp)
        AddChar(fr ? ' ' : '$');
    }
  return fr;
}

static void
add_key_to_buf(buf, key)
char *buf;
int key;
{
  debug1("help: key found: %c\n", key);
  buf += strlen(buf);
  if (key == ' ')
    sprintf(buf, "sp");
  else buf[AddXChar(buf, key)] = 0;
}


static void
HelpRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  if (y < 0)
    {
      struct helpdata *helpdata;

      helpdata = (struct helpdata *)D_lay->l_data;
      helpdata->grow = helpdata->refgrow;
      helpdata->command_search = helpdata->refcommand_search;
      helppage();
      return;
    }
  if (y != 0 && y != D_height - 1)
    return;
  if (isblank)
    return;
  Clear(xs, y, xs, xe, xe, y, 0);
}


/*
**
**    here is all the copyright stuff 
**
*/

static void CopyrightProcess __P((char **, int *));
static void CopyrightRedisplayLine __P((int, int, int, int));
static void CopyrightAbort __P((void));
static void CopyrightSetCursor __P((void));
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
  CopyrightSetCursor,
  DefResize,
  DefRestore
};

static const char cpmsg[] = "\
\n\
Screen version %v\n\
\n\
Copyright (c) 1993 Juergen Weigert, Michael Schroeder\n\
Copyright (c) 1987 Oliver Laumann\n\
\n\
This program is free software; you can redistribute it and/or \
modify it under the terms of the GNU General Public License as published \
by the Free Software Foundation; either version 2, or (at your option) \
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, \
but WITHOUT ANY WARRANTY; without even the implied warranty of \
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the \
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License \
along with this program (see the file COPYING); if not, write to the \
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\
\n\
Send bugreports, fixes, enhancements, t-shirts, money, beer & pizza to \
screen@uni-erlangen.de\n";


static void
CopyrightSetCursor()
{
  GotoPos(0, D_height - 1);
}

static void
CopyrightProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;
  struct copydata *copydata;

  copydata = (struct copydata *)D_lay->l_data;
  GotoPos(0, D_height - 1);
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
  LAY_CALL_UP(Activate(0));
  ExitOverlayPage();
}

void
display_copyright()
{
  struct copydata *copydata;

  if (D_width < 10 || D_height < 5)
    {
      Msg(0, "Window size too small for copyright page");
      return;
    }
  if (InitOverlayPage(sizeof(*copydata), &CopyrightLf, 0))
    return;
  copydata = (struct copydata *)D_lay->l_data;
  copydata->cps = (char *)cpmsg;
  copydata->savedcps = 0;
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

  copydata = (struct copydata *)D_lay->l_data;
  SetAttrFont(0, ASCII);
  ClearDisplay();
  x = y = 0;
  cps = copydata->cps;
  copydata->refcps = cps;
  copydata->refsavedcps = copydata->savedcps;
  while (*cps && y < D_height - 3)
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
      if (l > D_width - 1)
	l = D_width - 1;
      if (x && x + l >= D_width - 2)
	{
	  AddStr("\r\n");
	  x = 0;
	  y++;
	  continue;
	}
      if (x)
	{
	  AddChar(' ');
	  x++;
	}
      if (l)
        AddStrn(ws, l);
      x += l;
      cps += l;
      if (*cps == 0 && copydata->savedcps)
	{
	  cps = copydata->savedcps;
	  copydata->savedcps = 0;
	}
      if (*cps == '\n')
	{
	  AddStr("\r\n");
	  x = 0;
	  y++;
	}
      if (*cps == ' ' || *cps == '\n')
	cps++;
    }
  while (*cps == '\n')
    cps++;
  while (y++ < D_height - 2)
    AddStr("\r\n");
  sprintf(cbuf,"[Press Space %s Return to end.]",
	 *cps ? "for next page;" : "or");
  centerline(cbuf);
  SetLastPos(0, D_height-1);
  copydata->cps = cps;
}

static void
CopyrightRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  if (y < 0)
    {
      struct copydata *copydata;

      copydata = (struct copydata *)D_lay->l_data;
      copydata->cps = copydata->refcps;
      copydata->savedcps = copydata->refsavedcps;
      copypage();
      return;
    }
  if (y != 0 && y != D_height - 1)
    return;
  if (isblank)
    return;
  Clear(xs, y, xs, xe, xe, y, 0);
}

void 
display_displays()
{
  /* To be filled in... */
}

/*
**
**    The bindkey help page
**
*/

#ifdef MAPKEYS

extern char *kmap_extras[];
extern int kmap_extras_fl[];
extern struct term term[];

static void BindkeyProcess __P((char **, int *));
static void BindkeyAbort __P((void));
static void BindkeyRedisplayLine __P((int, int, int, int));
static void BindkeySetCursor __P((void));
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
  BindkeySetCursor,
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

  if (display == 0)
    return;
  if (D_height < 6)
    {
      Msg(0, "Window height too small for bindkey page");
      return;
    }
  if (InitOverlayPage(sizeof(*bindkeydata), &BindkeyLf, 0))
    return;

  bindkeydata = (struct bindkeydata *)D_lay->l_data;
  bindkeydata->title = title;
  bindkeydata->tab = tab;
  
  n = 0;
  for (i = 0; i < KMAP_KEYS+KMAP_AKEYS+KMAP_EXT; i++)
    {
      if (tab[i].nr != RC_ILLEGAL)
	n++;
    }
  bindkeydata->pos = 0;
  bindkeydata->page = 1;
  bindkeydata->pages = (n + D_height - 6) / (D_height - 5);
  if (bindkeydata->pages == 0)
    bindkeydata->pages = 1;
  bindkeypage();
}

static void
BindkeySetCursor()
{
  GotoPos(0, D_height - 1);
}

static void
BindkeyAbort()
{
  LAY_CALL_UP(Activate(0));
  ExitOverlayPage();
}

static void
bindkeypage()
{
  struct bindkeydata *bindkeydata;
  char tbuf[256];
  int del, i, ch, y;
  struct action *act;
  char *xch, *s, *p;

  bindkeydata = (struct bindkeydata *)D_lay->l_data;

  SetAttrFont(0, ASCII);
  ClearDisplay();

  sprintf(tbuf, "%s key bindings, page %d of %d.", bindkeydata->title, bindkeydata->page, bindkeydata->pages);
  centerline(tbuf);
  AddChar('\n');
  y = D_height - 5;
  for (i = bindkeydata->pos; i < KMAP_KEYS+KMAP_AKEYS+KMAP_EXT && y; i++)
    {
      p = tbuf;
      act = &bindkeydata->tab[i];
      if (act->nr == RC_ILLEGAL)
	continue;
      xch = "   ";
      if (i < KMAP_KEYS)
	{
	  del = *p++ = ':';
	  s = term[i + T_CAPS].tcname;
	}
      else if (i < KMAP_KEYS+KMAP_AKEYS)
	{
	  del = *p++ = ':';
	  s = term[i + (T_CAPS - T_OCAPS + T_CURSOR)].tcname;
	  xch = "[A]";
	}
      else
	{
	  del = 0;
	  s = kmap_extras[i - (KMAP_KEYS+KMAP_AKEYS)];
	  if (kmap_extras_fl[i - (KMAP_KEYS+KMAP_AKEYS)])
	    xch = "[T]";
	}
      while ((ch = *(unsigned char *)s++))
	p += AddXChar(p, ch);
      if (del)
	*p++ = del;
      *p++ = ' ';
      while (p < tbuf + 15)
        *p++ = ' ';
      sprintf(p, "%s -> ", xch);
      p += 7;
      if (p - tbuf > D_width - 1)
	{
	  tbuf[D_width - 2] = '$';
	  tbuf[D_width - 1] = 0;
	}
      AddStr(tbuf);
      AddAction(act, D_width - 1 - strlen(tbuf));
      AddStr("\r\n");
      y--;
    }
  y++;
  while(y--)
    AddChar('\n');
  bindkeydata->last = i;
  sprintf(tbuf,"[Press Space %s Return to end.]", bindkeydata->page < bindkeydata->pages ? "for next page;" : "or");
  centerline(tbuf);
  SetLastPos(0, D_height-1);
}
 
static void
BindkeyProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int done = 0;
  struct bindkeydata *bindkeydata;

  bindkeydata = (struct bindkeydata *)D_lay->l_data;
  GotoPos(0, D_height-1);
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
  if (y != 0 && y != D_height - 1)
    return;
  if (isblank)
    return;
  Clear(xs, y, xs, xe, xe, y, 0);
}

#endif
