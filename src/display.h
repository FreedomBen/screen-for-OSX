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


#ifdef MAPKEYS
struct kmap
{
  char seq[8];
  char off[8];
  int  nr;
};

#define KMAP_SEQ ((int)((struct kmap *)0)->seq)
#define KMAP_OFF ((int)((struct kmap *)0)->off)

#define KMAP_KEYS (T_OCAPS-T_CAPS)
#define KMAP_AKEYS (T_OCAPS-T_CURSOR)
#define KMAP_EXT 50

#define KMAP_NOTIMEOUT 0x4000

#endif

struct win;			/* forward declaration */

struct display
{
  struct display *d_next;	/* linked list */
  struct user *d_user;		/* user who owns that display */
  struct LayFuncs *d_layfn;	/* current layer functions */
  struct layer *d_lay;		/* layers on the display */
  struct win *d_fore;		/* pointer to fore window */
  struct win *d_other;		/* pointer to other window */
  char  d_nonblock;		/* don't block when d_obufmax reached */
  char  d_termname[20 + 1];	/* $TERM */
  char	*d_tentry;		/* buffer for tgetstr */
  char	d_tcinited;		/* termcap inited flag */
  int	d_width, d_height;	/* width/height of the screen */
  int	d_defwidth, d_defheight;	/* default width/height of windows */
  int	d_top, d_bot;		/* scrollregion start/end */
  int	d_x, d_y;		/* cursor position */
  struct mchar d_rend;		/* current rendition */
  char	d_atyp;			/* current attribute types */
#ifdef KANJI
  int   d_mbcs;			/* saved char for multibytes charset */
  int   d_kanji;		/* what kanji type the display is */
#endif
  int	d_insert;		/* insert mode flag */
  int	d_keypad;		/* application keypad flag */
  int	d_cursorkeys;		/* application cursorkeys flag */
  int	d_revvid;		/* reverse video */
  int	d_curvis;		/* cursor visibility */
  int	d_hstatus;		/* hardstatus used */
  int	d_lp_missing;		/* last character on bot line missing */
  struct mchar d_lpchar;	/* missing char */
  time_t d_status_time;		/* time of status display */
  char	d_status;		/* is status displayed? */
  char	d_status_bell;		/* is it only a vbell? */
  int	d_status_len;		/* length of status line */
  char *d_status_lastmsg;	/* last displayed message */
  int   d_status_buflen;	/* last message buffer len */
  int	d_status_lastx;		/* position of the cursor */
  int	d_status_lasty;		/*   before status was displayed */
  int	d_ESCseen;		/* Was the last char an ESC (^a) */
  int	d_userpid;		/* pid of attacher */
  char	d_usertty[MAXPATHLEN];	/* tty we are attached to */
  int	d_userfd;		/* fd of the tty */
  struct mode d_OldMode;	/* tty mode when screen was started */
  struct mode d_NewMode;	/* New tty mode */
  int	d_flow;			/* tty's flow control on/off flag*/
  char *d_obuf;			/* output buffer */
  int   d_obuflen;		/* len of buffer */
  int	d_obufmax;		/* len where we are blocking the pty */
  char *d_obufp;		/* pointer in buffer */
  int   d_obuffree;		/* free bytes in buffer */
#ifdef AUTO_NUKE
  int	d_auto_nuke;		/* autonuke flag */
#endif
#ifdef MAPKEYS
  int	d_nseqs;		/* number of valid mappings */
  char *d_seqp;			/* pointer into keymap array */
  int	d_seql;			/* number of parsed chars */
  int	d_seqruns;		/* number of select calls */
  int	d_dontmap;		/* do not map next */
  int	d_mapdefault;		/* do map next to default */
  struct kmap d_kmaps[KMAP_KEYS+KMAP_EXT];	/* keymaps */
#endif
  union	tcu d_tcs[T_N];		/* terminal capabilities */
  char *d_attrtab[NATTR];	/* attrib emulation table */
  char  d_attrtyp[NATTR];	/* attrib group table */
  short	d_dospeed;		/* baudrate of tty */
  char	d_c0_tab[256];		/* conversion for C0 */
  char ***d_xtable;		/* char translation table */
  int	d_UPcost, d_DOcost, d_LEcost, d_NDcost;
  int	d_CRcost, d_IMcost, d_EIcost, d_NLcost;
  int   d_printfd;		/* fd for vt100 print sequence */
#ifdef UTMPOK
  slot_t d_loginslot;		/* offset, where utmp_logintty belongs */
  struct utmp d_utmp_logintty;	/* here the original utmp structure is stored */
# ifdef _SEQUENT_
  char	d_loginhost[100+1];
# endif /* _SEQUENT_ */
#endif
};

#ifdef MULTI
# define DISPLAY(x) display->x
#else
extern struct display TheDisplay;
# define DISPLAY(x) TheDisplay.x
#endif

