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
 * $Id$ FAU
 */


/*
 * This is the overlay structure. It is used to create a seperate
 * layer over the current windows.
 */

struct LayFuncs
{
  void	(*LayProcess)();
  void	(*LayAbort)();
  void	(*LayRedisplayLine)();
  void	(*LayClearLine)();
  int	(*LayRewrite)();
  void	(*LaySetCursor)();
  int	(*LayResize)();
  void	(*LayRestore)();
};

struct layer
{
  struct layer *l_next;
  int	l_block;
  struct LayFuncs *l_layfn;
  char	*l_data;		/* should be void * */
};

#define Process		(*d_layfn->LayProcess)
#define Abort		(*d_layfn->LayAbort)
#define RedisplayLine	(*d_layfn->LayRedisplayLine)
#define ClearLine	(*d_layfn->LayClearLine)
#define Rewrite		(*d_layfn->LayRewrite)
#define SetCursor	(*d_layfn->LaySetCursor)
#define Resize		(*d_layfn->LayResize)
#define Restore		(*d_layfn->LayRestore)

#define LAY_CALL_UP(fn) \
	{ \
	  struct layer *oldlay = d_lay; \
	  d_lay = d_lay->l_next; \
	  d_layfn = d_lay->l_layfn; \
	  fn; \
	  d_lay = oldlay; \
	  d_layfn = d_lay->l_layfn; \
	}

