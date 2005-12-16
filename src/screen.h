/* Copyright (c) 1987-1990 Oliver Laumann and Wayne Davison.
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

#ifdef SUIDROOT
#  ifdef LOCKPTY
#    undef LOCKPTY
#  endif
#endif

#ifndef UTMPOK
#  ifdef USRLIMIT
#    undef USRLIMIT
#  endif
#endif

#ifndef LOGINDEFAULT
#  define LOGINDEFAULT 0
#endif

#if defined(LOADAV_3DOUBLES) || defined(LOADAV_3LONGS) || defined(LOADAV_4LONGS)
#  define LOADAV
#endif

#ifndef FSCALE
#  define FSCALE 1000.0		/* Sequent doesn't define FSCALE...grrrr */
#endif

#if defined(sun) || defined(ultrix)
   typedef void sig_t;
#else
   typedef int sig_t;
#endif

/*typedef long off_t;		/* Someone might need this */

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
    AKA          /* a.k.a. for current screen */
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
    int autoaka, akapos;
    char cmd[MAXSTR];
    char tty[MAXSTR];
    int args[MAXARGS];
    int NumArgs;
    int slot;
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
    int top, bot;
    int wrap;
    int origin;
    int insert;
    int keypad;
    int width;
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
};

#define MAXLINE 1024

#define MSG_CREATE    0
#define MSG_ERROR     1
#define MSG_ATTACH    2
#define MSG_CONT      3

struct msg {
    int type;
    union {
	struct {
	    int lflag;
	    int aflag;
	    int flowflag;
	    int nargs;
	    char line[MAXLINE];
	    char dir[1024];
	} create;
	struct {
	    int apid;
	    char tty[1024];
	} attach;
	char message[MAXLINE];
    } m;
};

#define BELL		7

#define BELL_OFF	0 /* No bell has occurred in the window */
#define BELL_ON		1 /* A bell has occurred, but user not yet notified */
#define BELL_DONE	2 /* A bell has occured, user has been notified */

#define MON_OFF		0 /* Monitoring is off in the window */
#define MON_ON		1 /* No activity has occurred in the window */
#define MON_FOUND	2 /* Activity has occured, but user not yet notified */
#define MON_DONE	3 /* Activity has occured, user has been notified */
