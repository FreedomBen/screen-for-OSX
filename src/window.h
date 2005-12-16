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

#ifndef MAXWIN
# define MAXWIN	10
#endif


struct NewWindow
{
  int	StartAt;	/* where to start the search for the slot */
  char	*aka;		/* aka string */
  char	**args;		/* argv vector */
  char	*dir;		/* directory for chdir */
  char	*term;		/* TERM to be set instead of "screen" */
  int	aflag;
  int	flowflag;
  int	lflag;
  int	histheight;
  int	monitor;
  int   wlock;		/* default writelock setting */
  int   wrap;
  int   c1;
  int   gr;
  int   kanji;
};


#ifdef PSEUDOS

struct pseudowin
{
  int fdpat;
  int p_pid;
  int p_ptyfd;
  char p_cmd[MAXSTR];
  char p_tty[MAXSTR];
  char p_inbuf[IOSIZE];		/* buffered writing to p_ptyfd */
  int p_inlen;
};

/* bits for fdpat: */
#define F_PMASK 	0x0003
#define F_PSHIFT	2
#define F_PFRONT	0x0001			/* . */
#define F_PBACK 	0x0002			/* ! */
#define F_PBOTH 	(F_PFRONT | F_PBACK)	/* : */

#define F_UWP		0x1000			/* | */

/* The screen process ...)
 * ... wants to write to pseudo */
#define W_WP(w) ((w)->w_pwin && ((w)->w_pwin->fdpat & F_PFRONT))

/* ... wants to write to window: user writes to window 
 * or stdout/stderr of pseudo are duplicated to window */
#define W_WW(w) (!((w)->w_pwin) || \
(((w)->w_pwin->fdpat & F_PMASK) == F_PBACK) || \
((((w)->w_pwin->fdpat >> F_PSHIFT) & F_PMASK) == F_PBOTH) || \
((((w)->w_pwin->fdpat >> (F_PSHIFT * 2)) & F_PMASK) == F_PBOTH))

/* ... wants to read from pseudowin */
#define W_RP(w) ((w)->w_pwin && ((w)->w_pwin->fdpat & \
((F_PFRONT << (F_PSHIFT * 2)) | (F_PFRONT << F_PSHIFT)) ))

/* ... wants to read from window */
#define W_RW(w) (!((w)->w_pwin) || ((w)->w_pwin->fdpat & F_PFRONT))

/* user input is written to pseudo */
#define W_UWP(w) ((w)->w_pwin && ((w)->w_pwin->fdpat & F_UWP))

/* pseudo output has to be stuffed in window */
#define W_PTOW(w) (\
((w)->w_pwin->fdpat & F_PMASK << F_PSHIFT) == F_PBOTH << F_PSHIFT || \
((w)->w_pwin->fdpat & F_PMASK << F_PSHIFT * 2) == F_PBOTH << F_PSHIFT * 2 )

/* window output has to be stuffed in pseudo */
#define W_WTOP(w) (((w)->w_pwin->fdpat & F_PMASK) == F_PBOTH)

#endif /* PSEUDOS */


struct displaylist
{
  struct displaylist *next;
  struct display *d;
};

#define w_display w_dlist->d


