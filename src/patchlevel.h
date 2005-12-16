/* Copyright (c) 1991 Juergen Weigert (jnweiger@immd4.uni-erlangen.de)
 *                    Michael Schroeder (mlschroe@immd4.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
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
 *  07.9.91 -- 3.00.99 secopen(), MIPS support, SVR4 support, backspace bug.
 *  09.9.91 -- 3.01    backspace bug fixed.
 */

#define ORIGIN "FAU"
#define REV 3
#define VERS 1
#define PATCHLEVEL 0
#define DATE "09/09/91"
#define STATE ""

