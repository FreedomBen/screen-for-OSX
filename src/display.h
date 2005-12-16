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

struct win;			/* forward declaration */

struct display
{
  struct display *_d_next;	/* linked list */
  struct user *_d_user;		/* user who owns that display */
  struct LayFuncs *_d_layfn;	/* current layer functions */
  struct layer *_d_lay;		/* layers on the display */
  struct win *_d_fore;		/* pointer to fore window */
  struct win *_d_other;		/* pointer to other window */
  char  _d_termname[20 + 1];	/* $TERM */
  char	_d_tentry[TERMCAP_BUFSIZE];	/* buffer for tgetstr */
  int	_d_tcinited;		/* termcap inited flag */
  int	_d_width, _d_height;	/* width/height of the screen */
  int	_d_defwidth, _d_defheight;	/* default width/height of windows */
  int	_d_top, _d_bot;		/* scrollregion start/end */
  int	_d_x, _d_y;		/* cursor position */
  char	_d_attr;		/* current attributes */
  char	_d_font;		/* current font */
  int	_d_insert;		/* insert mode flag */
  int	_d_keypad;		/* application keypad flag */
  int	_d_cursorkeys;		/* application cursorkeys flag */
  int	_d_flow;		/* flow control on/off flag*/
  int	_d_lp_missing;		/* last character on bot line missing */
  int	_d_lp_image;		/* missing image */
  int	_d_lp_attr;		/* missing attr */
  int	_d_lp_font;		/* missing font */
  int	_d_status;		/* is status displayed? */
  time_t _d_status_time;	/* time of status display */
  int	_d_status_bell;		/* is it only a vbell? */
  int	_d_status_len;		/* length of status line */
  char *_d_status_lastmsg;	/* last displayed message */
  int   _d_status_buflen;	/* last message buffer len */
  int	_d_status_lastx;	/* position of the cursor */
  int	_d_status_lasty;	/*   before status was displayed */
  int	_d_ESCseen;		/* Was the last char an ESC (^a) */
  int	_d_userpid;		/* pid of attacher */
  char	_d_usertty[MAXPATHLEN];	/* tty we are attached to */
  int	_d_userfd;		/* fd of the tty */
  struct mode _d_OldMode;	/* tty mode when screen was started */
  struct mode _d_NewMode;	/* New tty mode */
  char  *_d_obuf;		/* output buffer */
  int   _d_obuflen;		/* len of buffer */
  int	_d_obufmax;		/* len where we are blocking the pty */
  char  *_d_obufp;		/* pointer in buffer */
  int   _d_obuffree;		/* free bytes in buffer */
#ifdef AUTO_NUKE
  int	_d_auto_nuke;		/* autonuke flag */
#endif
  union	tcu _d_tcs[T_N];	/* terminal capabilities */
  char	*_d_attrtab[NATTR];
  short	_d_dospeed;		/* baudrate of tty */
  char _d_c0_tab[256];		/* conversion for C0 */
  int _d_UPcost, _d_DOcost, _d_LEcost, _d_NDcost;
  int _d_CRcost, _d_IMcost, _d_EIcost, _d_NLcost;
#ifdef UTMPOK
  slot_t _d_loginslot;		/* offset, where utmp_logintty belongs */
  struct utmp _d_utmp_logintty;	/* here the original utmp structure is stored */
# ifdef _SEQUENT_
  char _d_loginhost[100+1];
# endif /* _SEQUENT_ */
#endif
};

#ifdef MULTI
# define DISPLAY(x) display->x
#else
extern struct display TheDisplay;
# define DISPLAY(x) TheDisplay.x
#endif