struct win 
{
  struct win *w_next;		/* next window */
#ifdef PSEUDOS
  struct pseudowin *w_pwin;	/* ptr to pseudo */
#endif
  struct displaylist *w_dlist;	/* pointer to our display */
  struct display *w_pdisplay;	/* display for printer relay */
  int	 w_number;		/* window number */
  int	 w_active;		/* is window fore and has no layer? */
  struct layer *w_lay;		/* the layer of the window */
  struct layer w_winlay;	/* the layer of the window */
  int	 w_pid;			/* process at the other end of ptyfd */	
  char  *w_cmdargs[MAXARGS];	/* command line argument vector */
  int	 w_ptyfd;		/* fd of the master pty */
  int	 w_aflag;		/* (used for DUMP_TERMCAP) */
  char	 w_inbuf[IOSIZE];
  int	 w_inlen;
  char	 w_outbuf[IOSIZE];
  int	 w_outlen;
  char  *w_title;		/* name of the window */
  char  *w_akachange;		/* autoaka hack */
  int	 w_autoaka;		/* autoaka hack */
  char	 w_akabuf[MAXSTR];	/* aka buffer */
  char	 w_tty[MAXSTR];
  struct tty_attr w_t;
  int	 w_intermediate;	/* char used while parsing ESC-seq */
  int	 w_args[MAXARGS];
  int	 w_NumArgs;
  slot_t w_slot;		/* utmp slot */
#if defined (UTMPOK)
  struct utmp w_savut;		/* utmp entry of this window */
#endif
  char **w_image;
  char **w_attr;
  char **w_font;
  int	 w_x, w_y;		/* Cursor position */
  int	 w_width, w_height;	/* window size */
  char	 w_Attr;		/* character attributes */
  char	 w_Font;		/* character font GL */
  char	 w_FontR;		/* character font GR */
  int	 w_Charset;		/* charset number GL */
  int	 w_CharsetR;		/* charset number GR */
  int	 w_charsets[4];		/* Font = charsets[Charset] */
  int	 w_ss;		
  int	 w_saved;
  int	 w_Saved_x, w_Saved_y;
  char	 w_SavedAttr;
  int	 w_SavedCharset;
  int	 w_SavedCharsetR;
  int	 w_SavedCharsets[4];
  int	 w_top, w_bot;		/* scrollregion */
  int	 w_wrap;		/* autowrap */
  int	 w_origin;		/* origin mode */
  int	 w_insert;		/* window is in insert mode */
  int	 w_keypad;		/* keypad mode */
  int	 w_cursorkeys;		/* appl. cursorkeys mode */
  int	 w_revvid;		/* reverse video */
  int	 w_curinv;		/* cursor invisible */
  int	 w_autolf;		/* automatic linefeed */
  char  *w_hstatus;		/* hardstatus line */
#ifdef COPY_PASTE
  char	*w_pastebuf;		/* this gets pasted in the window */
  char	*w_pasteptr;		/* pointer in pastebuf */
  int	 w_pastelen;		/* bytes left to paste */
  int	 w_histheight;		/* all histbases are malloced with width * histheight */
  int	 w_histidx;		/* 0 <= histidx < histheight; where we insert lines */
  char **w_ihist; 		/* the history buffer image */
  char **w_ahist; 		/* attributes */
  char **w_fhist; 		/* fonts */
#endif
  enum state_t w_state;		/* parser state */
  enum string_t w_StringType;
  int	 w_gr;			/* enable GR flag */
  int	 w_c1;			/* enable C1 flag */
#ifdef KANJI
  int    w_kanji;		/* for input and paste */
  int    w_mbcs;		/* saved char for multibytes charset */
#endif
  char	 w_string[MAXSTR];
  char	*w_stringp;
  char	*w_tabs;		/* line with tabs */
  int	 w_bell;		/* bell status of this window */
  int	 w_flow;		/* flow flags */
  FILE	*w_logfp;		/* log to file */
  int	 w_monitor;		/* monitor status */
  struct lastio_s
    {
      time_t lastio;		/* timestamp of last filedescriptor activity */
      int seconds;		/* tell us when lastio + seconds < time() */
    } w_tstamp;
  char	 w_norefresh;		/* dont redisplay when switching to that win */
  char   w_wlock;		/* WLOCK_AUTO, WLOCK_OFF, WLOCK_ON */
  struct user *w_wlockuser;	/* NULL when unlocked or user who writes */
#ifdef MULTIUSER
  AclBits w_userbits[ACL_BITS_PER_WIN];
#endif
};

/* definitions for wlocktype */
#define WLOCK_OFF	0	/* all who are in w_userbits can write */
#define WLOCK_AUTO	1	/* who selects first, can write */
#define WLOCK_ON	2	/* w_wlockuser writes even if deselected */

/*
 * Definitions for flow
 *   000  -(-)
 *   001  +(-)
 *   010  -(+)
 *   011  +(+)
 *   100  -(a)
 *   111  +(a)
 */
#define FLOW_NOW	(1<<0)
#define FLOW_AUTO	(1<<1)
#define FLOW_AUTOFLAG	(1<<2)


/*
 * iWIN gives us a reference to line y of the *whole* image
 * where line 0 is the oldest line in our history.
 * y must be in WIN coordinate system, not in display.
 */
#define iWIN(y) ((y < fore->w_histheight) ? fore->w_ihist[(fore->w_histidx + y)\
                % fore->w_histheight] : fore->w_image[y - fore->w_histheight])
#define aWIN(y) ((y < fore->w_histheight) ? fore->w_ahist[(fore->w_histidx + y)\
                % fore->w_histheight] : fore->w_attr[y - fore->w_histheight])
#define fWIN(y) ((y < fore->w_histheight) ? fore->w_fhist[(fore->w_histidx + y)\
                % fore->w_histheight] : fore->w_font[y - fore->w_histheight])

