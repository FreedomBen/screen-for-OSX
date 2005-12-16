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
static int ResizeScreenArray __P((struct win *, char ***, int, int, int));
static void FreeArray __P((char ***, int));
#ifdef COPY_PASTE
static int ResizeHistArray __P((struct win *, char ***, int, int, int));
static void FreeScrollback __P((struct win *));
#endif

extern int maxwidth;
extern struct display *display, *displays;
extern char *blank, *null, *OldImage, *OldAttr;
extern char *OldFont;
extern struct win *windows;
extern int Z0width, Z1width;

#ifdef NETHACK
extern int nethackflag;
#endif

#if defined(TIOCGWINSZ) || defined(TIOCSWINSZ)
  struct winsize glwz;
#endif

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
          ChangeWindowSize(p, wi, he);
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
          ChangeWindowSize(p, wwi, he);
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

#ifdef COPY_PASTE

int
ChangeScrollback(p, histheight, histwidth)
struct win *p;
int histheight, histwidth;
{
  if (histheight > MAXHISTHEIGHT)
    histheight = MAXHISTHEIGHT;
  debug2("ChangeScrollback(..., %d, %d)\n", histheight, histwidth);
  debug2("  was %d, %d\n", p->w_histheight, p->w_width);

  if (histheight == 0)
    {
      FreeScrollback(p);
      return 0;
    }

  if (ResizeHistArray(p, &p->w_ihist, histwidth, histheight, 1)
      || ResizeHistArray(p, &p->w_ahist, histwidth, histheight, 0)
      || ResizeHistArray(p, &p->w_fhist, histwidth, histheight, 0))
    {
      debug("   failed, removing all histbuf\n");
      FreeScrollback(p);
      Msg(0, strnomem);
      return -1;
    }
  if (p->w_histheight != histheight)
    p->w_histidx = 0;
  p->w_histheight = histheight;

  return 0;
}

static void
FreeScrollback(p)
struct win *p;
{
  FreeArray(&p->w_ihist, p->w_histheight);
  FreeArray(&p->w_ahist, p->w_histheight);
  FreeArray(&p->w_fhist, p->w_histheight);
  p->w_histheight = 0;
}

static int
ResizeHistArray(p, arr, wi, hi, fillblank)
struct win *p;
char ***arr;
int wi, hi, fillblank;
{
  char **narr, **np, **onp, **onpe;
  int t, x, first;

  if (p->w_width == wi && p->w_histheight == hi)
    return(0);
  if (p->w_histheight != hi)
    {
      if ((narr = (char **)calloc(sizeof(char *), hi)) == NULL)
	{
	  FreeArray(arr, p->w_histheight);
	  return(-1);
	}
      np = narr;
      onp = (*arr) + p->w_histidx;
      onpe = (*arr) + p->w_histheight;
      first = p->w_histheight - hi;
      if (first < 0)
	 np -= first;
      for(t = 0; t < p->w_histheight; t++)
	{
	  ASSERT(*onp);
          if (t - first >= 0 && t - first < hi)
	    *np++ = *onp;
	  else if (*onp != null)
	    free(*onp);
	  if (++onp == onpe)
	    onp = *arr;
	}
      if (*arr)
	free((char *)*arr);
    }
  else
    narr = *arr;
 
  for (t=0, np=narr; t < hi; t++, np++)
    {
      if ((!fillblank && *np == 0) || *np == null)
	{
	  *np = null;
	  continue;
	}
      x = p->w_width;
      if (*np == 0)
	{
	  *np = (char *)malloc(wi + 1);
          x = 0;
	}
      else if (p->w_width != wi)
	{
	  *np = (char *)xrealloc(*np, wi + 1);
	}
      if (*np == 0)
	{
	  FreeArray(&narr, hi);
	  return -1;
	}
      if (x > wi)
	x = wi;
      if (fillblank)
	bclear(*np + x, wi + 1 - x);
      else
	bzero(*np + x, wi + 1 - x);
    }
  *arr = narr;
  return 0;
}
#endif /* COPY_PASTE */
      

