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


#undef IFCOLOR

#ifdef COLOR
# define IFCOLOR(x) x
#else
# define IFCOLOR(x)
#endif


struct mchar {
	char image;
	char attr;
	char font;
IFCOLOR(char color;)
};

struct mline {
	char *image;
	char *attr;
	char *font;
IFCOLOR(char *color;)
};



#define save_mline(ml, n) do {					\
	bcopy((ml)->image, mline_old.image, (n));		\
	bcopy((ml)->attr,  mline_old.attr,  (n));		\
	bcopy((ml)->font,  mline_old.font,  (n));		\
IFCOLOR(bcopy((ml)->color, mline_old.color, (n));	       )\
} while (0)

#define bcopy_mline(ml, xf, xt, n) do {				\
	bcopy((ml)->image + (xf), (ml)->image + (xt), (n));	\
	bcopy((ml)->attr  + (xf), (ml)->attr  + (xt), (n));	\
	bcopy((ml)->font  + (xf), (ml)->font  + (xt), (n));	\
IFCOLOR(bcopy((ml)->color + (xf), (ml)->color + (xt), (n));    )\
} while (0)

#define clear_mline(ml, x, n) do {				\
	bclear((ml)->image + (x), (n));				\
	if ((ml)->attr != null) bzero((ml)->attr  + (x), (n));	\
	if ((ml)->font != null) bzero((ml)->font  + (x), (n));	\
IFCOLOR(if ((ml)->color!= null) bzero((ml)->color + (x), (n)); )\
} while (0)

#define cmp_mline(ml1, ml2, x) (				\
	   (ml1)->image[x] == (ml2)->image[x]			\
	&& (ml1)->attr[x]  == (ml2)->attr[x]			\
	&& (ml1)->font[x]  == (ml2)->font[x]			\
IFCOLOR(&& (ml1)->color[x] == (ml2)->color[x]		       )\
)

#define cmp_mchar(mc1, mc2) (					\
	   (mc1)->image == (mc2)->image				\
	&& (mc1)->attr  == (mc2)->attr				\
	&& (mc1)->font  == (mc2)->font				\
IFCOLOR(&& (mc1)->color == (mc2)->color			       )\
)

#define cmp_mchar_mline(mc, ml, x) (				\
	   (mc)->image == (ml)->image[x]			\
	&& (mc)->attr  == (ml)->attr[x]				\
	&& (mc)->font  == (ml)->font[x]				\
IFCOLOR(&& (mc)->color == (ml)->color[x]		       )\
)

#define copy_mchar2mline(mc, ml, x) do {			\
	(ml)->image[x] = (mc)->image;				\
	(ml)->attr[x]  = (mc)->attr;				\
	(ml)->font[x]  = (mc)->font;				\
IFCOLOR((ml)->color[x] = (mc)->color;			       )\
} while (0)

#define copy_mline2mchar(mc, ml, x) do {			\
	(mc)->image = (ml)->image[x];				\
	(mc)->attr  = (ml)->attr[x];				\
	(mc)->font  = (ml)->font[x];				\
IFCOLOR((mc)->color = (ml)->color[x];			       )\
} while (0)

