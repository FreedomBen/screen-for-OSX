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
 * rcs.h
 *
 * jw 9.2.92
 **************************************************
 * RCS_ID("$Id$ FAU");
 */

#ifndef __RCS_H__
# define __RCS_H__

# if !defined(lint)
#  ifdef __GNUC__
#   define RCS_ID(id) static char *rcs_id() { return rcs_id(id); }
#  else
#   define RCS_ID(id) static char *rcs_id = id;
#  endif /* !__GNUC__ */
# else
#  define RCS_ID(id)      /* Nothing */
# endif /* !lint */

#endif /* __RCS_H__ */