static int
ResizeScreenArray(p, arr, wi, hi, fillblank)
struct win *p;
char ***arr;
int wi, hi, fillblank;
{
  int minr;
  char **cp;

  if (p->w_width == wi && p->w_height == hi)
    return(0);

  if (hi > p->w_height)
    minr = p->w_height;
  else
    minr = hi;

  if (p->w_height > hi)
    {
      for (cp = *arr; cp < *arr + (p->w_height - hi); cp++)
        if (*cp != null)
	  free(*cp);
      bcopy((char *)(*arr + (p->w_height - hi)), (char *)(*arr),
	    hi * sizeof(char *));
    }
  if (*arr && p->w_width != wi)
    for (cp = *arr; cp < *arr + minr; cp++)
      {
	int x = p->w_width;

	if (*cp == null)
	  continue;
	if ((*cp = (char *)xrealloc(*cp, (unsigned) wi + 1)) == 0)
	  {
	    FreeArray(arr, p->w_height);
	    return(-1);
	  }
	if (x > wi)
	  x = wi;
	if (fillblank)
	  bclear(*cp + x, wi + 1 - x);
	else
	  bzero(*cp + x, wi + 1 - x);
      }
  if (*arr)
    *arr = (char **) xrealloc((char *) *arr, (unsigned) hi * sizeof(char *));
  else
    *arr = (char **) malloc((unsigned) hi * sizeof(char *));
  if (*arr == NULL)
    return -1;
  for (cp = *arr + p->w_height; cp < *arr + hi; cp++)
    {
      if (!fillblank)
	{
	  *cp = null;
	  continue;
	}
      if ((*cp = malloc((unsigned) wi + 1)) == 0)
	{
	  while (--cp >= *arr)
	    free(*cp);
	  free((char *)*arr);
	  *arr = NULL;
          return -1;
	}
      bclear(*cp, wi + 1);
    }
  return 0;
}

static void
FreeArray(arr, hi)
char ***arr;
int hi;
{
  register char **p;
  register int t;

  if (*arr == 0)
    return;
  for (t = hi, p = *arr; t--; p++)
    if (*p && *p != null)
      free(*p);
  free((char *)*arr);
  *arr = 0;
}


static void
CheckMaxSize(wi)
int wi;
{
  char *oldnull = null;
  struct win *p;
  int i;

  wi = ((wi + 1) + 255) & ~255;
  if (wi <= maxwidth)
    return;
  maxwidth = wi;
  debug1("New maxwidth: %d\n", maxwidth);
  if (blank == 0)
    blank = malloc((unsigned) maxwidth);
  else
    blank = xrealloc(blank, (unsigned) maxwidth);
  if (null == 0)
    null = malloc((unsigned) maxwidth);
  else
    null = xrealloc(null, (unsigned) maxwidth);
  if (OldImage == 0)
    OldImage = malloc((unsigned) maxwidth);
  else
    OldImage = xrealloc(OldImage, (unsigned) maxwidth);
  if (OldAttr == 0)
    OldAttr = malloc((unsigned) maxwidth);
  else
    OldAttr = xrealloc(OldAttr, (unsigned) maxwidth);
  if (OldFont == 0)
    OldFont = malloc((unsigned) maxwidth);
  else
    OldFont = xrealloc(OldFont, (unsigned) maxwidth);
  if (!(blank && null && OldImage && OldAttr && OldFont))
    {
      Panic(0, "Out of memory -> Game over!!");
      /*NOTREACHED*/
    }
  MakeBlankLine(blank, maxwidth);
  bzero(null, maxwidth);

  /* We have to run through all windows to substitute
   * the null references.
   */
  for (p = windows; p; p = p->w_next)
    {
      for (i = 0; i < p->w_height; i++)
	{
	  if (p->w_attr[i] == oldnull)
	    p->w_attr[i] = null;
	  if (p->w_font[i] == oldnull)
	    p->w_font[i] = null;
	}
#ifdef COPY_PASTE
      for (i = 0; i < p->w_histheight; i++)
	{
	  if (p->w_ahist[i] == oldnull)
	    p->w_ahist[i] = null;
	  if (p->w_fhist[i] == oldnull)
	    p->w_fhist[i] = null;
	}
#endif
    }
}


