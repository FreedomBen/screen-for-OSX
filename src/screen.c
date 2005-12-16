/* Copyright (c) 1987, Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

static char ScreenVersion[] = "screen 1.1i  1.7 88/12/13";

#ifdef SUNOS
#define KVMLIB
#endif SUNOS

#include <stdio.h>
#include <sgtty.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <utmp.h>
#include <pwd.h>
#include <nlist.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "screen.h"
#ifdef KVMLIB
#include <kvm.h>
#include <sys/param.h>
#endif KVMLIB

#ifdef GETTTYENT
#   include <ttyent.h>
#else
    static struct ttyent {
	char *ty_name;
    } *getttyent();
    static char *tt, *ttnext;
    static char ttys[] = "/etc/ttys";
#endif

#define MAXWIN     10
#define MAXARGS    64
#define MAXLINE  1024
#define MSGWAIT     5

#define Ctrl(c) ((c)&037)

extern char *blank, Term[], **environ;
extern rows, cols;
extern status;
extern time_t TimeDisplayed;
extern char AnsiVersion[];
extern short ospeed;
extern flowctl;
extern errno;
extern sys_nerr;
extern char *sys_errlist[];
extern char *index(), *rindex(), *malloc(), *getenv(), *MakeTermcap();
extern char *getlogin(), *ttyname();
extern struct passwd *getpwuid();
#ifdef pe3200
extern struct utmp *SetUtmp();
#endif /* pe3200 */
static Finit(), SigChld();
static char *MakeBellMsg(), *Filename(), **SaveArgs();
#ifdef LOCK
static void LockTerminal();
static char LockEnd[] = "Welcome back to screen !!";
#endif LOCK

static char PtyName[32], TtyName[32];
static char *ShellProg;
static char *ShellArgs[2];
static char inbuf[IOSIZE];
static inlen;
static ESCseen;
static GotSignal;
#ifdef LOCK
static char DefaultLock[] = "/usr/local/bin/lck";
#endif LOCK
static char DefaultShell[] = "/bin/sh";
#ifdef pe3200
static char DefaultPath[] = ":/usr/local/bin:/bin:/usr/bin";
#else
static char DefaultPath[] = ":/usr/ucb:/bin:/usr/bin";
#endif
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
#ifdef SEQUENT
#define PTYCHAR1 "p"
#define PTYCHAR2 "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#endif SEQUENT
#ifndef PTYCHAR1
#define PTYCHAR1 "pqrst"
#endif PTYCHAR1
#ifndef PTYCHAR2
#define PTYCHAR2 "01213456789abcdef"
#endif PTYCHAR2
static char SockPath[512];
static char SockDir[] = ".screen";
static char *SockName;
static char *NewEnv[MAXARGS];
static char Esc = Ctrl('a');
static char MetaEsc = 'a';
static char *home;
static HasWindow;
static utmp;
#ifndef pe3200
static utmpf;
static char UtmpName[] = "/etc/utmp";
#endif
static char *LoginName;
static char *BellString = "Bell in window %";
static mflag, nflag, fflag;
static char HostName[MAXSTR];
#ifdef LOADAV
#ifdef SEQUENT
    static char AvenrunSym[] = "_loadav";
#else SEQUENT
    static char AvenrunSym[] = "_avenrun";
#endif
    static struct nlist nl[2];
    static avok;
#ifdef KVMLIB
    static kvm_t *kmemf;
#else
    static char KmemName[] = "/dev/kmem";
#ifdef SEQUENT
    static char UnixName[] = "/dynix";
#else SEQUENT
    static char UnixName[] = "/vmunix";
#endif SEQUENT
    static kmemf;
#endif KVMLIB
#ifndef vax
#define AVvar(X) long X[3]
#ifndef FSCALE
#define FSCALE 1
#endif FSCALE
#define AVfmt(X) (((double)X)/FSCALE)
#else
#define AVvar(X) double X[3]
#define AVfmt(X) X
#endif vax
#endif LOADAV

struct mode {
    struct sgttyb m_ttyb;
    struct tchars m_tchars;
    struct ltchars m_ltchars;
    int m_ldisc;
    int m_lmode;
} OldMode, NewMode;

static struct win *curr, *other;
static CurrNum, OtherNum;
static struct win *wtab[MAXWIN];

#define MSG_CREATE    0
#define MSG_ERROR     1

struct msg {
    int type;
    union {
	struct {
	    int aflag;
	    int nargs;
	    char line[MAXLINE];
	    char dir[1024];
	} create;
	char message[MAXLINE];
    } m;
};

#define KEY_IGNORE         0
#define KEY_HARDCOPY       1
#define KEY_SUSPEND        2
#define KEY_SHELL          3
#define KEY_NEXT           4
#define KEY_PREV           5
#define KEY_KILL           6
#define KEY_REDISPLAY      7
#define KEY_WINDOWS        8
#define KEY_VERSION        9
#define KEY_OTHER         10
#define KEY_0             11
#define KEY_1             12
#define KEY_2             13
#define KEY_3             14
#define KEY_4             15
#define KEY_5             16
#define KEY_6             17
#define KEY_7             18
#define KEY_8             19
#define KEY_9             20
#define KEY_XON           21
#define KEY_XOFF          22
#define KEY_INFO          23
#define KEY_TERMCAP       24
#define KEY_QUIT          25
#ifdef LOCK
#define KEY_LOCK	  26
#define KEY_CREATE        27
#else  LOCK
#define KEY_CREATE	  26
#endif LOCK

struct key {
    int type;
    char **args;
} ktab[256];

char *KeyNames[] = {
    "hardcopy", "suspend", "shell", "next", "prev", "kill", "redisplay",
    "windows", "version", "other", "select0", "select1", "select2", "select3",
    "select4", "select5", "select6", "select7", "select8", "select9",
    "xon", "xoff", "info", "termcap", "quit",
#ifdef LOCK
    "lock",
#endif LOCK
    0
};

