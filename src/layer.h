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
 * $Id$ FAU
 */

/*
 * This is the overlay structure. It is used to create a seperate
 * layer over the current windows.
 */

struct mchar;	/* forward declaration */

struct LayFuncs
{
  void	(*LayProcess) __P((char **, int *));
  void	(*LayAbort) __P((void));
  void	(*LayRedisplayLine) __P((int, int, int, int));
  void	(*LayClearLine) __P((int, int, int));
  int	(*LayRewrite) __P((int, int, int, struct mchar *, int));
  int	(*LayResize) __P((int, int));
  void	(*LayRestore) __P((void));
};

struct layer
{
  struct canvas *l_cvlist;	/* list of canvases displaying layer */
  int	 l_width;
  int	 l_height;
  int    l_x;			/* cursor position */
  int    l_y;
  struct LayFuncs *l_layfn;
  char	*l_data;

  struct layer *l_next;		/* layer stack, should be in data? */
  struct layer *l_bottom;	/* bottom element of layer stack */
  int	 l_blocking;
};

#define Process		(*flayer->l_layfn->LayProcess)
#define Abort		(*flayer->l_layfn->LayAbort)
#define RedisplayLine	(*flayer->l_layfn->LayRedisplayLine)
#define ClearLine	(*flayer->l_layfn->LayClearLine)
#define Rewrite		(*flayer->l_layfn->LayRewrite)
#define Resize		(*flayer->l_layfn->LayResize)
#define Restore		(*flayer->l_layfn->LayRestore)

#define SetCursor()	LGotoPos(flayer, flayer->l_x, flayer->l_y)
#define CanResize(l)	(l->l_layfn->LayResize != DefResize)

/* XXX: AArgh! think again! */

#define LAY_CALL_UP(fn) do				\
	{ 						\
	  struct layer *oldlay = flayer; 		\
	  struct canvas *oldcvlist, *cv;		\
	  debug("LayCallUp\n");				\
	  flayer = flayer->l_next;			\
	  oldcvlist = flayer->l_cvlist;			\
	  debug1("oldcvlist: %x\n", oldcvlist);		\
	  flayer->l_cvlist = oldlay->l_cvlist;		\
	  for (cv = flayer->l_cvlist; cv; cv = cv->c_lnext)	\
		cv->c_layer = flayer;			\
	  fn;						\
	  flayer = oldlay;				\
	  for (cv = flayer->l_cvlist; cv; cv = cv->c_lnext)	\
		cv->c_layer = flayer;			\
	  flayer->l_next->l_cvlist = oldcvlist;		\
	} while(0)

#define LAY_DISPLAYS(l, fn) do				\
	{ 						\
	  struct display *olddisplay = display;		\
	  struct canvas *cv;				\
	  for (display = displays; display; display = display->d_next) \
	    {						\
	      for (cv = D_cvlist; cv; cv = cv->c_next)	\
		if (cv->c_layer == l)			\
		  break;				\
	      if (cv == 0)				\
		continue;				\
	      fn;					\
	    }						\
	  display = olddisplay;				\
	} while(0)

