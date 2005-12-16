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
#include <signal.h>
#ifndef sun
#include <sys/ioctl.h>
#endif

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#include "config.h"
#include "screen.h"
#include "extern.h"

static void CheckMaxSize __P((int));
static void FreeMline  __P((struct mline *));
static int  AllocMline __P((struct mline *ml, int));
static void MakeBlankLine __P((char *, int));
static int  BcopyMline __P((struct mline *, int, struct mline *, int, int, int));

extern struct display *display, *displays;
extern char *blank, *null;
extern struct mline mline_blank, mline_null, mline_old;
extern struct win *windows;
extern int Z0width, Z1width;

#ifdef NETHACK
extern int nethackflag;
#endif

#if defined(TIOCGWINSZ) || defined(TIOCSWINSZ)
  struct winsize glwz;
#endif

static struct mline mline_zero = {
 (char *)0,
 (char *)0,
 (char *)0
#ifdef COLOR
 ,(char *)0
#endif
};

/*
 * ChangeFlag:   0: try to modify no window
 *               1: modify fore (and try to modify no D_other) + redisplay
 *               2: modify all windows
 *
 * Note: Activate() is only called if change_flag == 1
 *       i.e. on a WINCH event
 */

void
CheckScreenSize(change_flag)
int change_flag;
{
  int wi, he;
  struct win *p;
  struct layer *oldlay;

  if (display == 0)
    {
      debug("CheckScreenSize: No display -> no check.\n");
      return;
    }
  oldlay = D_lay;
#ifdef TIOCGWINSZ
  if (ioctl(D_userfd, TIOCGWINSZ, (char *)&glwz) != 0)
    {
      debug2("CheckScreenSize: ioctl(%d, TIOCGWINSZ) errno %d\n", D_userfd, errno);
      wi = D_CO;
      he = D_LI;
    }
  else
    {
      wi = glwz.ws_col;
      he = glwz.ws_row;
      if (wi == 0)
        wi = D_CO;
      if (he == 0)
        he = D_LI;
    }
#else
  wi = D_CO;
  he = D_LI;
#endif
  
  debug2("CheckScreenSize: screen is (%d,%d)\n", wi, he);

  if (change_flag == 2)
    {
      debug("Trying to adapt all windows (-A)\n");
      for (p = windows; p; p = p->w_next)
	if (p->w_display == 0 || p->w_display == display)
          ChangeWindowSize(p, wi, he, p->w_histheight);
    }
  if (D_width == wi && D_height == he)
    {
      debug("CheckScreenSize: No change -> return.\n");
      return;
    }
  ChangeScreenSize(wi, he, change_flag);
  if (change_flag == 1)
    Activate(D_fore ? D_fore->w_norefresh : 0);
  if (D_lay != oldlay)
    {
#ifdef NETHACK
      if (nethackflag)
	Msg(0, "KAABLAMM!!!  You triggered a land mine!");
      else
#endif
      Msg(0, "Aborted because of window size change.");
    }
}

void
ChangeScreenSize(wi, he, change_fore)
int wi, he;
int change_fore;
{
  struct win *p;
  int wwi;

  if (D_width == wi && D_height == he)
    {
      debug("ChangeScreenSize: no change\n");
      return;
    }
  debug2("ChangeScreenSize from (%d,%d) ", D_width, D_height);
  debug3("to (%d,%d) (change_fore: %d)\n",wi, he, change_fore);
  D_width = wi;
  D_height = he;

  CheckMaxSize(wi);
  if (D_CWS)
    {
      D_defwidth = D_CO;
      D_defheight = D_LI;
    }
  else
    {
      if (D_CZ0 && (wi == Z0width || wi == Z1width) &&
          (D_CO == Z0width || D_CO == Z1width))
        D_defwidth = D_CO;
      else
        D_defwidth = wi;
      D_defheight = he;
    }
  debug2("Default size: (%d,%d)\n", D_defwidth, D_defheight);
  if (change_fore)
    DoResize(wi, he);
  if (D_CWS == NULL && displays->d_next == 0)
    {
      /* adapt all windows  -  to be removed ? */
      for (p = windows; p; p = p->w_next)
        {
          debug1("Trying to change window %d.\n", p->w_number);
          wwi = wi;
          if (D_CZ0 && (wi == Z0width || wi == Z1width))
	    {
	      if (p->w_width > (Z0width + Z1width) / 2)
		wwi = Z0width;
	      else
		wwi = Z1width;
	    }
          ChangeWindowSize(p, wwi, he, p->w_histheight);
        }
    }
}

