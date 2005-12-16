/* Copyright (c) 1987,1988 Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

#include <sys/types.h>
#include <utmp.h> /* we need it with and without UTMP5 defined */

/*
 * system dependencies definitions
 */

#ifdef SUNOS
#define LOADAV
#define SUNLOADAV
#define KVMLIB
#define GETTTYENT
#endif SUNOS

#if defined(pe3200) || defined(hpux)
#define UTMP5
#endif

#ifdef pe3200
#define LOADAVSYSCALL
#define NEEDNDIR
#endif

#ifdef LOADAVSYSCALL
#if defined(LOADAV)
Error: do not define LOADAV if you have defined LOADAVSYSCALL
#endif
#endif LOADAVSYSCALL

#ifdef sequent

#define KERNELNAME "/dynix"

/*
 * The sequent has problems with binding AF_UNIX sockets
 * to NFS files. So we changed the path to point to a (hopefully)
 * local directory
 */

#ifndef SOCKDIR
#ifndef TMPTEST
#define SOCKDIR "/local/screens" /* /usr/local no more.jw.*/
#else
#define SOCKDIR "/tmp/screens" /* just for testing .jw.*/
#endif
#endif
#endif sequent

#ifdef hpux
#define TCGETA
#define KERNELNAME "/hp-ux"
#define SYSV
#define NAMEDPIPE
#define LOADAV
#define LOCKCMD "/usr/bin/lock"
#define PTYCHAR1 "vutsrqp"	/* start from backward ! */
#endif

/*
 * end of system dependecies declarations
 */

/*
 *	Beginning of User Configuration Section
 */

/*
 *
 * LOADAV    -- your system maintains a load average like 4.3 BSD does
 *              (an array of three doubles called _avenrun; it is read
 *              from /dev/kmem; _avenrun is taken from the namelist of
 *              /vmunix).
 */
/*#undef LOADAV*/

/*
 * SUNLOADAV -- the load average maintained by the kernel is in the
 *              Sun format (three longs).  Set this in addition to
 *              LOADAV.
 */
/* #define SUNLOADAV */

/*
 * GETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
 *              and the getttyent(3) library functions.
 *
 */
/* #undef GETTTYENT */

/*
 * USEBCOPY  -- use the bcopy() from the system's C-library.  If this
 *              is set, bcopy must support overlapping source and
 *              destination.  If USEBCOPY is not set, screen uses its
 *              own version of bcopy.
 *
 */
/* #define USEBCOPY */

/*
 * LOGINDEFAULT -- if set to 1 (one), windows will login (add entries to
 *                 /etc/utmp) by default.  Set to 0 if you don't want this.
 *                 (Also see USERLIMIT below).
 */
#define LOGINDEFAULT 0

/*
 * USERLIMIT    -- count all non-null entries in /etc/utmp before adding a new entry.
 *                 Some machine manufacturers (incorrectly) count logins by
 *                 counting non-null entries in /etc/utmp (instead of counting
 *                 non-null entries with no hostname and not on a pseudo tty).
 *                 Sequent does this, so you might reach your limited user license early.
 */
/* #define USRLIMIT 32 */

/*
 * SOCKDIR      -- If defined, this directory is where screen sockets will be placed,
 *                 (actually in a subdirectory by the user's loginid).  This is neccessary
 *                 because NFS doesn't support socket operations, and many people's homes
 *                 are on NFS mounted partitions.  Screen will create this directory
 *                 if it needs to.
 */
/* #define SOCKDIR "/tmp/screen"	/* NFS doesn't support named sockets */

/*
 *	End of User Configuration Section
 */

#ifndef FSCALE
/* #define FSCALE 1000.0		/* Sequent doesn't define FSCALE...grrrr */
#endif
/* #define MAXWIDTH 132		/* for handling terminals that can switch between 80 and 132 columns */

/* #define MAXROWS  80		/* if you do not expect that screen is run
				   from within a window, this can be 24 */
/* #define MAXCOLS  200		/* also another essumption		*/

#define TERMNAMELEN	32

/* jw has a fable for this kind of free: */
#define Free(a) { free((void *)(a)); (a)=0; }
#define PLAIN 0
#define TRICKY 1

enum state_t {
    LIT,         /* Literal input */
    ESC,         /* Start of escape sequence */
    STR,         /* Start of control string */
    TERM,        /* ESC seen in control string */
    CSI,         /* Reading arguments in "CSI Pn ; Pn ; ... ; XXX" */
    PRIN,        /* Printer mode */
    PRINESC,     /* ESC seen in printer mode */
    PRINCSI,     /* CSI seen in printer mode */
    PRIN4        /* CSI 4 seen in printer mode */
};

enum string_t {
    NONE,
    DCS,         /* Device control string */
    OSC,         /* Operating system command */
    APC,         /* Application program command */
    PM,          /* Privacy message */
};

#define MAXSTR       128
#define	MAXARGS      64

#define IOSIZE       80

struct win {
    int wpid;
    int ptyfd;
    int aflag;
    char outbuf[IOSIZE];
    int outlen;
    char cmd[MAXSTR];
    char tty[MAXSTR];
    int args[MAXARGS];
    char GotArg[MAXARGS]; /* 0 is a col, 1 a row? jw.*/
    int NumArgs;
#ifdef UTMP5
    struct utmp *slot;
#else
    int slot;
#endif UTMP5
    char **image;
    char **attr;
    char **font;
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
    int top, bot; /* top is a col, bot a row? jw */
    int wrap;
    int origin;
    int insert;
    int keypad;
    enum state_t state;
    enum string_t StringType;
    char string[MAXSTR];
    char *stringp;
    char *tabs;
    int vbwait;
    int bell;
    /* the following values are taken from attr structure
     * during creation.
     */
    int rows;
    int cols;
    int lflag;
    int locked;
	int visualbell;
    char term[TERMNAMELEN];
};

#define MAXLINE 1024

#define MSG_CREATE       0
#define MSG_ERROR        1
#define MSG_ATTACH       2
#define MSG_CONT         3
#define MSG_DETACH       4
#define MSG_POW_DETACH   5

/* attributes used during creation of window */

struct attr {
    int rows, cols; 		/* size of window 		     */
    int lflag;			/* if window shall have a utmp entry */
    int locked;			/* no commands allowed from this win.*/
	int visualbell;		/* lets blink, not beep */
    char term[TERMNAMELEN];
	char sname[20];		/* what this window shall be named (^Aw) jw.*/
};


struct msg {
    int type;
    union {
	struct {
	    struct attr attr;
	    int aflag;
	    int nargs;
	    char line[MAXLINE];
	    char dir[1024];
	} create;
	struct {
	    int apid;
	    int appid;
	    char tty[1024];
	    char term[TERMNAMELEN];
	    char passwd[20];
	} attach;
	struct {
	    char passwd[20];
	    int dpid;
	    char tty[1024];
	} detach;
	char message[MAXLINE];
    } m;
};

#ifdef SYSV
#define index strchr
#define rindex strrchr
#define killpg( pgrp, sig) kill( -pgrp, sig)
#endif SYSV

#define A_SO     0x1    /* Standout mode */
#define A_US     0x2    /* Underscore mode */
#define A_BL     0x4    /* Blinking */
#define A_BD     0x8    /* Bold mode */
#define A_DI    0x10    /* Dim mode */
#define A_RV    0x20    /* Reverse mode */
#define A_MAX   0x20


