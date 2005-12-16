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
 */

#define NATTR		6

#define ATTR_DI		0	/* Dim mode */
#define ATTR_US		1	/* Underscore mode */
#define ATTR_BD		2	/* Bold mode */
#define ATTR_RV		3	/* Reverse mode */
#define ATTR_SO		4	/* Standout mode */
#define ATTR_BL		5	/* Blinking */

#define A_DI	(1<<ATTR_DI)
#define A_US	(1<<ATTR_US)
#define A_BD	(1<<ATTR_BD)
#define A_RV	(1<<ATTR_RV)
#define A_SO	(1<<ATTR_SO)
#define A_BL	(1<<ATTR_BL)
#define A_MAX	(1<<(NATTR-1))

/* Types of movement used by Goto() */
enum move_t {
	M_NONE,
	M_UP,
	M_DO,
	M_LE,
	M_RI,
	M_RW,
	M_CR,
};

#define EXPENSIVE	 1000

#define G0			 0
#define G1			 1
#define G2			 2
#define G3			 3

#define ASCII		 0

#ifdef TOPSTAT
#define STATLINE	 (0)
#else
#define STATLINE	 (screenheight-1)
#endif