void
DoResize(wi, he)
int wi, he;
{
  struct layer *oldlay;
  int q = 0;

  for(;;)
    {
      oldlay = D_lay;
      for (; D_lay; D_lay = D_lay->l_next)
	{
	  D_layfn = D_lay->l_layfn;
	  if ((q = Resize(wi, he)))
	    break;
	}
      D_lay = oldlay;
      D_layfn = D_lay->l_layfn;
      if (q == 0)
	break;
      ExitOverlayPage();
    }
}


static void
FreeMline(ml)
struct mline *ml;
{
  if (ml->image)
    free(ml->image);
  if (ml->attr && ml->attr != null)
    free(ml->attr);
  if (ml->font && ml->font != null)
    free(ml->font);
#ifdef COLOR
  if (ml->color && ml->color != null)
    free(ml->color);
#endif
  *ml = mline_zero;
}

static int
AllocMline(ml, w)
struct mline *ml;
int w;
{
  ml->image = malloc(w);
  ml->attr  = null;
  ml->font  = null;
#ifdef COLOR
  ml->color = null;
#endif
  if (ml->image == 0)
    return -1;
  return 0;
}


static int
BcopyMline(mlf, xf, mlt, xt, l, w)
struct mline *mlf, *mlt;
int xf, xt, l, w;
{
  int r = 0;

  bcopy(mlf->image + xf, mlt->image + xt, l);
  if (mlf->attr != null && mlt->attr == null)
    {
      if ((mlt->attr = malloc(w)) == 0)
	mlt->attr = null, r = -1;
      bzero(mlt->attr, w);
    }
  if (mlt->attr != null)
    bcopy(mlf->attr + xf, mlt->attr + xt, l);
  if (mlf->font != null && mlt->font == null)
    {
      if ((mlt->font = malloc(w)) == 0)
	mlt->font = null, r = -1;
      bzero(mlt->font, w);
    }
  if (mlt->font != null)
    bcopy(mlf->font + xf, mlt->font + xt, l);
#ifdef COLOR
  if (mlf->color != null && mlt->color == null)
    {
      if ((mlt->color = malloc(w)) == 0)
	mlt->color = null, r = -1;
      bzero(mlt->color, w);
    }
  if (mlt->color != null)
    bcopy(mlf->color + xf, mlt->color + xt, l);
#endif
  return r;
}


static int maxwidth;

static void
CheckMaxSize(wi)
int wi;
{
  char *oldnull = null;
  struct win *p;
  int i;
  struct mline *ml;

  wi = ((wi + 1) + 255) & ~255;
  if (wi <= maxwidth)
    return;
  maxwidth = wi;
  debug1("New maxwidth: %d\n", maxwidth);
  if (blank == 0)
    blank = malloc((unsigned) maxwidth);
  else
    blank = xrealloc(blank, maxwidth);
  if (null == 0)
    null = malloc((unsigned) maxwidth);
  else
    null = xrealloc(null, maxwidth);
  if (mline_old.image == 0)
    mline_old.image = malloc((unsigned) maxwidth);
  else
    mline_old.image = xrealloc(mline_old.image, maxwidth);
  if (mline_old.attr == 0)
    mline_old.attr = malloc((unsigned) maxwidth);
  else
    mline_old.attr = xrealloc(mline_old.attr, maxwidth);
  if (mline_old.font == 0)
    mline_old.font = malloc((unsigned) maxwidth);
  else
    mline_old.font = xrealloc(mline_old.font, maxwidth);
#ifdef COLOR
  if (mline_old.color == 0)
    mline_old.color = malloc((unsigned) maxwidth);
  else
    mline_old.color = xrealloc(mline_old.color, maxwidth);
  if (!(blank && null && mline_old.image && mline_old.attr
                      && mline_old.font && mline_old.color))
    {
      Panic(0, "Out of memory -> Game over!!");
      /*NOTREACHED*/
    }
#else
  if (!(blank && null && mline_old.image && mline_old.attr
                      && mline_old.font))
    {
      Panic(0, "Out of memory -> Game over!!");
      /*NOTREACHED*/
    }
#endif
  MakeBlankLine(blank, maxwidth);
  bzero(null, maxwidth);

  mline_blank.image = blank;
  mline_blank.attr  = null;
  mline_blank.font  = null;
  mline_null.image = null;
  mline_null.attr  = null;
  mline_null.font  = null;
#ifdef COLOR
  mline_blank.color = null;
  mline_null.color = null;
#endif

  /* We have to run through all windows to substitute
   * the null references.
   */
  for (p = windows; p; p = p->w_next)
    {
      ml = p->w_mlines;
      for (i = 0; i < p->w_height; i++, ml++)
	{
	  if (ml->attr == oldnull)
	    ml->attr = null;
	  if (ml->font == oldnull)
	    ml->font = null;
#ifdef COLOR
	  if (ml->color== oldnull)
	    ml->color= null;
#endif
	}
#ifdef COPY_PASTE
      ml = p->w_hlines;
      for (i = 0; i < p->w_histheight; i++, ml++)
	{
	  if (ml->attr == oldnull)
	    ml->attr = null;
	  if (ml->font == oldnull)
	    ml->font = null;
# ifdef COLOR
	  if (ml->color== oldnull)
	    ml->color= null;
# endif
	}
#endif
    }
}


