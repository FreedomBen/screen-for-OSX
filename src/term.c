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

#include "term.h"

struct term term[T_N] =
{
/* display size */
  { "li", T_NUM  },
  { "co", T_NUM  },

/* terminal types*/
  { "hc", T_FLG  },
  { "os", T_FLG  },
  { "ns", T_FLG  },
/* cursor movement */
  { "cm", T_STR  },
  { "ho", T_STR  },
  { "cr", T_STR  },
  { "up", T_STR  },
  { "UP", T_STR  },
  { "do", T_STR  },
  { "DO", T_STR  },
  { "bs", T_FLG  },
  { "bc", T_STR  },
  { "le", T_STR  },
  { "LE", T_STR  },
  { "nd", T_STR  },
  { "RI", T_STR  },

/* scroll */
  { "cs", T_STR  },
  { "nl", T_STR  },
  { "sf", T_STR  },
  { "sr", T_STR  },
  { "al", T_STR  },
  { "AL", T_STR  },
  { "dl", T_STR  },
  { "DL", T_STR  },

/* insert/delete */
  { "in", T_FLG  },
  { "im", T_STR  },
  { "ei", T_STR  },
  { "ic", T_STR  },
  { "IC", T_STR  },
  { "dc", T_STR  },
  { "DC", T_STR  },

/* erase */
  { "cl", T_STR  },
  { "cd", T_STR  },
  { "ce", T_STR  },
  { "cb", T_STR  },

/* initialise */
  { "is", T_STR  },
  { "ti", T_STR  },
  { "te", T_STR  },

/* bell */
  { "bl", T_STR  },
  { "vb", T_STR  },

/* resizing */
  { "WS", T_STR  },
  { "Z0", T_STR  },
  { "Z1", T_STR  },

/* attributes */
/* define T_ATTR */
  { "mh", T_STR  },
  { "us", T_STR  },
  { "md", T_STR  },
  { "mr", T_STR  },
  { "so", T_STR  },
  { "mb", T_STR  },
  { "ue", T_STR  },
  { "se", T_STR  },
  { "me", T_STR  },
  { "ms", T_FLG  },
  { "sg", T_FLG  },
  { "ug", T_FLG  },

/* keypad/cursorkeys */
  { "ks", T_STR  },
  { "ke", T_STR  },
  { "CS", T_STR  },
  { "CE", T_STR  },

/* printer */
  { "po", T_STR  },
  { "pf", T_STR  },

/* status line */
  { "hs", T_FLG  },
  { "ws", T_NUM  },
  { "ts", T_STR  },
  { "fs", T_STR  },
  { "ds", T_STR  },

/* cursor visibility */
  { "vi", T_STR  },
  { "ve", T_STR  },
  { "vs", T_STR  },

/* margin handling */
  { "am", T_FLG  },
  { "xv", T_FLG  },
  { "xn", T_FLG  },
  { "OP", T_FLG  },
  { "LP", T_FLG  },

/* special settings */
  { "NF", T_FLG  },
  { "xo", T_FLG  },
  { "AN", T_FLG  },
  { "OL", T_NUM  },

/* d_font setting */
  { "G0", T_FLG  },
  { "S0", T_STR  },
  { "E0", T_STR  },
  { "C0", T_STR  },
  { "as", T_STR  },
  { "ae", T_STR  },
  { "ac", T_STR  },
  { "B8", T_STR  },

/* keycaps */
/* define T_CAPS */
/* nolist */
  { "km", T_FLG  },
  { "k0", T_STR  },
  { "k1", T_STR  },
  { "k2", T_STR  },
  { "k3", T_STR  },
  { "k4", T_STR  },
  { "k5", T_STR  },
  { "k6", T_STR  },
  { "k7", T_STR  },
  { "k8", T_STR  },
  { "k9", T_STR  },
  { "k;", T_STR  },
  { "kb", T_STR  },
  { "kd", T_STR  },
  { "kh", T_STR  },
  { "kl", T_STR  },
  { "ko", T_STR  },
  { "kr", T_STR  },
  { "ku", T_STR  },
  { "K1", T_STR  },
  { "K2", T_STR  },
  { "K3", T_STR  },
  { "K4", T_STR  },
  { "K5", T_STR  },
  { "l0", T_STR  },
  { "l1", T_STR  },
  { "l2", T_STR  },
  { "l3", T_STR  },
  { "l4", T_STR  },
  { "l5", T_STR  },
  { "l6", T_STR  },
  { "l7", T_STR  },
  { "l8", T_STR  },
  { "l9", T_STR  },
  { "la", T_STR  },
/* more keys for Andrew A. Chernov (ache@astral.msk.su) */
  { "kA", T_STR  },
  { "ka", T_STR  },
  { "kC", T_STR  },
  { "kD", T_STR  },
  { "kE", T_STR  },
  { "kF", T_STR  },
  { "kH", T_STR  },
  { "kI", T_STR  },
  { "kL", T_STR  },
  { "kM", T_STR  },
  { "kN", T_STR  },
  { "kP", T_STR  },
  { "kR", T_STR  },
  { "kS", T_STR  },
  { "kT", T_STR  },
  { "kt", T_STR  },
/* list */
/* define T_ECAPS */
/* define T_N */
};