int
ChangeWindowSize(p, wi, he)
struct win *p;
int wi, he;
{
  int t, scr;
  
  CheckMaxSize(wi);
  
  if (wi == p->w_width && he == p->w_height)
    {
      debug("ChangeWindowSize: No change.\n");
      return 0;
    }

  debug2("ChangeWindowSize from (%d,%d) to ", p->w_width, p->w_height);
  debug2("(%d,%d)\n", wi, he);
  if (p->w_lay != &p->w_winlay)
    {
      debug("ChangeWindowSize: No resize because of overlay.\n");
      return -1;
    }
  if (wi == 0 && he == 0)
    {
      FreeArray(&p->w_image, p->w_height);
      FreeArray(&p->w_attr, p->w_height);
      FreeArray(&p->w_font, p->w_height);
      if (p->w_tabs)
	free(p->w_tabs);
      p->w_tabs = NULL;
      p->w_width = 0;
      p->w_height = 0;
#ifdef COPY_PASTE
      FreeScrollback(p);
#endif
      return 0;
    }

  /* when window gets smaller, scr is the no. of lines we scroll up */
  scr = p->w_height - he;
  if (scr < 0)
    scr = 0;
#ifdef COPY_PASTE
  for (t = 0; t < scr; t++)
    AddLineToHist(p, p->w_image+t, p->w_attr+t, p->w_font+t); 
#endif
  if (ResizeScreenArray(p, &p->w_image, wi, he, 1)
      || ResizeScreenArray(p, &p->w_attr, wi, he, 0)
      || ResizeScreenArray(p, &p->w_font, wi, he, 0))
    {
nomem:	  KillWindow(p);
      Msg(0, "Out of memory -> Window destroyed !!");
      return -1;
    }
  /* this won't change the D_height of the scrollback history buffer, but
   * it will check the D_width of the lines.
   */
#ifdef COPY_PASTE
  ChangeScrollback(p, p->w_histheight, wi);
#endif

  if (p->w_tabs == 0)
    {
      /* tabs get D_width+1 because 0 <= x <= wi */
      if ((p->w_tabs = malloc((unsigned) wi + 1)) == 0)
        goto nomem;
      t = 8;
    }
  else
    {
      if ((p->w_tabs = xrealloc(p->w_tabs, (unsigned) wi + 1)) == 0)
        goto nomem;
      t = p->w_width;
    }
  for (t = (t + 7) & 8; t < wi; t += 8)
    p->w_tabs[t] = 1; 
  p->w_height = he;
  p->w_width = wi;
  if (p->w_x >= wi)
    p->w_x = wi - 1;
  if ((p->w_y -= scr) < 0)
    p->w_y = 0;
  if (p->w_Saved_x >= wi)
    p->w_Saved_x = wi - 1;
  if ((p->w_Saved_y -= scr) < 0)
    p->w_Saved_y = 0;
  if (p->w_autoaka > 0) 
    if ((p->w_autoaka -= scr) < 1)
      p->w_autoaka = 1;
  p->w_top = 0;
  p->w_bot = he - 1;
#ifdef TIOCSWINSZ
  if (p->w_ptyfd >= 0 && p->w_pid)
    {
      glwz.ws_col = wi;
      glwz.ws_row = he;
      debug("Setting pty winsize.\n");
      if (ioctl(p->w_ptyfd, TIOCSWINSZ, (char *)&glwz))
	debug2("SetPtySize: errno %d (fd:%d)\n", errno, p->w_ptyfd);
# if defined(STUPIDTIOCSWINSZ) && defined(SIGWINCH)
#  ifdef POSIX
      pgrp = tcgetpgrp(p->w_ptyfd);
#  else /* POSIX */
      if (ioctl(p->w_ptyfd, TIOCGPGRP, (char *)&pgrp))
	pgrp = 0;
#  endif /* POSIX */
      if (pgrp)
	{
	  debug1("Sending SIGWINCH to pgrp %d.\n", pgrp);
	  if (killpg(pgrp, SIGWINCH))
	    debug1("killpg: errno %d\n", errno);
	}
      else
	debug1("Could not get pgrp: errno %d\n", errno);
# endif /* STUPIDTIOCSWINSZ */
    }
#endif /* TIOCSWINSZ */
  return 0;
}


char *
xrealloc(mem, len)
char *mem;
int len;
{
  register char *nmem;

  if ((nmem = realloc(mem, len)))
    return(nmem);
  free(mem);
  return((char *)0);
}