char *
xrealloc(mem, len)
char *mem;
int len;
{
  register char *nmem;

  if ((nmem = realloc(mem, len)))
    return nmem;
  free(mem);
  return (char *)0;
}

static void
MakeBlankLine(p, n)
register char *p;
register int n;
{
  while (n--)
    *p++ = ' ';
}




#ifdef COPY_PASTE

#define OLDWIN(y) ((y < p->w_histheight) \
        ? &p->w_hlines[(p->w_histidx + y) % p->w_histheight] \
        : &p->w_mlines[y - p->w_histheight])

#define NEWWIN(y) ((y < hi) ? &nhlines[y] : &nmlines[y - hi])
	
#else

#define OLDWIN(y) (&p->w_mlines[y])
#define NEWWIN(y) (&nmlines[y])

#endif


int
ChangeWindowSize(p, wi, he, hi)
struct win *p;
int wi, he, hi;
{
  struct mline *mlf = 0, *mlt = 0, *ml, *nmlines, *nhlines;
  int fy, ty, l, lx, lf, lt, yy, oty, addone;
  int ncx, ncy, naka, t;
  int y, shift;

  if (wi == 0)
    he = hi = 0;

  if (p->w_width == wi && p->w_height == he && p->w_histheight == hi)
    {
      debug("ChangeWindowSize: No change.\n");
      return 0;
    }

  CheckMaxSize(wi);

  /* just in case ... */
  if (wi && (p->w_width != wi || p->w_height != he) && p->w_lay != &p->w_winlay)
    {
      debug("ChangeWindowSize: No resize because of overlay?\n");
      return -1;
    }

  debug("ChangeWindowSize");
  debug3(" from (%d,%d)+%d", p->w_width, p->w_height, p->w_histheight);
  debug3(" to(%d,%d)+%d\n", wi, he, hi);

  fy = p->w_histheight + p->w_height - 1;
  ty = hi + he - 1;

  nmlines = nhlines = 0;
  ncx = 0;
  ncy = 0;
  naka = 0;

  if (wi)
    {
      if (wi != p->w_width || he != p->w_height)
	{
	  if ((nmlines = (struct mline *)calloc(he, sizeof(struct mline))) == 0)
	    {
	      KillWindow(p);
	      Msg(0, strnomem);
	      return -1;
	    }
	}
      else
	{
	  debug1("image stays the same: %d lines\n", he);
	  nmlines = p->w_mlines;
	  fy -= he;
	  ty -= he;
	  ncx = p->w_x;
	  ncy = p->w_y;
	  naka = p->w_autoaka;
	}
    }
#ifdef COPY_PASTE
  if (hi)
    {
      if ((nhlines = (struct mline *)calloc(hi, sizeof(struct mline))) == 0)
	{
	  Msg(0, "No memory for history buffer - turned off");
	  hi = 0;
	  ty = he - 1;
	}
    }
#endif

  /* special case: cursor is at magic margin position */
  addone = 0;
  if (p->w_width && p->w_x == p->w_width)
    {
      debug2("Special addone case: %d %d\n", p->w_x, p->w_y);
      addone = 1;
      p->w_x--;
    }

  /* handle the cursor and autoaka lines now if the widths are equal */
  if (p->w_width == wi)
    {
      ncx = p->w_x + addone;
      ncy = p->w_y + he - p->w_height;
      /* never lose sight of the line with the cursor on it */
      shift = -ncy;
      for (yy = p->w_y + p->w_histheight - 1; yy >= 0 && ncy + shift < he; yy--)
	{
	  ml = OLDWIN(yy);
	  if (ml->image[p->w_width] == ' ')
	    break;
	  shift++;
	}
      if (shift < 0)
	shift = 0;
      else
	debug1("resize: cursor out of bounds, shifting %d\n", shift);
      ncy += shift;
      if (p->w_autoaka > 0)
	{
	  naka = p->w_autoaka + he - p->w_height + shift;
	  if (naka < 1 || naka > he)
	    naka = 0;
	}
      while (shift-- > 0)
	{
	  ml = OLDWIN(fy);
	  FreeMline(ml);
	  fy--;
	}
    }
  debug2("fy %d ty %d\n", fy, ty);
  if (fy >= 0)
    mlf = OLDWIN(fy);
  if (ty >= 0)
    mlt = NEWWIN(ty);

  while (fy >= 0 && ty >= 0)
    {
      if (p->w_width == wi)
	{
	  /* here is a simple shortcut: just copy over */
	  *mlt = *mlf;
          *mlf = mline_zero;
	  if (--fy >= 0)
	    mlf = OLDWIN(fy);
	  if (--ty >= 0)
	    mlt = NEWWIN(ty);
	  continue;
	}

      /* calculate lenght */
      for (l = p->w_width - 1; l > 0; l--)
	if (mlf->image[l] != ' ' || mlf->attr[l])
	  break;
      if (fy == p->w_y + p->w_histheight && l < p->w_x)
	l = p->w_x;	/* cursor is non blank */
      l++;
      lf = l;

      /* add wrapped lines to length */
      for (yy = fy - 1; yy >= 0; yy--)
	{
	  ml = OLDWIN(yy);
	  if (ml->image[p->w_width] == ' ')
	    break;
	  l += p->w_width;
	}

      /* rewrap lines */
      lt = (l - 1) % wi + 1;	/* lf is set above */
      oty = ty;
      while (l > 0 && fy >= 0 && ty >= 0)
	{
	  lx = lt > lf ? lf : lt;
	  if (mlt->image == 0)
	    {
	      if (AllocMline(mlt, wi + 1))
		goto nomem;
    	      MakeBlankLine(mlt->image + lt, wi - lt);
	      mlt->image[wi] = ((oty == ty) ? ' ' : 0);
	    }
	  if (BcopyMline(mlf, lf - lx, mlt, lt - lx, lx, wi + 1))
	    goto nomem;

	  /* did we copy the cursor ? */
	  if (fy == p->w_y + p->w_histheight && lf - lx <= p->w_x && lf > p->w_x)
	    {
	      ncx = p->w_x + lt - lf + addone;
	      ncy = ty - hi;
	      shift = wi ? -ncy + (l - lx) / wi : 0;
	      if (ty + shift > hi + he - 1)
		shift = hi + he - 1 - ty;
	      if (shift > 0)
		{
	          debug3("resize: cursor out of bounds, shifting %d [%d/%d]\n", shift, lt - lx, wi);
		  for (y = hi + he - 1; y >= ty; y--)
		    {
		      mlt = NEWWIN(y);
		      FreeMline(mlt);
		      if (y - shift < ty)
			continue;
		      ml  = NEWWIN(y - shift);
		      *mlt = *ml;
		      *ml = mline_zero;
		    }
		  ncy += shift;
		  ty += shift;
		  mlt = NEWWIN(ty);
		  if (naka > 0)
		    naka = naka + shift > he ? 0 : naka + shift;
		}
	      ASSERT(ncy >= 0);
	    }
	  /* did we copy autoaka line ? */
	  if (p->w_autoaka > 0 && fy == p->w_autoaka - 1 + p->w_histheight && lf - lx <= 0)
	    naka = ty - hi >= 0 ? 1 + ty - hi : 0;

	  lf -= lx;
	  lt -= lx;
	  l  -= lx;
	  if (lf == 0)
	    {
	      FreeMline(mlf);
	      lf = p->w_width;
	      if (--fy >= 0)
	        mlf = OLDWIN(fy);
	    }
	  if (lt == 0)
	    {
	      lt = wi;
	      if (--ty >= 0)
	        mlt = NEWWIN(ty);
	    }
	}
      ASSERT(l != 0 || fy == yy);
    }
  while (fy >= 0)
    {
      FreeMline(mlf);
      if (--fy >= 0)
	mlf = OLDWIN(fy);
    }
  while (ty >= 0)
    {
      if (AllocMline(mlt, wi + 1))
	goto nomem;
      MakeBlankLine(mlt->image, wi + 1);
      if (--ty >= 0)
	mlt = NEWWIN(ty);
    }

#ifdef DEBUG
  if (nmlines != p->w_mlines)
    for (fy = 0; fy < p->w_height + p->w_histheight; fy++)
      {
	ml = OLDWIN(fy);
	ASSERT(ml->image == 0);
      }
#endif

  if (p->w_mlines && p->w_mlines != nmlines)
    free((char *)p->w_mlines);
  p->w_mlines = nmlines;
#ifdef COPY_PASTE
  if (p->w_hlines && p->w_hlines != nhlines)
    free((char *)p->w_hlines);
  p->w_hlines = nhlines;
#endif
  nmlines = nhlines = 0;

  /* change tabs */
  if (p->w_width != wi)
    {
      if (wi)
	{
	  if (p->w_tabs == 0)
	    {
	      /* tabs get wi+1 because 0 <= x <= wi */
	      p->w_tabs = malloc((unsigned) wi + 1);
	      t = 0;
	    }
	  else
	    {
	      p->w_tabs = xrealloc(p->w_tabs, wi + 1);
	      t = p->w_width;
	    }
	  if (p->w_tabs == 0)
	    {
	    nomem:
	      if (nmlines)
		{
		  for (ty = he + hi - 1; ty >= 0; ty--)
		    {
		      mlt = NEWWIN(ty);
		      FreeMline(mlt);
		    }
		  if (nmlines && p->w_mlines != nmlines)
		    free((char *)nmlines);
#ifdef COPY_PASTE
		  if (nhlines && p->w_hlines != nhlines)
		    free((char *)nhlines);
#endif
		}
	      KillWindow(p);
	      Msg(0, strnomem);
	      return -1;
	    }
	  for (; t < wi; t++)
	    p->w_tabs[t] = t && !(t & 7) ? 1 : 0; 
	  p->w_tabs[wi] = 0; 
	}
      else
	{
	  if (p->w_tabs)
	    free(p->w_tabs);
	  p->w_tabs = 0;
	}
    }

  /* Change w_Saved_y - this is only an estimate... */
  p->w_Saved_y += ncy - p->w_y;

  p->w_x = ncx;
  p->w_y = ncy;
  if (p->w_autoaka > 0)
    p->w_autoaka = naka;

  /* do sanity checks */
  if (p->w_x > wi)
    p->w_x = wi;
  if (p->w_y >= he)
    p->w_y = he - 1;
  if (p->w_Saved_x > wi)
    p->w_Saved_x = wi;
  if (p->w_Saved_y < 0)
    p->w_Saved_y = 0;
  if (p->w_Saved_y >= he)
    p->w_Saved_y = he - 1;

  /* reset scrolling region */
  p->w_top = 0;
  p->w_bot = he - 1;

  /* signal new size to window */
#ifdef TIOCSWINSZ
  if (wi && (p->w_width != wi || p->w_height != he) && p->w_ptyfd >= 0 && p->w_pid)
    {
      glwz.ws_col = wi;
      glwz.ws_row = he;
      debug("Setting pty winsize.\n");
      if (ioctl(p->w_ptyfd, TIOCSWINSZ, (char *)&glwz))
	debug2("SetPtySize: errno %d (fd:%d)\n", errno, p->w_ptyfd);
    }
#endif /* TIOCSWINSZ */

  /* store new size */
  p->w_width = wi;
  p->w_height = he;
#ifdef COPY_PASTE
  p->w_histidx = 0;
  p->w_histheight = hi;
#endif

#ifdef DEBUG
  /* Test if everything was ok */
  for (fy = 0; fy < p->w_height + p->w_histheight; fy++)
    {
      ml = OLDWIN(fy);
      ASSERT(ml->image);
      for (l = 0; l < p->w_width; l++)
      ASSERT((unsigned char)ml->image[l] >= ' ');
    }
#endif
  return 0;
}