main (ac, av) char **av; {
    register n, len;
    register struct win **pp, *p;
    char *ap;
    int s, r, w, x = 0;
    int aflag = 0;
    struct timeval tv;
    time_t now;
    char buf[IOSIZE], *myname = (ac == 0) ? "??screen??" : av[0];
    char rc[256];

    while (ac > 0) {
	ap = *++av;
	if (--ac > 0 && *ap == '-') {
	    switch (ap[1]) {
	    case 'c':        /* Compatibility with older versions. */
		break;
	    case 'a':
		aflag = 1;
		break;
	    case 'm':
		mflag = 1;
		break;
	    case 'n':
		nflag = 1;
		break;
	    case 'f':
		fflag = 1;
		break;
	    case 'e':
		if (ap[2]) {
		    ap += 2;
		} else {
		    if (--ac == 0) goto help;
		    ap = *++av;
		}
		if (strlen (ap) != 2)
		    Msg (0, "Two characters are required with -e option.");
		Esc = ap[0];
		MetaEsc = ap[1];
		break;
	    default:
	    help:
		Msg (0, "Use: %s [-a] [-f] [-n] [-exy] [cmd args]", myname);
	    }
	} else break;
    }
    if (nflag && fflag)
	Msg (0, "-f and -n are conflicting options.");
    if ((ShellProg = getenv ("SHELL")) == 0)
	ShellProg = DefaultShell;
    ShellArgs[0] = ShellProg;
    if (ac == 0) {
	ac = 1;
	av = ShellArgs;
    }
    if (GetSockName ()) {
	/* Client */
	s = MakeClientSocket ();
	SendCreateMsg (s, ac, av, aflag);
	close (s);
	exit (0);
    }
    (void) gethostname (HostName, MAXSTR);
    HostName[MAXSTR-1] = '\0';
    if (ap = index (HostName, '.'))
	*ap = '\0';
    s = MakeServerSocket ();
    InitTerm ();
    if (fflag)
	flowctl = 1;
    else if (nflag)
	flowctl = 0;
    MakeNewEnv ();
    GetTTY (0, &OldMode);
    ospeed = (short)OldMode.m_ttyb.sg_ospeed;
    InitUtmp ();
#ifdef LOADAV
    InitKmem ();
#endif
    signal (SIGHUP, Finit);
    signal (SIGINT, Finit);
    signal (SIGQUIT, Finit);
    signal (SIGTERM, Finit);
    InitKeytab ();
    sprintf (rc, "%.*s/.screenrc", 245, home);
    signal (SIGCHLD, SigChld);
    ReadRc (rc);
    if ((n = MakeWindow (*av, av, aflag, 0, (char *)0)) == -1) {
	SetTTY (0, &OldMode);
	FinitTerm ();
	exit (1);
    }
    SetCurrWindow (n);
    HasWindow = 1;
    SetMode (&OldMode, &NewMode);
    SetTTY (0, &NewMode);
    tv.tv_usec = 0;
    while (1) {
#ifdef EBUG
fprintf(stderr,"DEBUG: next while.  ");
#endif * EBUG */
	if (status) {
	    time (&now);
	    if (now - TimeDisplayed < MSGWAIT) {
		tv.tv_sec = MSGWAIT - (now - TimeDisplayed);
	    } else RemoveStatus (curr);
	}
	r = 0;
	w = 0;
	if (inlen)
	    w |= 1 << curr->ptyfd;
	else
	    r |= 1 << 0;
	for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
	    if (!(p = *pp))
		continue;
	    if ((*pp)->active && status)
		continue;
	    if ((*pp)->outlen > 0)
		continue;
	    r |= 1 << (*pp)->ptyfd;
	}
	r |= 1 << s;
	fflush (stdout);
#ifdef EBUG
fprintf(stderr,"DEBUG: selecting..  ");
#endif * EBUG */
	if (select (32, &r, &w, &x, status ? &tv : (struct timeval *)0) == -1) {
	    if (errno == EINTR)
		continue;
	    HasWindow = 0;
	    Msg (errno, "select");
	    /*NOTREACHED*/
	}
#ifdef EBUG
fprintf(stderr,"DEBUG: select DONE. ");
#endif * EBUG */
	if (GotSignal && !status) {
	    SigHandler ();
#ifdef EBUG
fprintf(stderr,"DEBUG: handle sigs  ");
#endif * EBUG */
	    continue;
	}
	if (r & 1 << s) {
	    RemoveStatus (curr);
	    ReceiveMsg (s);
	}
	if (r & 1 << 0) {
	    RemoveStatus (curr);
	    if (ESCseen) {
		inbuf[0] = Esc;
#ifdef EBUG
fprintf(stderr,"DEBUG: read(ESC)    ");
#endif * EBUG */
		inlen = read (0, inbuf+1, IOSIZE-1) + 1;
		ESCseen = 0;
	    } else {
#ifdef EBUG
fprintf(stderr,"DEBUG: read(nonESC) ");
#endif * EBUG */
		inlen = read (0, inbuf, IOSIZE);
	    }
	    if (inlen > 0)
		inlen = ProcessInput (inbuf, inlen);
#ifdef EBUG
fprintf(stderr,"DEBUG: Input DONE. \n");
#endif * EBUG */
	    if (inlen > 0)
		continue;
	}
	if (GotSignal && !status) {
	    SigHandler ();
#ifdef EBUG
fprintf(stderr,"DEBUG: handle sigs2 ");
#endif * EBUG */
	    continue;
	}
	if (w & 1 << curr->ptyfd && inlen > 0) {
	    if ((len = write (curr->ptyfd, inbuf, inlen)) > 0) {
		inlen -= len;
		bcopy (inbuf+len, inbuf, inlen);
	    }
	}
	if (GotSignal && !status) {
	    SigHandler ();
#ifdef EBUG
fprintf(stderr,"DEBUG: handle sigs3");
#endif * EBUG */
	    continue;
	}
	for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
	    if (!(p = *pp))
		continue;
	    if (p->outlen) {
		WriteString (p, p->outbuf, p->outlen);
	    } else if (r & 1 << p->ptyfd) {
		if ((len = read (p->ptyfd, buf, IOSIZE)) == -1) {
		    if (errno == EWOULDBLOCK)
			len = 0;
		}
		if (len > 0)
		    WriteString (p, buf, len);
	    }
	    if (p->bell) {
		p->bell = 0;
		Msg (0, MakeBellMsg (pp-wtab));
	    }
	}
	if (GotSignal && !status)
	    SigHandler ();
    }
    /*NOTREACHED*/
}

