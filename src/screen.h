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
 * $Id$ FAU
 */

/* screen.h now includes enough to satisfy its own references.
 * only config.h is still needed.
 */

#include <stdio.h>
#include <errno.h>
#if defined(pyr)
extern int errno;
#endif

#ifdef sun
# define getpgrp __getpgrp
# define exit __exit
#endif

#ifdef POSIX
#include <unistd.h>
# ifdef __STDC__
#  include <stdlib.h>
# endif
#endif

#ifdef sun
# undef getpgrp
# undef exit
#endif

#ifdef POSIX
# include <termios.h>
# ifdef hpux
#  include <bsdtty.h>
# endif
#else
# ifdef TERMIO
#  include <termio.h>
# else
#  include <sgtty.h>
# endif /* TERMIO */
#endif /* POSIX */

#ifdef SUIDROOT
#  ifdef LOCKPTY
#	 undef LOCKPTY
#  endif
#endif

#ifndef UTMPOK
#  ifdef USRLIMIT
#	 undef USRLIMIT
#  endif
#endif

#ifndef LOGINDEFAULT
#  define LOGINDEFAULT 0
#endif

#if defined(LOADAV_3DOUBLES) || defined(LOADAV_3LONGS) || defined(LOADAV_4LONGS)
#  define LOADAV
#endif

#ifndef FSCALE
# ifdef MIPS
#  define FSCALE 256            /* MIPS doesn't, and... */
# else
#  define FSCALE 1000.0 	/* Sequent doesn't define FSCALE...grrrr */
# endif	
#endif

#ifndef F_OK
#define F_OK 0
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif

#define MAXPATH 1024

#ifdef SIGVOID
# if defined(ultrix)
#  define sig_t void
# else /* nice compilers: */
   typedef void sig_t;
# endif
#else
   typedef int sig_t;
#endif

#if (!defined(SYSV) && !defined(POSIX)) || defined(sysV68)
typedef int pid_t;
#endif

#if defined(UTMPOK) && defined(_SEQUENT_)
# define GETUTENT
#endif

#ifdef GETUTENT
  typedef char *slot_t;
#else
  typedef int slot_t;
#endif

#if !defined(BSD) && !defined(sequent)
# define index strchr
# define rindex strrchr
#endif

#ifdef SYSV /* jw. */
# define bzero(poi,len) memset(poi,0,len)
# define killpg(pgrp,sig) kill( -(pgrp), sig)
#endif

/* here comes my own Free: jw. */
#define Free(a) {if ((a) == 0) abort(); else free((void *)(a)); (a)=0;}

#define Ctrl(c) ((c)&037)

/* modes for markroutine 
 */
#define PLAIN 0
#define TRICKY 1

/*typedef long off_t; */	/* Someone might need this */

enum state_t 
{
  LIT,				/* Literal input */
  ESC,				/* Start of escape sequence */
  ASTR,				/* Start of control string */
  STRESC,			/* ESC seen in control string */
  CSI,				/* Reading arguments in "CSI Pn ; Pn ; ... ; XXX" */
  PRIN,				/* Printer mode */
  PRINESC,			/* ESC seen in printer mode */
  PRINCSI,			/* CSI seen in printer mode */
  PRIN4			/* CSI 4 seen in printer mode */
};

enum string_t 
{
  NONE,
  DCS,				/* Device control string */
  OSC,				/* Operating system command */
  APC,				/* Application program command */
  PM,				/* Privacy message */
  AKA				/* a.k.a. for current screen */
};

#define MAXSTR		256
#define MAXARGS 	64
#define MSGWAIT 	5

/* 
 * 4 <= IOSIZE <=1000
 * you may try to vary this value. Use low values if your (VMS) system
 * tends to choke when pasting. Use high values if you want to test
 * how many characters your pty's can buffer.
 */
#define IOSIZE		80

/*
 * if a nasty user really wants to try a history of 2000 lines on all 10
 * windows, he will allocate 5 MegaBytes of memory, which is quite enough.
 */
#define MAXHISTHEIGHT 2000
#define DEFAULTHISTHEIGHT 50

#define IMAGE(p,y) p->image[((y) + p->firstline) % p->height]
#define ATTR(p,y) p->attr[((y) + p->firstline) % p->height]
#define FONT(p,y) p->font[((y) + p->firstline) % p->height]

