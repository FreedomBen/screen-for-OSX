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
 *	Larry W. Virden (lwv27%cas.BITNET@CUNYVM.CUNY.Edu)
 *	Howard Chu (hyc@hanauma.jpl.nasa.gov)
 *	Tim MacKenzie (tym@dibbler.cs.monash.edu.au)
 *	Markku Jarvinen (mta@{cc,cs,ee}.tut.fi)
 *	Marc Boucher (marc@CAM.ORG)
 *
 ****************************************************************
 */

/****************************************************************
 * $Header$ 
 *
 * patchlevel.h: Our life story.
 *   8.7.91 -- 3.00.01 -wipe and a 'setenv TERM dumb' bugfix.
 *  17.7.91 -- 3.00.02 another patchlevel by Wayne Davison
 *  31.7.91 -- 3.00.03 E0, S0, C0 for flexible semi-graphics, nonblocking 
 *                     window title input and 'C-a :' command input.
 *  10.8.91 -- 3.00.04 scrollback, markkeys and some bugfixes.
 *  13.8.91 -- 3.00.05 mark routine improved, ansi prototypes added.
 *  20.8.91 -- 3.00.06 screen -h, faster GotoPos in overlay, termcap %.
 *                     instead of %c
 *  28.8.91 -- 3.00.07 environment variable support. security. terminfo.
 *                     pyramid and ultrix support.
 *  07.9.91 -- 3.00.99 secopen(), MIPS support, SVR4 support.
 *  09.9.91 -- 3.01.00 backspace bug fixed.
 * 03.10.91 -- 3.01.01 ansi.c: null-ptr fixed, CLS now saves to scrollback.
 *                     Using setresuid on hpux. Memory leak fixed.
 *		       Better GotoPos(). Support for IC. Another resize bug.
 *                     Detach() w/o fore crashed. -T and -A(dapt) option.
 *                     GNU copyleft.
 */

#define ORIGIN "FAU"
#define REV 3
#define VERS 1
#define PATCHLEVEL 1
#define DATE "10/10/91"
#define STATE ""