#define D_user		DISPLAY(d_user)
#define D_username	(DISPLAY(d_user) ? DISPLAY(d_user)->u_name : 0)
#define D_layfn		DISPLAY(d_layfn)
#define D_lay		DISPLAY(d_lay)
#define D_fore		DISPLAY(d_fore)
#define D_other		DISPLAY(d_other)
#define D_nonblock      DISPLAY(d_nonblock)
#define D_termname	DISPLAY(d_termname)
#define D_tentry	DISPLAY(d_tentry)
#define D_tcinited	DISPLAY(d_tcinited)
#define D_width		DISPLAY(d_width)
#define D_height	DISPLAY(d_height)
#define D_defwidth	DISPLAY(d_defwidth)
#define D_defheight	DISPLAY(d_defheight)
#define D_top		DISPLAY(d_top)
#define D_bot		DISPLAY(d_bot)
#define D_x		DISPLAY(d_x)
#define D_y		DISPLAY(d_y)
#define D_rend		DISPLAY(d_rend)
#define D_atyp		DISPLAY(d_atyp)
#define D_mbcs		DISPLAY(d_mbcs)
#define D_kanji		DISPLAY(d_kanji)
#define D_insert	DISPLAY(d_insert)
#define D_keypad	DISPLAY(d_keypad)
#define D_cursorkeys	DISPLAY(d_cursorkeys)
#define D_revvid	DISPLAY(d_revvid)
#define D_curvis	DISPLAY(d_curvis)
#define D_hstatus	DISPLAY(d_hstatus)
#define D_lp_missing	DISPLAY(d_lp_missing)
#define D_lpchar	DISPLAY(d_lpchar)
#define D_status	DISPLAY(d_status)
#define D_status_time	DISPLAY(d_status_time)
#define D_status_bell	DISPLAY(d_status_bell)
#define D_status_len	DISPLAY(d_status_len)
#define D_status_lastmsg	DISPLAY(d_status_lastmsg)
#define D_status_buflen	DISPLAY(d_status_buflen)
#define D_status_lastx	DISPLAY(d_status_lastx)
#define D_status_lasty	DISPLAY(d_status_lasty)
#define D_ESCseen	DISPLAY(d_ESCseen)
#define D_userpid	DISPLAY(d_userpid)
#define D_usertty	DISPLAY(d_usertty)
#define D_userfd	DISPLAY(d_userfd)
#define D_OldMode	DISPLAY(d_OldMode)
#define D_NewMode	DISPLAY(d_NewMode)
#define D_flow		DISPLAY(d_flow)
#define D_obuf		DISPLAY(d_obuf)
#define D_obuflen	DISPLAY(d_obuflen)
#define D_obufmax	DISPLAY(d_obufmax)
#define D_obufp		DISPLAY(d_obufp)
#define D_obuffree	DISPLAY(d_obuffree)
#define D_auto_nuke	DISPLAY(d_auto_nuke)
#define D_nseqs		DISPLAY(d_nseqs)
#define D_seqp		DISPLAY(d_seqp)
#define D_seql		DISPLAY(d_seql)
#define D_seqruns	DISPLAY(d_seqruns)
#define D_dontmap	DISPLAY(d_dontmap)
#define D_mapdefault	DISPLAY(d_mapdefault)
#define D_kmaps		DISPLAY(d_kmaps)
#define D_tcs		DISPLAY(d_tcs)
#define D_attrtab	DISPLAY(d_attrtab)
#define D_attrtyp	DISPLAY(d_attrtyp)
#define D_dospeed	DISPLAY(d_dospeed)
#define D_c0_tab	DISPLAY(d_c0_tab)
#define D_xtable	DISPLAY(d_xtable)
#define D_UPcost	DISPLAY(d_UPcost)
#define D_DOcost	DISPLAY(d_DOcost)
#define D_LEcost	DISPLAY(d_LEcost)
#define D_NDcost	DISPLAY(d_NDcost)
#define D_CRcost	DISPLAY(d_CRcost)
#define D_IMcost	DISPLAY(d_IMcost)
#define D_EIcost	DISPLAY(d_EIcost)
#define D_NLcost	DISPLAY(d_NLcost)
#define D_printfd	DISPLAY(d_printfd)
#define D_loginslot	DISPLAY(d_loginslot)
#define D_utmp_logintty	DISPLAY(d_utmp_logintty)
#define D_loginhost	DISPLAY(d_loginhost)


#define GRAIN 4096  /* Allocation grain size for output buffer */
#define OBUF_MAX 256 /* default for obuflimit */

#define OUTPUT_BLOCK_SIZE 256  /* Block size of output to tty */

#define AddChar(c) do {			\
    if (--D_obuffree == 0)		\
      Resize_obuf();			\
    *D_obufp++ = (c);			\
} while (0)