struct win 
{
  int wpid; /* process, that is connected to the other end of ptyfd */
  int ptyfd; /* usually the master side of our pty pair */
  int ttyflag; /* 1 if ptyfd is connected to a user specified tty. */
  int aflag;
  char outbuf[IOSIZE];
  int outlen;
  int autoaka, akapos;
  char cmd[MAXSTR];
  char tty[MAXSTR];
  int args[MAXARGS];
  int NumArgs;
  slot_t slot;
  char **image;
  char **attr;
  char **font;
  int firstline;
  int LocalCharset;
  int charsets[4];
  int ss;
  int active;
  int x, y;
  char LocalAttr;
  int saved;
  int Saved_x, Saved_y;
  char SavedLocalAttr;
  int SavedLocalCharset;
  int SavedCharsets[4];
  int top, bot;
  int wrap;
  int origin;
  int insert;
  int keypad;
  int width, height;	/* width AND height, as we have now resized wins. jw.*/
  int histheight;       /* all histbases are malloced with width * histheight */
  int histidx;          /* 0= < histidx < histheight; where we insert lines */
  char **ihist; 	/* the history buffer  image */
  char **ahist; 	/* attributes */
  char **fhist; 	/* fonts */
  enum state_t state;
  enum string_t StringType;
  char string[MAXSTR];
  char *stringp;
  char *tabs;
  int vbwait;            
  int bell;
  int flow;
  int autoflow;
  int WinLink;
  FILE *logfp;
  int monitor;
  int cursor_invisible;
};

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
  union
    {
      struct
	{
	  int lflag;
	  int aflag;
	  int flowflag;
	  int hheight;  /* size of scrollback buffer */
	  int nargs;
	  char line[MAXPATH];
	  char dir[MAXPATH];
	}
      create;
      struct
	{
	  int apid;
	  char tty[MAXPATH];
	  char password[20];
	  char envterm[MAXPATH];
	}
      attach;
      struct 
	{
	  char password[20];
	  int dpid;
	  char tty[MAXPATH];
	}
      detach;
      char message[MAXPATH*2];
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


struct mode
{
#ifdef TIOCSWINSZ
  struct winsize ws;
#endif
#ifdef POSIX
  struct termios tio;
# ifdef hpux
  struct ltchars m_ltchars;
# endif
#else
# ifdef TERMIO
  struct termio tio;
# else
  struct sgttyb m_ttyb;
  struct tchars m_tchars;
  struct ltchars m_ltchars;
  int m_ldisc;
  int m_lmode;
# endif				/* TERMIO */
#endif				/* POSIX */
};

#define BELL		7

#define BELL_OFF	0 /* No bell has occurred in the window */
#define BELL_ON 	1 /* A bell has occurred, but user not yet notified */
#define BELL_DONE	2 /* A bell has occured, user has been notified */
#define BELL_VISUAL     3 /* A bell has occured in fore win, notify him visually */

#define MON_OFF 	0 /* Monitoring is off in the window */
#define MON_ON		1 /* No activity has occurred in the window */
#define MON_FOUND	2 /* Activity has occured, but user not yet notified */
#define MON_DONE	3 /* Activity has occured, user has been notified */

#define DUMP_TERMCAP	0 /* WriteFile() options */
#define DUMP_HARDCOPY	1
#define DUMP_EXCHANGE	2

#undef MAXWIN20

#ifdef MAXWIN20
#define MAXWIN	20
#else
#define MAXWIN	10
#endif

/* the key definitions are used in screen.c and help.c */
/* keep this list synchronus with the names given in fileio.c */
enum keytype
{
  KEY_IGNORE, /* Keep these first 2 at the start */
  KEY_SCREEN,
  KEY_0,  KEY_1,  KEY_2,  KEY_3,  KEY_4,
  KEY_5,  KEY_6,  KEY_7,  KEY_8,  KEY_9,
#ifdef MAXWIN20
  KEY_10, KEY_11, KEY_12, KEY_13, KEY_14,
  KEY_15, KEY_16, KEY_17, KEY_18, KEY_19,
#endif
  KEY_AKA,
  KEY_AUTOFLOW,
  KEY_CLEAR,
  KEY_COLON,
  KEY_COPY,
  KEY_DETACH,
  KEY_FLOW,
  KEY_HARDCOPY,
  KEY_HELP,
  KEY_HISTORY,
  KEY_INFO,
  KEY_KILL,
  KEY_LASTMSG,
  KEY_LOCK,
  KEY_LOGTOGGLE,
  KEY_LOGIN,
  KEY_MONITOR,
  KEY_NEXT,
  KEY_OTHER,
  KEY_PASTE,
  KEY_POW_DETACH,
  KEY_PREV,
  KEY_QUIT,
  KEY_READ_BUFFER,
  KEY_REDISPLAY,
  KEY_REMOVE_BUFFERS,
  KEY_RESET,
  KEY_SET,
  KEY_SHELL,
  KEY_SUSPEND,
  KEY_TERMCAP,
  KEY_TIME,
  KEY_VBELL,
  KEY_VERSION,
  KEY_WIDTH,
  KEY_WINDOWS,
  KEY_WRAP,
  KEY_WRITE_BUFFER,
  KEY_XOFF,
  KEY_XON,
  KEY_EXTEND,
  KEY_X_WINDOWS,
  KEY_BONUSWINDOW,
  KEY_CREATE,
};

struct key 
{
  enum keytype type;
  char **args;
};

#ifdef NETHACK
#	define Msg_nomem Msg(0, "You feel stupid.")
#else
#	define Msg_nomem Msg(0, "Out of memory.")
#endif

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

#if __STDC__
# define __P(a) a
#else
# define __P(a) ()
# define const
#endif

#if !defined(SYSV) || defined(sun) || defined(RENO) || defined(xelos)
# define BSDWAIT
#endif