static SigHandler () {
    while (GotSignal) {
	GotSignal = 0;
	DoWait ();
    }
}

static SigChld () {
    GotSignal = 1;
}

static DoWait () {
    register pid;
    register struct win **pp;
    union wait wstat;

    while ((pid = wait3 (&wstat, WNOHANG|WUNTRACED, NULL)) > 0) {
	for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
	    if (*pp && pid == (*pp)->wpid) {
		if (WIFSTOPPED (wstat)) {
		    kill((*pp)->wpid, SIGCONT);
		} else {
		    if (*pp == curr)
			curr = 0;
		    if (*pp == other)
			other = 0;
		    FreeWindow (*pp);
		    *pp = 0;
		}
	    }
	}
    }
    CheckWindows ();
}

static CheckWindows () {
    register struct win **pp;

    /* If the current window disappeared and the "other" window is still
     * there, switch to the "other" window, else switch to the window
     * with the lowest index.
     * If there current window is still there, but the "other" window
     * vanished, "SetCurrWindow" is called in order to assign a new value
     * to "other".
     * If no window is alive at all, exit.
     */
    if (!curr && other) {
	SwitchWindow (OtherNum);
	return;
    }
    if (curr && !other) {
	SetCurrWindow (CurrNum);
	return;
    }
    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
	if (*pp) {
	    if (!curr)
		SwitchWindow (pp-wtab);
	    return;
	}
    }
    Finit ();
}

static Finit () {
    register struct win *p, **pp;

    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
	if (p = *pp)
	    FreeWindow (p);
    }
    SetTTY (0, &OldMode);
    FinitTerm ();
    printf ("[screen is terminating]\n");
    exit (0);
}

static InitKeytab () {
    register i;

    ktab['h'].type = ktab[Ctrl('h')].type = KEY_HARDCOPY;
    ktab['z'].type = ktab[Ctrl('z')].type = KEY_SUSPEND;
    ktab['c'].type = ktab[Ctrl('c')].type = KEY_SHELL;
    ktab[' '].type = ktab[Ctrl(' ')].type = 
    ktab['n'].type = ktab[Ctrl('n')].type = KEY_NEXT;
    ktab['-'].type = ktab['p'].type = ktab[Ctrl('p')].type = KEY_PREV;
    ktab['k'].type = ktab[Ctrl('k')].type = KEY_KILL;
    ktab['l'].type = ktab[Ctrl('l')].type = KEY_REDISPLAY;
    ktab['w'].type = ktab[Ctrl('w')].type = KEY_WINDOWS;
    ktab['v'].type = ktab[Ctrl('v')].type = KEY_VERSION;
    ktab['q'].type = ktab[Ctrl('q')].type = KEY_XON;
    ktab['s'].type = ktab[Ctrl('s')].type = KEY_XOFF;
    ktab['t'].type = ktab[Ctrl('t')].type = KEY_INFO;
#ifdef LOCK
    ktab['x'].type = ktab[Ctrl('x')].type = KEY_LOCK;
#endif LOCK
    ktab['.'].type = KEY_TERMCAP;
    ktab[Ctrl('\\')].type = KEY_QUIT;
    ktab[Esc].type = KEY_OTHER;
    for (i = 0; i <= 9; i++)
	ktab[i+'0'].type = KEY_0+i;
}

static ProcessInput (buf, len) char *buf; {
    register n, k;
    register char *s, *p;
    register struct win **pp;

    for (s = p = buf; len > 0; len--, s++) {
	if (*s == Esc) {
	    if (len > 1) {
		len--; s++;
		k = ktab[*s].type;
		if (*s == MetaEsc) {
		    *p++ = Esc;
		} else if (k >= KEY_0 && k <= KEY_9) {
		    p = buf;
		    SwitchWindow (k - KEY_0);
		} else switch (ktab[*s].type) {
		case KEY_TERMCAP:
		    p = buf;
		    WriteFile (0);
		    break;
		case KEY_HARDCOPY:
		    p = buf;
		    WriteFile (1);
		    break;
		case KEY_SUSPEND:
		    p = buf;
		    SetTTY (0, &OldMode);
		    FinitTerm ();
		    kill (getpid (), SIGTSTP);
		    SetTTY (0, &NewMode);
		    Activate (wtab[CurrNum]);
		    break;
		case KEY_SHELL:
		    p = buf;
		    if ((n = MakeWindow (ShellProg, ShellArgs,
			    0, 0, (char *)0)) != -1)
			SwitchWindow (n);
		    break;
		case KEY_NEXT:
		    p = buf;
		    if (MoreWindows ())
			SwitchWindow (NextWindow ());
		    break;
		case KEY_PREV:
		    p = buf;
		    if (MoreWindows ())
			SwitchWindow (PreviousWindow ());
		    break;
		case KEY_KILL:
		    p = buf;
		    FreeWindow (wtab[CurrNum]);
		    if (other == curr)
			other = 0;
		    curr = wtab[CurrNum] = 0;
		    CheckWindows ();
		    break;
		case KEY_QUIT:
		    for (pp = wtab; pp < wtab+MAXWIN; ++pp)
			if (*pp) FreeWindow (*pp);
		    Finit ();
		    /*NOTREACHED*/
		case KEY_REDISPLAY:
		    p = buf;
		    Activate (wtab[CurrNum]);
		    break;
		case KEY_WINDOWS:
		    p = buf;
		    ShowWindows ();
		    break;
		case KEY_VERSION:
		    p = buf;
		    Msg (0, "%s; %s", ScreenVersion, AnsiVersion);
		    break;
		case KEY_INFO:
		    p = buf;
		    ShowInfo ();
		    break;
		case KEY_OTHER:
		    p = buf;
		    if (MoreWindows ())
			SwitchWindow (OtherNum);
		    break;
		case KEY_XON:
		    *p++ = Ctrl('q');
		    break;
		case KEY_XOFF:
		    *p++ = Ctrl('s');
		    break;
#ifdef LOCK
		case KEY_LOCK:
		    p = buf;
		    LockTerminal();
		    break;
#endif LOCK
		case KEY_CREATE:
		    p = buf;
		    if ((n = MakeWindow (ktab[*s].args[0], ktab[*s].args,
			    0, 0, (char *)0)) != -1)
			SwitchWindow (n);
		    break;
		}
	    } else ESCseen = 1;
	} else *p++ = *s;
    }
    return p - buf;
}

