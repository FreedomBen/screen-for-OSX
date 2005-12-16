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

#include "os.h"
#include "ansi.h"
#include "comm.h"
#include "overlay.h"
#include "term.h"


#ifdef DEBUG
#	define debug(x) {fprintf(dfp,x);fflush(dfp);}
#	define debug1(x,a) {fprintf(dfp,x,a);fflush(dfp);}
#	define debug2(x,a,b) {fprintf(dfp,x,a,b);fflush(dfp);}
#	define debug3(x,a,b,c) {fprintf(dfp,x,a,b,c);fflush(dfp);}
	extern FILE *dfp;
#else
#	define debug(x) {}
#	define debug1(x,a) {}
#	define debug2(x,a,b) {}
#	define debug3(x,a,b,c) {}
#endif

#if defined(__STDC__)
# ifndef __P
#  define __P(a) a
# endif
#else
# ifndef __P
#  define __P(a) ()
# endif
# define const
#endif


#ifndef NOASSERT
# if defined(__STDC__)
#  define ASSERT(lousy_cpp) {if (!(lousy_cpp)) {debug2("ASSERT("#lousy_cpp")ion failed file %s line %d\n", __FILE__, __LINE__);abort();}}
# else
#  define ASSERT(lousy_cpp) {if (!(lousy_cpp)) {debug2("ASSERT(lousy_cpp)ion failed file %s line %d\n", __FILE__, __LINE__);abort();}}
# endif
#else
# define ASSERT(lousy_cpp) {;}
#endif

/* here comes my own Free: jw. */
#define Free(a) {if ((a) == 0) abort(); else free((void *)(a)); (a)=0;}

#define Ctrl(c) ((c)&037)

#define MAXSTR		256
#define MAXARGS 	64
#define MSGWAIT 	5
#define MSGMINWAIT 	1

/*
 * if a nasty user really wants to try a history of 3000 lines on all 10
 * windows, he will allocate 8 MegaBytes of memory, which is quite enough.
 */
#define MAXHISTHEIGHT		3000
#define DEFAULTHISTHEIGHT	100
#define DEFAULT_BUFFERFILE	"/tmp/screen-exchange"

#define	TTY_FLAG_PLAIN		0x01

struct tty_attr
{
  int flags;			/* a PLAIN tty or a process behind */
};

struct mode
{
#ifdef POSIX
  struct termios tio;
# ifdef hpux
  struct ltchars m_ltchars;
# endif /* hpux */
#else /* POSIX */
# ifdef TERMIO
  struct termio tio;
# else /* TERMIO */
  struct sgttyb m_ttyb;
  struct tchars m_tchars;
  struct ltchars m_ltchars;
  int m_ldisc;
  int m_lmode;
# endif /* TERMIO */
#endif /* POSIX */
};

#include "display.h"
#include "window.h"

/*
 * Parameters for the Detach() routine
 */
#define D_DETACH	0
#define D_STOP		1
#define D_REMOTE	2
#define D_POWER 	3
#define D_REMOTE_POWER	4
#define D_LOCK		5

/*
 * Here are the messages the attacher sends to the backend
 */
#define MSG_CREATE	0
#define MSG_ERROR	1
#define MSG_ATTACH	2
#define MSG_CONT	3
#define MSG_DETACH	4
#define MSG_POW_DETACH	5
#define MSG_WINCH	6
#define MSG_HANGUP	7

struct msg
{
  int type;
  char m_tty[MAXPATH];		/* ttyname */
  union
    {
      struct
	{
	  int lflag;
	  int aflag;
	  int flowflag;
	  int hheight;		/* size of scrollback buffer */
	  int nargs;
	  char line[MAXPATH];
	  char dir[MAXPATH];
	  char screenterm[20]; /* is screen really "screen" ? */
	}
      create;
      struct
	{
	  char auser[20 + 1];	/* username */
	  int apid;		/* pid of frontend */
	  int adaptflag;	/* adapt window size? */
	  int lines, columns;	/* display size */
	  char password[20];
	  char envterm[20 + 1];	/* terminal type */
	}
      attach;
      struct 
	{
	  char duser[20 + 1];	/* username */
	  char password[20];
	  int dpid;		/* pid of frontend */
	}
      detach;
      char message[MAXPATH * 2];
    } m;
};

/*
 * And the signals the attacher receives from the backend
 */
#define SIG_BYE		SIGHUP
#define SIG_POWER_BYE	SIGUSR1
#define SIG_LOCK	SIGUSR2
#define SIG_STOP	SIGTSTP
#define SIG_PW_OK	SIGUSR1
#define SIG_PW_FAIL	SIG_BYE


#define BELL		(Ctrl('g'))
#define VBELLWAIT	1 /* No. of seconds a vbell will be displayed */

#define BELL_OFF	0 /* No bell has occurred in the window */
#define BELL_ON 	1 /* A bell has occurred, but user not yet notified */
#define BELL_MSG	2 /* A bell has occured, user sees a message */
#define BELL_DONE	3 /* A bell has occured, user has been notified */
#define BELL_VISUAL     4 /* A bell has occured in fore win, notify him visually */

#define MON_OFF 	0 /* Monitoring is off in the window */
#define MON_ON		1 /* No activity has occurred in the window */
#define MON_FOUND	2 /* Activity has occured, but user not yet notified */
#define MON_MSG		3 /* Activity has occured, user sees a message */
#define MON_DONE	4 /* Activity has occured, user has been notified */

#define DUMP_TERMCAP	0 /* WriteFile() options */
#define DUMP_HARDCOPY	1
#define DUMP_EXCHANGE	2

extern char strnomem[];


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
  struct pseudowin *pwin;
};

/*
 * line modes used by Input()
 */
#define INP_COOKED	0
#define INP_NOECHO	1
#define INP_RAW		2


#ifdef MULTIUSER
struct acl
{
  struct acl *next;
  char *name;
};
#endif