#define d_user		DISPLAY(_d_user)
#define d_username	(DISPLAY(_d_user) ? DISPLAY(_d_user)->u_name : 0)
#define d_layfn		DISPLAY(_d_layfn)
#define d_lay		DISPLAY(_d_lay)
#define d_fore		DISPLAY(_d_fore)
#define d_other		DISPLAY(_d_other)
#define d_termname	DISPLAY(_d_termname)
#define d_tentry	DISPLAY(_d_tentry)
#define d_tcinited	DISPLAY(_d_tcinited)
#define d_width		DISPLAY(_d_width)
#define d_height	DISPLAY(_d_height)
#define d_defwidth	DISPLAY(_d_defwidth)
#define d_defheight	DISPLAY(_d_defheight)
#define d_top		DISPLAY(_d_top)
#define d_bot		DISPLAY(_d_bot)
#define d_x		DISPLAY(_d_x)
#define d_y		DISPLAY(_d_y)
#define d_attr		DISPLAY(_d_attr)
#define d_font		DISPLAY(_d_font)
#define d_insert	DISPLAY(_d_insert)
#define d_keypad	DISPLAY(_d_keypad)
#define d_cursorkeys	DISPLAY(_d_cursorkeys)
#define d_flow		DISPLAY(_d_flow)
#define d_lp_missing	DISPLAY(_d_lp_missing)
#define d_lp_image	DISPLAY(_d_lp_image)
#define d_lp_attr	DISPLAY(_d_lp_attr)
#define d_lp_font	DISPLAY(_d_lp_font)
#define d_status	DISPLAY(_d_status)
#define d_status_time	DISPLAY(_d_status_time)
#define d_status_bell	DISPLAY(_d_status_bell)
#define d_status_len	DISPLAY(_d_status_len)
#define d_status_lastmsg	DISPLAY(_d_status_lastmsg)
#define d_status_buflen	DISPLAY(_d_status_buflen)
#define d_status_lastx	DISPLAY(_d_status_lastx)
#define d_status_lasty	DISPLAY(_d_status_lasty)
#define d_ESCseen	DISPLAY(_d_ESCseen)
#define d_userpid	DISPLAY(_d_userpid)
#define d_usertty	DISPLAY(_d_usertty)
#define d_userfd	DISPLAY(_d_userfd)
#define d_OldMode	DISPLAY(_d_OldMode)
#define d_NewMode	DISPLAY(_d_NewMode)
#define d_obuf		DISPLAY(_d_obuf)
#define d_obuflen	DISPLAY(_d_obuflen)
#define d_obufmax	DISPLAY(_d_obufmax)
#define d_obufp		DISPLAY(_d_obufp)
#define d_obuffree	DISPLAY(_d_obuffree)
#define d_auto_nuke	DISPLAY(_d_auto_nuke)
#define d_tcs		DISPLAY(_d_tcs)
#define d_attrtab	DISPLAY(_d_attrtab)
#define d_dospeed	DISPLAY(_d_dospeed)
#define d_c0_tab	DISPLAY(_d_c0_tab)
#define d_UPcost	DISPLAY(_d_UPcost)
#define d_DOcost	DISPLAY(_d_DOcost)
#define d_LEcost	DISPLAY(_d_LEcost)
#define d_NDcost	DISPLAY(_d_NDcost)
#define d_CRcost	DISPLAY(_d_CRcost)
#define d_IMcost	DISPLAY(_d_IMcost)
#define d_EIcost	DISPLAY(_d_EIcost)
#define d_NLcost	DISPLAY(_d_NLcost)
#define d_loginslot	DISPLAY(_d_loginslot)
#define d_utmp_logintty	DISPLAY(_d_utmp_logintty)
#define d_loginhost	DISPLAY(_d_loginhost)


#define GRAIN 4096  /* Allocation grain size for output buffer */
#define OBUF_MAX 256
   /* Maximum amount of buffered output before input is blocked */

#define OUTPUT_BLOCK_SIZE 256  /* Block size of output to tty */

#define AddChar(c) \
  { \
    if (--d_obuffree == 0) \
      Resize_obuf(); \
    *d_obufp++ = (c); \
  }