static SwitchWindow (n) {
    if (!wtab[n])
	return;
    SetCurrWindow (n);
    Activate (wtab[n]);
}

static SetCurrWindow (n) {
    /*
     * If we come from another window, this window becomes the
     * "other" window:
     */
    if (curr) {
	curr->active = 0;
	other = curr;
	OtherNum = CurrNum;
    }
    CurrNum = n;
    curr = wtab[n];
    curr->active = 1;
    /*
     * If the "other" window is currently undefined (at program start
     * or because it has died), or if the "other" window is equal to the
     * one just selected, we try to find a new one:
     */
    if (other == 0 || other == curr) {
	OtherNum = NextWindow ();
	other = wtab[OtherNum];
    }
}

static NextWindow () {
    register struct win **pp;

    for (pp = wtab+CurrNum+1; pp != wtab+CurrNum; ++pp) {
	if (pp == wtab+MAXWIN)
	    pp = wtab;
	if (*pp)
	    break;
    }
    return pp-wtab;
}

static PreviousWindow () {
    register struct win **pp;

    for (pp = wtab+CurrNum-1; pp != wtab+CurrNum; --pp) {
	if (pp < wtab)
	    pp = wtab+MAXWIN-1;
	if (*pp)
	    break;
    }
    return pp-wtab;
}

static MoreWindows () {
    register struct win **pp;
    register n;

    for (n = 0, pp = wtab; pp < wtab+MAXWIN; ++pp)
	if (*pp) ++n;
    if (n <= 1)
	Msg (0, "No other window.");
    return n > 1;
}

static FreeWindow (wp) struct win *wp; {
    register i;

    RemoveUtmp (wp->slot);
    (void) chmod (wp->tty, 0666);
    (void) chown (wp->tty, 0, 0);
    close (wp->ptyfd);
    for (i = 0; i < rows; ++i) {
	free (wp->image[i]);
	free (wp->attr[i]);
    }
    free (wp->image);
    free (wp->attr);
    free (wp);
}

static MakeWindow (prog, args, aflag, StartAt, dir)
	char *prog, **args, *dir; {
    register struct win **pp, *p;
    register char **cp;
    register n, f;
    int tf;
    int mypid;
    char ebuf[10];

#ifdef EBUG
fprintf(stderr,"DEBUG: makewindow(%s)\n",prog);
#endif /* EBUG */
    pp = wtab+StartAt;
    do {
	if (*pp == 0)
	    break;
	if (++pp == wtab+MAXWIN)
	    pp = wtab;
    } while (pp != wtab+StartAt);
    if (*pp) {
	Msg (0, "No more windows.");
	return -1;
    }
    n = pp - wtab;
    if ((f = OpenPTY ()) == -1) {
	Msg (0, "No more PTYs.");
	return -1;
    }
    fcntl (f, F_SETFL, FNDELAY);
    if ((p = *pp = (struct win *)malloc (sizeof (struct win))) == 0) {
nomem:
	Msg (0, "Out of memory.");
	return -1;
    }
    if ((p->image = (char **)malloc (rows * sizeof (char *))) == 0)
	goto nomem;
    for (cp = p->image; cp < p->image+rows; ++cp) {
	if ((*cp = malloc (cols)) == 0)
	    goto nomem;
	bclear (*cp, cols);
    }
    if ((p->attr = (char **)malloc (rows * sizeof (char *))) == 0)
	goto nomem;
    for (cp = p->attr; cp < p->attr+rows; ++cp) {
	if ((*cp = malloc (cols)) == 0)
	    goto nomem;
	bzero (*cp, cols);
    }
    if ((p->tabs = malloc (cols+1)) == 0)  /* +1 because 0 <= x <= cols */
	goto nomem;
    ResetScreen (p);
    p->aflag = aflag;
    p->active = 0;
    p->bell = 0;
    p->outlen = 0;
    p->ptyfd = f;
    strncpy (p->cmd, Filename (args[0]), MAXSTR-1);
    p->cmd[MAXSTR-1] = '\0';
    strncpy (p->tty, TtyName, MAXSTR-1);
    (void) chown (TtyName, getuid (), getgid ());
    (void) chmod (TtyName, 0622);
    p->slot = SetUtmp (TtyName);
    switch (p->wpid = fork ()) {
    case -1:
	Msg (errno, "Cannot fork");
	free (p);
	return -1;
    case 0:
	signal (SIGHUP, SIG_DFL);
	signal (SIGINT, SIG_DFL);
	signal (SIGQUIT, SIG_DFL);
	signal (SIGTERM, SIG_DFL);
	setuid (getuid ());
	setgid (getgid ());
	if (dir && chdir (dir) == -1) {
	    SendErrorMsg ("Cannot chdir to %s: %s", dir, sys_errlist[errno]);
	    exit (1);
	}
	mypid = getpid ();
	if ((f = open ("/dev/tty", O_RDWR)) != -1) {
	    ioctl (f, TIOCNOTTY, (char *)0);
	    close (f);
	}
	if ((tf = open (TtyName, O_RDWR)) == -1) {
	    SendErrorMsg ("Cannot open %s: %s", TtyName, sys_errlist[errno]);
	    exit (1);
	}
	dup2 (tf, 0);
	dup2 (tf, 1);
	dup2 (tf, 2);
	for (f = getdtablesize () - 1; f > 2; f--)
	    close (f);
	ioctl (0, TIOCSPGRP, &mypid);
	setpgrp (0, mypid);
	SetTTY (0, &OldMode);
	NewEnv[2] = MakeTermcap (aflag);
	sprintf (ebuf, "WINDOW=%d", n);
	NewEnv[3] = ebuf;
	execvpe (prog, args, NewEnv);
	SendErrorMsg ("Cannot exec %s: %s", prog, sys_errlist[errno]);
	exit (1);
    }
    return n;
}

