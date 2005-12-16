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
 * $Id$ FAU
 */

/*
 * This is the overlay structure. It is used to create a seperate
 * layer over the current windows.
 */

struct LayFuncs
{
  void	(*LayProcess) __P((char **, int *));
  void	(*LayAbort) __P((void));
  void	(*LayRedisplayLine) __P((int, int, int, int));
  void	(*LayClearLine) __P((int, int, int));
  int	(*LayRewrite) __P((int, int, int, int));
  void	(*LaySetCursor) __P((void));
  int	(*LayResize) __P((int, int));
  void	(*LayRestore) __P((void));
};

struct layer
{
  struct layer *l_next;
  int	l_block;
  struct LayFuncs *l_layfn;
  char	*l_data;		/* should be void * */
};

#define Process		(*D_layfn->LayProcess)
#define Abort		(*D_layfn->LayAbort)
#define RedisplayLine	(*D_layfn->LayRedisplayLine)
#define ClearLine	(*D_layfn->LayClearLine)
#define Rewrite		(*D_layfn->LayRewrite)
#define SetCursor	(*D_layfn->LaySetCursor)
#define Resize		(*D_layfn->LayResize)
#define Restore		(*D_layfn->LayRestore)

#define LAY_CALL_UP(fn) \
	{ \
	  struct layer *oldlay = D_lay; \
	  D_lay = D_lay->l_next; \
	  D_layfn = D_lay->l_layfn; \
	  fn; \
	  D_lay = oldlay; \
	  D_layfn = D_lay->l_layfn; \
	}

