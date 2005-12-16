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

static void InpProcess __P((char **, int *));
static void InpAbort __P((void));
static void InpRedisplayLine __P((int, int, int, int));
static void InpSetCursor __P((void));

extern struct display *display;

struct inpdata
{
  char	inpbuf[101];
  int	inplen;
  int	inpmaxlen;
  char	*inpstring;
  int	inpstringlen;
  int	inpmode;
  void	(*inpfinfunc)();
};

static struct LayFuncs InpLf =
{
  InpProcess,
  InpAbort,
  InpRedisplayLine,
  DefClearLine,
  DefRewrite,
  InpSetCursor,
  DefResize,
  DefRestore
};

/*
**   Here is the input routine
*/

void
inp_setprompt(p, s)
char *p, *s;
{
  struct inpdata *inpdata;
  
  inpdata = (struct inpdata *)D_lay->l_data;
  if (p)
    {
      inpdata->inpstringlen = strlen(p);
      inpdata->inpstring = p;
    }
  if (s)
    {
      strncpy(inpdata->inpbuf, s, sizeof(inpdata->inpbuf) - 1);
      inpdata->inpbuf[sizeof(inpdata->inpbuf) - 1] = 0;
      inpdata->inplen = strlen(inpdata->inpbuf);
    }
  RefreshLine(STATLINE, 0, D_width - 1, 0);
}

/*
 * We dont use HS status line with Input().
 * If we would use it, then we should check e_tgetflag("es") if
 * we are allowed to use esc sequences there.
 *
 * mode is an OR of
 * INP_NOECHO == suppress echoing of characters.
 * INP_RAW    == raw mode. call finfunc after each character typed.
 */
void
Input(istr, len, finfunc, mode)
char *istr;
int len;
void (*finfunc)();
int mode;
{
  int maxlen;
  struct inpdata *inpdata;
  
  if (!display)
    {
      Msg(0, "Input: cannot interact with user w/o display. Try other form of command\n");
      return;
    }
  if (len > 100)
    len = 100;
  if (!(mode & INP_NOECHO))
    {
      maxlen = D_width - strlen(istr);
      if (!D_CLP && STATLINE == D_bot)
	maxlen--;
      if (len > maxlen)
	len = maxlen;
    }
  if (len < 0)
    {
      Msg(0, "Width %d chars too small", -len);
      return;
    }
  if (InitOverlayPage(sizeof(*inpdata), &InpLf, 1))
    return;
  inpdata = (struct inpdata *)D_lay->l_data;
  inpdata->inpmaxlen = len;
  inpdata->inpfinfunc = finfunc;
  inpdata->inplen = 0;
  inpdata->inpmode = mode;
  inp_setprompt(istr, (char *)NULL);
}

static void
InpSetCursor()
{
  struct inpdata *inpdata;
  
  inpdata = (struct inpdata *)D_lay->l_data;
  GotoPos(inpdata->inpstringlen + (inpdata->inpmode & INP_NOECHO ? 0 : inpdata->inplen), STATLINE);
}

static void
InpProcess(ppbuf, plen)
char **ppbuf;
int *plen;
{
  int len, x;
  char *pbuf;
  char ch;
  struct inpdata *inpdata;
  
  inpdata = (struct inpdata *)D_lay->l_data;

  GotoPos(inpdata->inpstringlen + (inpdata->inpmode & INP_NOECHO ? 0 : inpdata->inplen), STATLINE);
  if (ppbuf == 0)
    {
      InpAbort();
      return;
    }
  x = inpdata->inpstringlen + inpdata->inplen;
  len = *plen;
  pbuf = *ppbuf;
  while (len)
    {
      ch = *pbuf++;
      len--;
      if (inpdata->inpmode & INP_RAW)
	{
          (*inpdata->inpfinfunc)(&ch, 1);	/* raw */
	  if (ch)
	    continue;
	}
      if ((unsigned char)ch >= ' ' && ch != 0177 && inpdata->inplen < inpdata->inpmaxlen)
	{
	  inpdata->inpbuf[inpdata->inplen++] = ch;
	  if (!(inpdata->inpmode & INP_NOECHO))
	    {
	      GotoPos(x, STATLINE);
	      SetAttrFont(A_SO, ASCII);
	      PUTCHAR(ch);
	      x++;
	    }
	}
      else if ((ch == '\b' || ch == 0177) && inpdata->inplen > 0)
	{
	  inpdata->inplen--;
	  if (!(inpdata->inpmode & 1))
	    {
	      x--;
	      GotoPos(x, STATLINE);
	      SetAttrFont(0, ASCII);
	      PUTCHAR(' ');
	      GotoPos(x, STATLINE);
	    }
	}
      else if (ch == '\004' || ch == '\003' || ch == '\007' || ch == '\033' ||
	       ch == '\000' || ch == '\n' || ch == '\r')
	{
          if (ch != '\033' && ch != '\n' && ch != '\r')
	    inpdata->inplen = 0;
	  inpdata->inpbuf[inpdata->inplen] = 0;
	  
  	  D_lay->l_data = 0;
          InpAbort(); /* redisplays... */
	  *ppbuf = pbuf;
	  *plen = len;
          if ((inpdata->inpmode & INP_RAW) == 0)
            (*inpdata->inpfinfunc)(inpdata->inpbuf, inpdata->inplen);
	  else
            (*inpdata->inpfinfunc)(pbuf - 1, 0);
	  free((char *)inpdata);
	  return;
	}
    }
  *ppbuf = pbuf;
  *plen = len;
}

static void
InpAbort()
{
  LAY_CALL_UP(RefreshLine(STATLINE, 0, D_width - 1, 0));
  ExitOverlayPage();
}

static void
InpRedisplayLine(y, xs, xe, isblank)
int y, xs, xe, isblank;
{
  int q, r, s, l, v;
  struct inpdata *inpdata;
  
  inpdata = (struct inpdata *)D_lay->l_data;

  if (y != STATLINE)
    {
      LAY_CALL_UP(RefreshLine(y, xs, xe, isblank));
      return;
    }
  inpdata->inpbuf[inpdata->inplen] = 0;
  GotoPos(xs, y);
  q = xs;
  v = xe - xs + 1;
  s = 0;
  r = inpdata->inpstringlen;
  if (v > 0 && q < r)
    {
      SetAttrFont(A_SO, ASCII);
      l = v;
      if (l > r-q)
	l = r-q;
      AddStrn(inpdata->inpstring + q - s, l);
      q += l;
      v -= l;
    }
  s = r;
  r += inpdata->inplen;
  if (!(inpdata->inpmode & INP_NOECHO) && v > 0 && q < r)
    {
      SetAttrFont(A_SO, ASCII);
      l = v;
      if (l > r-q)
	l = r-q;
      AddStrn(inpdata->inpbuf + q - s, l);
      q += l;
      v -= l;
    }
  s = r;
  r = D_width;
  if (!isblank && v > 0 && q < r)
    {
      SetAttrFont(0, ASCII);
      l = v;
      if (l > r-q)
	l = r-q;
      AddStrn("", l);
      q += l;
    }
  SetLastPos(q, y);
}