execvpe (prog, args, env) char *prog, **args, **env; {
    register char *path, *p;
    char buf[1024];
    char *shargs[MAXARGS+1];
    register i, eaccess = 0;

    if (prog[0] == '/')
	path = "";
    else if ((path = getenv ("PATH")) == 0)
	path = DefaultPath;
    do {
	p = buf;
	while (*path && *path != ':')
	    *p++ = *path++;
	if (p > buf)
	    *p++ = '/';
	strcpy (p, prog);
	if (*path)
	    ++path;
	execve (buf, args, env);
	switch (errno) {
	case ENOEXEC:
	    shargs[0] = DefaultShell;
	    shargs[1] = buf;
	    for (i = 1; shargs[i+1] = args[i]; ++i)
		;
	    execve (DefaultShell, shargs, env);
	    return;
	case EACCES:
	    eaccess = 1;
	    break;
	case ENOMEM: case E2BIG: case ETXTBSY:
	    return;
	}
    } while (*path);
    if (eaccess)
	errno = EACCES;
}

static WriteFile (dump) {   /* dump==0: create .termcap, dump==1: hardcopy */
    register i, j, k;
    register char *p;
    register FILE *f;
    char fn[1024];
    int pid, s;

    if (dump)
	sprintf (fn, "hardcopy.%d", CurrNum);
    else
	sprintf (fn, "%s/%s/.termcap", home, SockDir);
    switch (pid = fork ()) {
    case -1:
	Msg (errno, "fork");
	return;
    case 0:
	setuid (getuid ());
	setgid (getgid ());
	if ((f = fopen (fn, "w")) == NULL)
	    exit (1);
	if (dump) {
	    for (i = 0; i < rows; ++i) {
		p = curr->image[i];
		for (k = cols-1; k >= 0 && p[k] == ' '; --k) ;
		for (j = 0; j <= k; ++j)
		    putc (p[j], f);
		putc ('\n', f);
	    }
	} else {
	    if (p = index (MakeTermcap (curr->aflag), '=')) {
		fputs (++p, f);
		putc ('\n', f);
	    }
	}
	fclose (f);
	exit (0);
    default:
	while ((i = wait (&s)) != pid)
	    if (i == -1) return;
	if ((s >> 8) & 0377)
	    Msg (0, "Cannot open \"%s\".", fn);
	else
	    Msg (0, "%s written to \"%s\".", dump ? "Screen image" :
		"Termcap entry", fn);
    }
}

static ShowWindows () {
    char buf[1024];
    register char *s;
    register struct win **pp, *p;

    for (s = buf, pp = wtab; pp < wtab+MAXWIN; ++pp) {
	if ((p = *pp) == 0)
	    continue;
	if (s - buf + 5 + strlen (p->cmd) > cols-1)
	    break;
	if (s > buf) {
	    *s++ = ' '; *s++ = ' ';
	}
	*s++ = pp - wtab + '0';
	if (p == curr)
	    *s++ = '*';
	else if (p == other)
	    *s++ = '-';
	*s++ = ' ';
	strcpy (s, p->cmd);
	s += strlen (s);
    }
    Msg (0, buf);
}

static ShowInfo () {
    char buf[1024], *p;
    register struct win *wp = curr;
    struct tm *tp;
    time_t now;
    int i;
#ifdef LOADAV
    AVvar(av);
#endif

    time (&now);
    tp = localtime (&now);
    sprintf (buf, "%2d:%02.2d:%02.2d %s", tp->tm_hour, tp->tm_min, tp->tm_sec,
	HostName);
#ifdef LOADAV
    if (avok && GetAvenrun (av)) {
	p = buf + strlen (buf);
	for (i = 0; i < sizeof(av)/sizeof(av[0]); i++) {
	    /* sprintf (p, " %2.2f" , AVfmt(av[i])), */
	    sprintf (p, " %g" , AVfmt(av[i])),
	    p = buf + strlen (buf);
	}
    }
#endif
    p = buf + strlen (buf);
    sprintf (p, " (%d,%d) %cflow %cins %corg %cwrap %cpad", wp->y, wp->x,
	flowctl ? '+' : '-',
	wp->insert ? '+' : '-', wp->origin ? '+' : '-',
	wp->wrap ? '+' : '-', wp->keypad ? '+' : '-');
    Msg (0, buf);
}

static OpenPTY () {
    register char *p, *l, *d;
    register i, f, tf;

    strcpy (PtyName, PtyProto);
    strcpy (TtyName, TtyProto);
    for (p = PtyName, i = 0; *p != 'X'; ++p, ++i) ;
    for (l = PTYCHAR1; *p = *l; ++l) {
	for (d = PTYCHAR2; p[1] = *d; ++d) {
	    if ((f = open (PtyName, O_RDWR)) != -1) {
		TtyName[i] = p[0];
		TtyName[i+1] = p[1];
		if ((tf = open (TtyName, O_RDWR)) != -1) {
		    close (tf);
		    return f;
		}
		close (f);
	    }
	}
    }
    return -1;
}

static SetTTY (fd, mp) struct mode *mp; {
    ioctl (fd, TIOCSETP, &mp->m_ttyb);
    ioctl (fd, TIOCSETC, &mp->m_tchars);
    ioctl (fd, TIOCSLTC, &mp->m_ltchars);
    ioctl (fd, TIOCLSET, &mp->m_lmode);
    ioctl (fd, TIOCSETD, &mp->m_ldisc);
}

static GetTTY (fd, mp) struct mode *mp; {
    ioctl (fd, TIOCGETP, &mp->m_ttyb);
    ioctl (fd, TIOCGETC, &mp->m_tchars);
    ioctl (fd, TIOCGLTC, &mp->m_ltchars);
    ioctl (fd, TIOCLGET, &mp->m_lmode);
    ioctl (fd, TIOCGETD, &mp->m_ldisc);
}

static SetMode (op, np) struct mode *op, *np; {
    *np = *op;
    np->m_ttyb.sg_flags &= ~(CRMOD|ECHO);
    np->m_ttyb.sg_flags |= CBREAK;
    np->m_tchars.t_intrc = -1;
    np->m_tchars.t_quitc = -1;
#ifdef pe3200
    /* this is because this is really a System V
     * In CBREAK mode eof = min number of chars to be read before return
     *                eol = time in 1/10 seconds waiting for input
     */
    np->m_tchars.t_eofc = 1;
    np->m_tchars.t_brkc = 1;
#endif
    if (!flowctl) {
	np->m_tchars.t_startc = -1;
	np->m_tchars.t_stopc = -1;
    }
    np->m_ltchars.t_suspc = -1;
    np->m_ltchars.t_dsuspc = -1;
    np->m_ltchars.t_flushc = -1;
    np->m_ltchars.t_lnextc = -1;
}

static GetSockName () {
    struct stat s;
    register client;
    register char *p;

    if (!mflag && (SockName = getenv ("STY")) != 0 && *SockName != '\0') {
	client = 1;
	setuid (getuid ());
	setgid (getgid ());
    } else {
	if ((p = ttyname (0)) == 0 || (p = ttyname (1)) == 0 ||
		(p = ttyname (2)) == 0 || *p == '\0')
	    Msg (0, "screen must run on a tty.");
	SockName = Filename (p);
	client = 0;
    }
    if ((home = getenv ("HOME")) == 0)
	Msg (0, "$HOME is undefined.");
#ifdef SEQUENT
    /* the sequent has problems with binding AF_UNIX sockets
     * to NFS files. So we changed the path to point to a (hopefully)
     * local directory
     */
    sprintf (SockPath, "/tmp/.%s.%d",  SockDir, getuid());
#else SEQUENT
    sprintf (SockPath, "/tmp/.%s.%d",  home, SockDir);
#endif SEQUENT
    if (stat (SockPath, &s) == -1) {
	if (errno == ENOENT) {
	    if (mkdir (SockPath, 0700) == -1)
		Msg (errno, "Cannot make directory %s", SockPath);
	    (void) chown (SockPath, getuid (), getgid ());
	} else Msg (errno, "Cannot get status of %s", SockPath);
    } else {
	if ((s.st_mode & S_IFMT) != S_IFDIR)
	    Msg (0, "%s is not a directory.", SockPath);
	if ((s.st_mode & 0777) != 0700)
	    Msg (0, "Directory %s must have mode 700.", SockPath);
	if (s.st_uid != getuid ())
	    Msg (0, "You are not the owner of %s.", SockPath);
    }
    strcat (SockPath, "/");
    strcat (SockPath, SockName);
    return client;
}

static MakeServerSocket () {
    register s;
    struct sockaddr_un a;

    (void) unlink (SockPath);
    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
	Msg (errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy (a.sun_path, SockPath);
    if (bind (s, (struct sockaddr *)&a, strlen (SockPath)+2) == -1)
	Msg (errno, "bind");
    (void) chown (SockPath, getuid (), getgid ());
    if (listen (s, 5) == -1)
	Msg (errno, "listen");
    return s;
}

static MakeClientSocket () {
    register s;
    struct sockaddr_un a;

    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
	Msg (errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy (a.sun_path, SockPath);
    if (connect (s, (struct sockaddr *)&a, strlen (SockPath)+2) == -1)
	Msg (errno, "connect: %s", SockPath);
    return s;
}

static SendCreateMsg (s, ac, av, aflag) char **av; {
    struct msg m;
    register char *p;
    register len, n;

    m.type = MSG_CREATE;
    p = m.m.create.line;
    for (n = 0; ac > 0 && n < MAXARGS-1; ++av, --ac, ++n) {
	len = strlen (*av) + 1;
	if (p + len >= m.m.create.line+MAXLINE)
	    break;
	strcpy (p, *av);
	p += len;
    }
    m.m.create.nargs = n;
    m.m.create.aflag = aflag;
    if (getwd (m.m.create.dir) == 0)
	Msg (0, "%s", m.m.create.dir);
    if (write (s, &m, sizeof (m)) != sizeof (m))
	Msg (errno, "write");
}

/*VARARGS1*/
static SendErrorMsg (fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
    register s;
    struct msg m;

    s = MakeClientSocket ();
    m.type = MSG_ERROR;
    sprintf (m.m.message, fmt, p1, p2, p3, p4, p5, p6);
    (void) write (s, &m, sizeof (m));
    close (s);
    sleep (2);
}

static ReceiveMsg (s) {
    register ns;
    struct sockaddr_un a;
    int len = sizeof (a);
    struct msg m;

    if ((ns = accept (s, (struct sockaddr *)&a, &len)) == -1) {
	Msg (errno, "accept");
	return;
    }
    if ((len = read (ns, &m, sizeof (m))) != sizeof (m)) {
	if (len == -1)
	    Msg (errno, "read");
	else
	    Msg (0, "Short message (%d bytes)", len);
	close (ns);
	return;
    }
    switch (m.type) {
    case MSG_CREATE:
	ExecCreate (&m);
	break;
    case MSG_ERROR:
	Msg (0, "%s", m.m.message);
	break;
    default:
	Msg (0, "Invalid message (type %d).", m.type);
    }
    close (ns);
}

static ExecCreate (mp) struct msg *mp; {
    char *args[MAXARGS];
    register n;
    register char **pp = args, *p = mp->m.create.line;

    for (n = mp->m.create.nargs; n > 0; --n) {
	*pp++ = p;
	p += strlen (p) + 1;
    }
    *pp = 0;
    if ((n = MakeWindow (mp->m.create.line, args, mp->m.create.aflag, 0,
	    mp->m.create.dir)) != -1)
	SwitchWindow (n);
}

static ReadRc (fn) char *fn; {
    FILE *f;
    register char *p, **pp, **ap;
    register argc, num, c;
    char buf[256];
    char *args[MAXARGS];
    int key;

    ap = args;
    if (access (fn, R_OK) == -1)
	return;
    if ((f = fopen (fn, "r")) == NULL)
	return;
    while (fgets (buf, 256, f) != NULL) {
	if (p = rindex (buf, '\n'))
	    *p = '\0';
	if ((argc = Parse (fn, buf, ap)) == 0)
	    continue;
	if (strcmp (ap[0], "escape") == 0) {
	    p = ap[1];
	    if (argc < 2 || strlen (p) != 2)
		Msg (0, "%s: two characters required after escape.", fn);
	    Esc = *p++;
	    MetaEsc = *p;
	} else if (strcmp (ap[0], "chdir") == 0) {
	    p = argc < 2 ? home : ap[1];
	    if (chdir (p) == -1)
		Msg (errno, "%s", p);
	} else if (strcmp (ap[0], "bell") == 0) {
	    if (argc != 2) {
		Msg (0, "%s: bell: one argument required.", fn);
	    } else {
		if ((BellString = malloc (strlen (ap[1]) + 1)) == 0)
		    Msg (0, "Out of memory.");
		strcpy (BellString, ap[1]);
	    }
	} else if (strcmp (ap[0], "screen") == 0) {
	    num = 0;
	    if (argc > 1 && IsNum (ap[1], 10)) {
		num = atoi (ap[1]);
		if (num < 0 || num > MAXWIN-1)
		    Msg (0, "%s: illegal screen number %d.", fn, num);
		--argc; ++ap;
	    }
	    if (argc < 2) {
		ap[1] = ShellProg; argc = 2;
	    }
	    ap[argc] = 0;
	    (void) MakeWindow (ap[1], ap+1, 0, num, (char *)0);
	} else if (strcmp (ap[0], "bind") == 0) {
	    p = ap[1];
	    if (argc < 2 || *p == '\0')
		Msg (0, "%s: key expected after bind.", fn);
	    if (p[1] == '\0') {
		key = *p;
	    } else if (p[0] == '^' && p[1] != '\0' && p[2] == '\0') {
		c = p[1];
		if (isupper (c))
		    p[1] = tolower (c);    
		key = Ctrl(c);
	    } else if (IsNum (p, 7)) {
		(void) sscanf (p, "%o", &key);
	    } else {
		Msg (0,
		    "%s: bind: character, ^x, or octal number expected.", fn);
	    }
	    if (argc < 3) {
		ktab[key].type = 0;
	    } else {
		for (pp = KeyNames; *pp; ++pp)
		    if (strcmp (ap[2], *pp) == 0) break;
		if (*pp) {
		    ktab[key].type = pp-KeyNames+1;
		} else {
		    ktab[key].type = KEY_CREATE;
		    ktab[key].args = SaveArgs (argc-2, ap+2);
		}
	    }
	} else Msg (0, "%s: unknown keyword \"%s\".", fn, ap[0]);
    }
    fclose (f);
}

static Parse (fn, buf, args) char *fn, *buf, **args; {
    register char *p = buf, **ap = args;
    register delim, argc = 0;

    argc = 0;
    for (;;) {
	while (*p && (*p == ' ' || *p == '\t')) ++p;
	if (*p == '\0' || *p == '#')
	    return argc;
	if (argc > MAXARGS-1)
	    Msg (0, "%s: too many tokens.", fn);
	delim = 0;
	if (*p == '"' || *p == '\'') {
	    delim = *p; *p = '\0'; ++p;
	}
	++argc;
	*ap = p; ++ap;
	while (*p && !(delim ? *p == delim : (*p == ' ' || *p == '\t')))
	    ++p;
	if (*p == '\0') {
	    if (delim)
		Msg (0, "%s: Missing quote.", fn);
	    else
		return argc;
	}
	*p++ = '\0';
    }
}

static char **SaveArgs (argc, argv) register argc; register char **argv; {
    register char **ap, **pp;

    if ((pp = ap = (char **)malloc ((argc+1) * sizeof (char **))) == 0)
	Msg (0, "Out of memory.");
    while (argc--) {
	if ((*pp = malloc (strlen (*argv)+1)) == 0)
	    Msg (0, "Out of memory.");
	strcpy (*pp, *argv);
	++pp; ++argv;
    }
    *pp = 0;
    return ap;
}

static MakeNewEnv () {
    register char **op, **np = NewEnv;
    static char buf[MAXSTR];

    if (strlen (SockName) > MAXSTR-5)
	SockName = "?";
    sprintf (buf, "STY=%s", SockName);
    *np++ = buf;
    *np++ = Term;
    np += 2;
    for (op = environ; *op; ++op) {
	if (np == NewEnv + MAXARGS - 1)
	    break;
	if (!IsSymbol (*op, "TERM") && !IsSymbol (*op, "TERMCAP")
		&& !IsSymbol (*op, "STY"))
	    *np++ = *op;
    }
    *np = 0;
}

static IsSymbol (e, s) register char *e, *s; {
    register char *p;
    register n;

    for (p = e; *p && *p != '='; ++p) ;
    if (*p) {
	*p = '\0';
	n = strcmp (e, s);
	*p = '=';
	return n == 0;
    }
    return 0;
}

/*VARARGS2*/
Msg (err, fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
    char buf[1024];
    register char *p = buf;

    sprintf (p, fmt, p1, p2, p3, p4, p5, p6);
    if (err) {
	p += strlen (p);
	if (err > 0 && err < sys_nerr)
	    sprintf (p, ": %s", sys_errlist[err]);
	else
	    sprintf (p, ": Error %d", err);
    }
    if (HasWindow) {
	MakeStatus (buf, curr);
    } else {
	printf ("%s\r\n", buf);
	exit (1);
    }
}

bclear (p, n) char *p; {
    bcopy (blank, p, n);
}

static char *Filename (s) char *s; {
    register char *p;

    p = s + strlen (s) - 1;
    while (p >= s && *p != '/') --p;
    return ++p;
}

static IsNum (s, base) register char *s; register base; {
    for (base += '0'; *s; ++s)
	if (*s < '0' || *s > base)
	    return 0;
    return 1;
}

static char *MakeBellMsg (n) {
    static char buf[MAXSTR];
    register char *p = buf, *s = BellString;

    for (s = BellString; *s && p < buf+MAXSTR-1; s++)
	*p++ = (*s == '%') ? n + '0' : *s;
    *p = '\0';
    return buf;
}

static InitUtmp () {
    struct passwd *p;

#ifndef pe3200
    if ((utmpf = open (UtmpName, O_WRONLY)) == -1) {
	if (errno != EACCES)
	    Msg (errno, UtmpName);
	return;
    }
#endif
    if ((LoginName = getlogin ()) == 0 || LoginName[0] == '\0') {
	if ((p = getpwuid (getuid ())) == 0)
	    LoginName = "SCREEN";
        else
	  LoginName = p->pw_name;
    }
    utmp = 1;
}

static
#ifdef pe3200
  struct utmp *
#endif
  SetUtmp (name) char *name; {
    register char *p;
#ifdef pe3200
    struct utmp *u;
#else
    register struct ttyent *tp;
    register slot = 1;
    struct utmp u;

    if (!utmp)
	return 0;
#endif
    if (p = rindex (name, '/'))
	++p;
    else p = name;
#ifndef pe3200
    setttyent ();
    while ((tp = getttyent ()) != NULL && strcmp (p, tp->ty_name) != 0)
	++slot;
    if (tp == NULL)
	return 0;
    strncpy (u.ut_line, p, 8);
    strncpy (u.ut_name, LoginName, 8);
    time (&u.ut_time);
    u.ut_host[0] = '\0';
    (void) lseek (utmpf, (long)(slot * sizeof (u)), 0);
    (void) write (utmpf, &u, sizeof (u));
    return slot;
#else
    if ( (u = (struct utmp *)malloc(sizeof(struct utmp)) == (struct utmp *)NULL) )
      {
        return NULL;
      }
    strncpy (u->ut_line, p, 12);
    u->ut_id[0] = *p;
    strncpy(&u->ut_id[1],strlen(p) > 1 ? p + strlen(p) - 2 : p, 2);
    u->ut_id[3] = '\0';
    strncpy (u->ut_user, LoginName, 8);
    time (&u->ut_time);
    u->ut_type = USER_PROCESS;
    u->ut_pid = getpid();
    pututline(u);
    return u;
#endif
}

static RemoveUtmp (slot)
#ifdef pe3200
  struct utmp *slot;
#endif
{
#ifdef pe3200
    if (slot)
      {
        slot->ut_type = DEAD_PROCESS;
	pututline(slot);
      }
#else
    struct utmp u;

    if (slot) {
	bzero (&u, sizeof (u));
	(void) lseek (utmpf, (long)(slot * sizeof (u)), 0);
	(void) write (utmpf, &u, sizeof (u));
    }
#endif
}

#ifdef LOCK

/* ADDED by Rainer Pruy 10/15/87 */
static void LockTerminal()
{
  char *prg;
  int sig, pid;
  void * sigs[NSIG];

  for ( sig = 1; sig < NSIG; sig++ )
    {
      sigs[sig] = (void *)signal(sig,SIG_IGN);
    }
  SetTTY (0, &OldMode);
  FinitTerm ();
  printf("\n");

  if ( (pid = fork()) == 0)
    {
      /* Child */
      if ( (prg = getenv("LOCKPRG")) == NULL)
	prg = DefaultLock;
  
      execl(prg,"SCREEN-LOCK",NULL);
      Msg(errno,"%s",prg);
      sleep(2);
      exit(0);
    }
  if (pid == -1)
    {
      SetTTY (0, &NewMode);
      Activate (wtab[CurrNum]);
      Msg(errno,"Cannot lock terminal - fork failed");
    }
  else
    {
      union wait status;
      int wret, saverrno;

      errno = 0;
      while ( ((wret=wait(&status)) != pid) ||
	      ((wret == -1) && (errno = EINTR))
	    ) errno = 0;

      saverrno = errno;
      SetTTY (0, &NewMode);
      Activate (wtab[CurrNum]);

      if (saverrno)
	Msg(saverrno,"Lock");
      else
	if (status.w_T.w_Termsig != 0)
	  Msg(0,"Lock: %s: Killed by signal: %d%s",prg,
	      status.w_T.w_Termsig,status.w_T.w_Coredump?" (Core dumped)":"");
	else
	  if (status.w_T.w_Retcode)
	    Msg(0,"Lock: %s: Returnecode = %d",prg,status.w_T.w_Retcode);
	  else
	    {
	      int startpos;
	      char *endmsg = getenv("LOCKMSG");

	      if ( endmsg == NULL ) endmsg = LockEnd;
	      startpos = (cols - strlen(endmsg)) / 2 - 1;
	      if ( startpos < 0 ) startpos = 0;
	      Msg(0,"%*s%-*s", startpos, "", cols - startpos, endmsg);
	    }
    }
  /* reset signals */
  for ( sig = 1; sig < NSIG; sig++ )
    {
      signal(sig,(int (*)())sigs[sig]);
    }
} /* LockTerminal */

#endif LOCK


#ifndef GETTTYENT

static setttyent () {
    struct stat s;
    register f;
    register char *p, *ep;

    if (ttnext) {
	ttnext = tt;
	return;
    }
    if ((f = open (ttys, O_RDONLY)) == -1 || fstat (f, &s) == -1)
	Msg (errno, ttys);
    if ((tt = malloc (s.st_size + 1)) == 0)
	Msg (0, "Out of memory.");
    if (read (f, tt, s.st_size) != s.st_size)
	Msg (errno, ttys);
    close (f);
    for (p = tt, ep = p + s.st_size; p < ep; ++p)
	if (*p == '\n') *p = '\0';
    *p = '\0';
    ttnext = tt;
}

static struct ttyent *getttyent () {
    static struct ttyent t;

    if (*ttnext == '\0')
	return NULL;
    t.ty_name = ttnext + 2;
    ttnext += strlen (ttnext) + 1;
    return &t;
}

#endif

#ifdef LOADAV

static InitKmem () {
    avok = 0;
    nl[0].n_name = AvenrunSym;
#ifdef KVMLIB
    if ((kmemf = kvm_open (NULL, NULL, NULL, O_RDONLY, "screen")) == NULL)
	return;
    kvm_nlist (kmemf, nl);
#else KVMLIB
    if ((kmemf = open (KmemName, O_RDONLY)) == -1)
	return;
    nlist (UnixName, nl);
#endif KVMLIB
    if (nl[0].n_type == 0 || nl[0].n_value == 0)
	return;
    avok = 1;
}

static GetAvenrun (av) AVvar(av); {
#ifdef KVMLIB
    if (kvm_read( kmemf, nl[0].n_value, av, sizeof (av)) != sizeof(av))
	return 0;
#else KVMLIB
    if (lseek (kmemf, nl[0].n_value, 0) == -1)
	return 0;
    if (read (kmemf, av, sizeof (av)) != sizeof (av))
	return 0;
#endif KVMLIB
    return 1;
}

#endif
