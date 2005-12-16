/* Copyright (c) 1987-1990 Oliver Laumann and Wayne Davison.
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 *
 * Noteworthy contributors to screen's design and implementation:
 *	Patrick Wolfe (pat@kai.com, kailand!pat)
 *	Bart Schaefer (schaefer@cse.ogi.edu)
 *	Nathan Glasser (nathan@brokaw.lcs.mit.edu)
 *	Larry W. Virden (lwv27%cas.BITNET@CUNYVM.CUNY.Edu)
 */

static char ScreenVersion[] = "screen 2.3-PreRelease7 19-Jun-90";

#include <stdio.h>
#include <strings.h>
#include <sgtty.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <nlist.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "config.h"

#ifdef UTMPOK
#   include <utmp.h>
#endif
#ifdef USEVARARGS
#   include <varargs.h>
#endif
#ifdef sun
#   include <sys/resource.h>
#   include <sys/param.h>
#   include <dirent.h>
    typedef struct dirent DIRENT;
#else
#   include <sys/dir.h>
    typedef struct direct DIRENT;
#endif
#ifdef GETTTYENT
#   include <ttyent.h>
#else
    static struct ttyent {
	char *ty_name;
    } *getttyent();
    static char *tt, *ttnext;
    static char ttys[] = "/etc/ttys";
#endif

#include "screen.h"

#define MAXWIN     10
#define MSGWAIT     5


#ifdef USRLIMIT
    struct utmp utmpbuf;
    int UserCount;
#endif

#define Ctrl(c) ((c)&037)

extern char *blank, *null, Term[], **environ, *extra_incap, *extra_outcap;
int    force_vt = 1, assume_LP = 0, help_page = 0;
extern rows, cols;
extern maxwidth, minwidth, default_width;
extern ISO2022;
extern status, HS;
extern char *Z0, *LastMsg;
extern time_t TimeDisplayed, time();
extern char AnsiVersion[];
extern short ospeed;
extern flowctl, wrap;
extern errno;
extern sys_nerr;
extern char *sys_errlist[];

extern off_t lseek();
extern void exit();
extern char *getwd(), *malloc(), *realloc(), *getenv(), *MakeTermcap();
extern char *getlogin(), *ttyname(), *ParseChar();
static char *MakeWinMsg(), *Filename(), **SaveArgs(), *GetTtyName();
static char *ProcessInput(), *SaveStr(), *CatStr();
static sig_t AttacherFinit(), Finit(), SigHup(), SigChld(), SigInt();
static sig_t SigHandler(), SigStop();

static char PtyName[32], TtyName[32];
static char *ShellProg;
static char *ShellArgs[2];
static char inbuf[MAXWIN][IOSIZE];
static inlen[MAXWIN];
static inbuf_ct;
static ESCseen;
static GotSignal;
static char DefaultShell[] = "/bin/sh";
static char DefaultPath[] = ":/usr/ucb:/bin:/usr/bin";
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
static int TtyMode = 0622;
static char SockPath[512];
#ifdef SOCKDIR
static char SockDir[] = SOCKDIR;
#else
static char SockDir[] = ".screen";
#endif
static char *RcFileName = NULL;
static char *SockNamePtr, *SockName;
static ServerSocket;
static char **NewEnv;
static char Esc = Ctrl('a');
static char MetaEsc = 'a';
static char *home;
static HasWindow;
#ifdef UTMPOK
    static utmp, utmpf;
    static char UtmpName[] = "/etc/utmp";
#endif
static char *LoginName;
static char *BellString = "Bell in window %";
static char *ActivityString = "Activity in window %";
static auto_detach = 1;
static iflag, mflag, rflag;
static loginflag = LOGINDEFAULT;
static intrc, startc, stopc;
static char HostName[MAXSTR];
static Detached, Suspended;
static command_search, command_bindings = 0;
static AttacherPid;	/* Non-Zero in child if we have an attacher */
static real_uid, real_gid, eff_uid, eff_gid;
static DevTty;
#ifdef LOADAV
    static char KmemName[] = "/dev/kmem";
#ifdef sequent
    static char UnixName[] = "/dynix";
#else
    static char UnixName[] = "/vmunix";
#endif
#ifdef alliant
    static char AvenrunSym[] = "_Loadavg";
#else
    static char AvenrunSym[] = "_avenrun";
#endif
    static struct nlist nl[2];
    static avenrun, kmemf;
#ifdef LOADAV_3LONGS
    long loadav[3];
#else
#ifdef LOADAV_4LONGS
    long loadav[4];
#else
    double loadav[3];
#endif
#endif

#endif

struct mode {
    struct sgttyb m_ttyb;
    struct tchars m_tchars;
    struct ltchars m_ltchars;
    int m_ldisc;
    int m_lmode;
} OldMode, NewMode;

struct win *fore;
static ForeNum, WinList = -1;
static struct win *wtab[MAXWIN];

#define KEY_IGNORE         0	/* Keep these first 3 at the start */
#define KEY_SCREEN         1
#define KEY_0              2
#define KEY_1              3
#define KEY_2              4
#define KEY_3              5
#define KEY_4              6
#define KEY_5              7
#define KEY_6              8
#define KEY_7              9
#define KEY_8             10
#define KEY_9             11
#define KEY_AKA           12
#define KEY_AUTOFLOW      13
#define KEY_CLEAR         14
#define KEY_DETACH        15
#define KEY_FLOW          16
#define KEY_HARDCOPY      17
#define KEY_HELP          18
#define KEY_INFO          19
#define KEY_KILL          20
#define KEY_LASTMSG       21
#define KEY_LOGTOGGLE     22
#define KEY_LOGIN         23
#define KEY_MONITOR       24
#define KEY_NEXT          25
#define KEY_OTHER         26
#define KEY_PREV          27
#define KEY_QUIT          28
#define KEY_REDISPLAY     29
#define KEY_RESET         30
#define KEY_SHELL         31
#define KEY_SUSPEND       32
#define KEY_TERMCAP       33
#define KEY_TIME          34
#define KEY_VERSION       35
#define KEY_WIDTH         36
#define KEY_WINDOWS       37
#define KEY_WRAP          38
#define KEY_XOFF          39
#define KEY_XON           40
#define KEY_CREATE       255

struct key {
    int type;
    char **args;
} ktab[256];

#ifndef FD_SET
typedef struct fd_set { int fd_bits[1]; } fd_set;
#   define FD_ZERO(fd) ((fd)->fd_bits[0] = 0)
#   define FD_SET(b,fd) ((fd)->fd_bits[0] |= 1 << (b))
#   define FD_ISSET(b,fd) ((fd)->fd_bits[0] & 1 << (b))
#endif
union fd_mask {
    fd_set fd_val;
    long long_val;
};


char *shellaka = NULL;

char *KeyNames[] = {
    "screen",
    "select0", "select1", "select2", "select3", "select4",
    "select5", "select6", "select7", "select8", "select9",
    "aka", "autoflow", "clear", "detach", "flow", "hardcopy", "help",
    "info", "kill", "lastmsg", "log", "login", "monitor", "next",
    "other", "prev", "quit", "redisplay", "reset", "shell",
    "suspend", "termcap", "time", "version", "width", "windows",
    "wrap", "xoff", "xon",
    0
};

main(ac, av) char **av; {
    register int n, len;
    register struct win *p;
    char *ap, *aka = NULL;
    struct passwd *ppp;
    int s = 0;
    union fd_mask r, w, x;
    int aflag = 0, oumask;
    struct timeval tv;
    time_t now;
    char buf[IOSIZE], *bufp, *myname = (ac == 0) ? "screen" : av[0];
    struct stat st;
    int buflen,tmp;

    FD_ZERO(&x.fd_val);
    while (ac > 0) {
	ap = *++av;
	if (--ac > 0 && *ap == '-') {
	    switch (ap[1]) {
	    case 'a':
		aflag = 1;
		break;
	    case 'c':
		if (ap[2])
		    RcFileName = ap+2;
		else {
		    if (--ac == 0) goto help;
		    RcFileName = *++av;
		}
		break;
	    case 'e':
		if (ap[2])
		    ap += 2;
		else {
		    if (--ac == 0) goto help;
		    ap = *++av;
		}
		if (!ParseEscape(ap))
		    Msg(0, "Two characters are required with -e option.");
		break;
	    case 'n':			/* backward compatibility */
		flowctl = 1;
		break;
	    case 'f':
		switch (ap[2]) {
		case 'n':
		case '0':
		    flowctl = 1;
		    break;
		case 'y':
		case '1':
		case '\0':
		    flowctl = 2;
		    break;
		case 'a':
		    flowctl = 3;
		    break;
		default:
		    goto help;
		}
		break;
	    case 'i':
		iflag = 1;
		break;
	    case 'k':
		if (ap[2])
		    aka = ap+2;
		else {
		    if (--ac == 0) goto help;
		    aka = *++av;
		}
		break;
	    case 'l':
		switch (ap[2]) {
		case 'n':
		case '0':
		    loginflag = 0;
		    break;
		case 'y':
		case '1':
		case '\0':
		    loginflag = 1;
		    break;
		default:
		    goto help;
		}
		break;
	    case 'L':
		assume_LP = 1;
		break;
	    case 'm':
		mflag = 1;
		break;
	    case 'O':
		force_vt = 0;
		break;
	    case 'r':
		if (ap[2]) {
		    SockName = ap+2;
		    if (ac != 1) goto help;
		} else if (--ac == 1) {
		    SockName = *++av;
		} else if (ac != 0) goto help;
	    case 'R':
		rflag = (ap[1] == 'r' ? 1 : 2);
		break;
	    case 's':
		if (ap[2])
		    ShellProg = ap+2;
		else {
		    if (--ac == 0) goto help;
		    ShellProg = *++av;
		}
		break;
	    default:
	    help:
		printf("Use: %s [-opts] [cmd [args]]\n", myname);
		printf(" or: %s -r [host.tty]\n\nOptions:\n", myname);
		printf("-a       Force all capabilities into each window's termcap\n");
		printf("-c file  Read configuration file instead of .screenrc\n");
		printf("-e xy    Change command characters\n");
		printf("-f       Flow control on, -fn = off, -fa = auto\n");
		printf("-i       Interrupt output sooner when flow control is on\n");
		printf("-k aka   Set command's a.k.a. (name alias)\n");
		printf("-l       Login mode on (update /etc/utmp), -ln = off\n");
		printf("-L       Terminal's last character can be safely updated\n");
		printf("-O       Choose optimal output rather than exact vt100 emulation\n");
		printf("-r       Reattach to a detached screen process\n");
		printf("-R       Reattach if possible, otherwise start a new session\n");
		printf("-s shell Shell to execute rather than $SHELL\n");
		exit(1);
	    }
	} else
	    break;
    }
    real_uid = getuid();
    real_gid = getgid();
    eff_uid = geteuid();
    eff_gid = getegid();
    if (!ShellProg && (ShellProg = getenv("SHELL")) == 0)
	ShellProg = DefaultShell;
    ShellArgs[0] = ShellProg;
    if ((home = getenv("HOME")) == 0)
	Msg(0, "$HOME is undefined.");
    if ((LoginName = getlogin()) == 0 || LoginName[0] == '\0') {
	if ((ppp = getpwuid(real_uid)) == 0)
	    return 1;
	LoginName = ppp->pw_name;
    }
    if ((oumask = umask(0)) == -1)
	Msg(errno, "Cannot change umask to zero");
#ifdef SOCKDIR
    if (stat(SOCKDIR, &st) == -1) {
	if (errno == ENOENT)
	    Msg(errno, "Directory not found: %s", SOCKDIR);
	else
	    Msg(errno, "Cannot get status of %s", SOCKDIR);
    } else if ((st.st_mode & S_IFMT) != S_IFDIR)
	Msg(0, "%s is not a directory.", SOCKDIR);
    sprintf(SockPath, "%s/S-%s", SockDir, LoginName);
#else
    sprintf(SockPath, "%s/%s", home, SockDir);
#endif
    if (stat(SockPath, &st) == -1) {
	if (errno == ENOENT) {
	    if (mkdir(SockPath, 0700) == -1)
		Msg(errno, "Cannot make directory %s", SockPath);
	    (void) chown(SockPath, real_uid, real_gid);
	} else
	    Msg(errno, "Cannot get status of %s", SockPath);
    } else {
	if ((st.st_mode & S_IFMT) != S_IFDIR)
	    Msg(0, "%s is not a directory.", SockPath);
	if ((st.st_mode & 0777) != 0700)
	    Msg(0, "Directory %s must have mode 700.", SockPath);
	if (st.st_uid != real_uid)
	    Msg(0, "You are not the owner of %s.", SockPath);
    }
    umask(oumask);
    (void) gethostname(HostName, MAXSTR);
    HostName[MAXSTR-1] = '\0';
    if (ap = index(HostName, '.'))
	*ap = '\0';
    strcat(SockPath, "/");
    SockNamePtr = SockPath + strlen(SockPath);
    if (rflag) {
	if (Attach(MSG_ATTACH)) {
	    Attacher();
	    /*NOTREACHED*/
	}
	rflag = 0;
    }
    if (GetSockName()) {
	s = MakeClientSocket(1);
	if (ac == 0) {
	    ac = 1;
	    av = ShellArgs;
	}
	av[ac] = aka;
	SendCreateMsg(s, ac, av, aflag, flowctl, loginflag);
	close(s);
	exit(0);
    }
    if ((DevTty = open("/dev/tty", O_RDWR|O_NDELAY)) == -1)
	Msg(errno, "/dev/tty");
    switch (fork()) {
    case -1:
	Msg(errno, "fork");
	/*NOTREACHED*/
    case 0:
	break;
    default:
	Attacher();
	/*NOTREACHED*/
    }
    AttacherPid = getppid();
    ServerSocket = s = MakeServerSocket();
    StartRc();
    InitTermcap();
    InitTerm();
    MakeNewEnv();
    GetTTY(0, &OldMode);
    ospeed = (short)OldMode.m_ttyb.sg_ospeed;
#ifdef UTMPOK
    InitUtmp();
#endif
#ifdef LOADAV
    InitKmem();
#endif
    signal(SIGHUP, SigHup);
    signal(SIGINT, Finit);
    signal(SIGQUIT, Finit);
    signal(SIGTERM, Finit);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    InitKeytab();
    FinishRc();
    if (ac == 0) {
	ac = 1;
	av = ShellArgs;
	if (!aka)
	    aka = shellaka;
    }
    if ((n = MakeWindow(aka,av,aflag,flowctl,0,(char*)0,loginflag)) == -1) {
	Finit();
	/*NOTREACHED*/
    }
    SetForeWindow(n);
    HasWindow = 1;
    SetMode(&OldMode, &NewMode);
    SetTTY(0, &NewMode);
    signal(SIGCHLD, SigChld);
    signal(SIGINT, SigInt);
    tv.tv_usec = 0;
    while (1) {
	/*
	 *	check to see if message line should be removed
	 */
	if (status) {
	    (void) time(&now);
	    if (now - TimeDisplayed < MSGWAIT) {
		tv.tv_sec = MSGWAIT - (now - TimeDisplayed);
	    } else
		RemoveStatus();
	}
	/*
	 *	check for I/O on all available I/O descriptors
	 */
	FD_ZERO(&r.fd_val);
	FD_ZERO(&w.fd_val);
	if (inbuf_ct > 0)
	    for (n = 0; n < MAXWIN; n++)
		if (inlen[n] > 0)
		    FD_SET(wtab[n]->ptyfd,&w.fd_val);
	if (!Detached)
	    FD_SET(0, &r.fd_val);
	for (n = WinList; n != -1; n = p->WinLink) {
	    p = wtab[n];
	    if (p->active && status && !HS)
		continue;
	    if (p->outlen > 0)
		continue;
	    FD_SET(p->ptyfd, &r.fd_val);
	}
	FD_SET(s, &r.fd_val);
	(void) fflush(stdout);
	if (GotSignal && !status) {
	    SigHandler();
	    continue;
	}
	if (select(32, &r, &w, &x.fd_val, status ? &tv : (struct timeval *)0) == -1) {
	    if (errno == EINTR) {
		errno = 0;
		continue;
	    }
	    perror("select");
	    Finit();
	    /*NOTREACHED*/
	}
	if (GotSignal && !status) {
	    SigHandler();
	    continue;
	}
	/* Process a client connect attempt and message */
	if (FD_ISSET(s, &r.fd_val)) {
	    if (!HS)
		RemoveStatus();
	    ReceiveMsg(s);
	    continue;
	}
	/* Read, process, and store the user input */
	if (FD_ISSET(0, &r.fd_val)) {
	    if (!HS)
		RemoveStatus();
	    if (ESCseen) {
		buf[0] = Esc;
		buflen = read(0, buf + 1, IOSIZE - 1) + 1;
		ESCseen = 0;
	    } else
		buflen = read(0, buf, IOSIZE);
	    if (buflen < 0) {
		perror("read");
		Finit();
	    }
	    bufp = buf;
	    if (help_page)
		process_help_input(&bufp,&buflen);
	    while (buflen > 0) {
		n = ForeNum;
		len = inlen[n];
		bufp = ProcessInput(bufp,&buflen,inbuf[n],&inlen[n],
				    sizeof *inbuf);
		if (inlen[n] > 0 && len == 0)
		    inbuf_ct++;
	    }
	    if (inbuf_ct > 0)
		continue;
	}
	if (GotSignal && !status) {
	    SigHandler();
	    continue;
	}
	/* Write the stored user input to the window descriptors */
	if (inbuf_ct > 0 && w.long_val != 0) {
	    for (n = 0; n < MAXWIN; n++) {
		if (inlen[n] > 0) {
		    tmp = wtab[n]->ptyfd;
		    if (FD_ISSET(tmp,&w.fd_val)
		     && (len = write(tmp, inbuf[n], inlen[n])) > 0) {
			if ((inlen[n] -= len) == 0)
			    inbuf_ct--;
			bcopy(inbuf[n] + len, inbuf[n], inlen[n]);
		    }
		}
	    }
	}
	if (GotSignal && !status) {
	    SigHandler();
	    continue;
	}
	/* Read and process the output from the window descriptors */
	for (n = WinList; n != -1; n = p->WinLink) {
	    p = wtab[n];
	    if (p->outlen)
		WriteString(p, p->outbuf, p->outlen);
	    else if (FD_ISSET(p->ptyfd, &r.fd_val)) {
		if ((len = read(p->ptyfd, buf, IOSIZE)) == -1) {
		    if (errno == EWOULDBLOCK)
			len = 0;
		}
		if (len > 0)
		    WriteString(p, buf, len);
	    }
	    if (p->bell == BELL_ON) {
		p->bell = BELL_DONE;
		Msg(0, MakeWinMsg(BellString, n));
		if (p->monitor == MON_FOUND)
		    p->monitor = MON_DONE;
	    } else if (p->monitor == MON_FOUND) {
		p->monitor= MON_DONE;
		Msg(0, MakeWinMsg(ActivityString, n));
	    }
	}
	if (GotSignal && !status)
	    SigHandler();
    }
    /*NOTREACHED*/
}

static sig_t SigHandler() {
    while (GotSignal) {
	GotSignal = 0;
	DoWait();
    }
}

static sig_t SigChld() {
    GotSignal = 1;
}

static sig_t SigHup() {
    if (auto_detach)
	Detach(0);
    else
	Finit();
}

static sig_t SigInt() {
    char buf[1];

    *buf = (char)intrc;
    inlen[ForeNum] = 0;
    if (!help_page)
	write(fore->ptyfd, buf, 1);
}

static DoWait() {
    register n, next, pid;
    union wait wstat;

    while ((pid = wait3(&wstat, WNOHANG|WUNTRACED, (struct rusage *)0)) > 0) {
	for (n = WinList; n != -1; n = next) {
	    next = wtab[n]->WinLink;
	    if (pid == wtab[n]->wpid) {
		if (WIFSTOPPED(wstat))
		    (void) killpg(getpgrp(wtab[n]->wpid), SIGCONT);
		else
		    KillWindow(n);
	    }
	}
    }
    CheckWindows();
}

static KillWindow(n) {
    register i;

    /*
     * Remove window from linked list.
     */
    if (n == WinList) {
	RemoveStatus();
	WinList = fore->WinLink;
	fore = 0;
    } else {
	i = WinList;
	while (wtab[i]->WinLink != n)
	    i = wtab[i]->WinLink;
	wtab[i]->WinLink = wtab[n]->WinLink;
    }
    FreeWindow(wtab[n]);
    wtab[n] = 0;
    if (inlen[n] > 0) {
	inlen[n] = 0;
	inbuf_ct--;
    }
}

static CheckWindows() {
    /* If the foreground window disappeared check the head of the linked
     * list of windows for the most recently used window.
     * If no window is alive at all, exit.
     */
    if (WinList == -1)
	Finit();
    if (!fore)
	SwitchWindow(WinList);
}

static sig_t Finit() {
    register n, next;

    for (n = WinList; n != -1; n = next) {
	next = wtab[n]->WinLink;
	FreeWindow(wtab[n]);
    }
    SetTTY(0, &OldMode);
    FinitTerm();
    printf("\n[screen is terminating]\n");
    Kill(AttacherPid, SIGHUP);
    (void) unlink(SockPath);
    exit(0);
}

static InitKeytab() {
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
    ktab['t'].type = ktab[Ctrl('t')].type = KEY_TIME;
    ktab['i'].type = ktab[Ctrl('i')].type = KEY_INFO;
    ktab['m'].type = ktab[Ctrl('m')].type = KEY_LASTMSG;
    ktab['A'].type = KEY_AKA, ktab['A'].args = NULL;
    ktab['L'].type = KEY_LOGIN;
    ktab['W'].type = KEY_WIDTH;
    ktab['.'].type = KEY_TERMCAP;
    ktab[Ctrl('\\')].type = KEY_QUIT;
    ktab['d'].type = ktab[Ctrl('d')].type = KEY_DETACH;
    ktab['r'].type = ktab[Ctrl('r')].type = KEY_WRAP;
    ktab['f'].type = ktab[Ctrl('f')].type = KEY_FLOW;
    ktab['/'].type = KEY_AUTOFLOW;
    ktab['C'].type = KEY_CLEAR;
    ktab['Z'].type = KEY_RESET;
    ktab['H'].type = KEY_LOGTOGGLE;
    if (Esc != MetaEsc)
	ktab[Esc].type = KEY_OTHER;
    else
	ktab[Esc].type = KEY_IGNORE;
    ktab['M'].type = KEY_MONITOR;
    ktab['?'].type = KEY_HELP;
    for (i = 0; i <= 9; i++)
	ktab[i+'0'].type = KEY_0+i;
}

static char *ProcessInput(ibuf, pilen, obuf, polen, obuf_size)
char *ibuf,*obuf;
register int *pilen,*polen,obuf_size;
{
    register n, k;
    register char *s, *p;

    for (s = ibuf, p = obuf + *polen; *pilen > 0; --*pilen, s++) {
	if (*s == Esc) {
	    if (*pilen > 1) {
		--*pilen; s++;
		k = ktab[*s].type;
		if (*s == MetaEsc) {
		    if (*polen < obuf_size) {
			*p++ = Esc;
			++*polen;
		    }
		} else if (k >= KEY_0 && k <= KEY_9)
		    SwitchWindow(k - KEY_0);
		else switch (ktab[*s].type) {
		case KEY_TERMCAP:
		    WriteFile(0);
		    break;
		case KEY_HARDCOPY:
		    WriteFile(1);
		    break;
		case KEY_LOGTOGGLE:
#ifdef NOREUID
		    Msg(0,"Output logging is unavailable.\n");
#else
		    LogToggle();
#endif
		    break;
		case KEY_SUSPEND:
		    *pilen = 0;
		    Detach(1);
		    break;
		case KEY_SHELL:
		    if ((n = MakeWindow(shellaka, ShellArgs,
			    0, flowctl, 0, (char *)0, loginflag)) != -1)
			SwitchWindow(n);
		    break;
		case KEY_NEXT:
		    if (MoreWindows())
			SwitchWindow(NextWindow());
		    break;
		case KEY_PREV:
		    if (MoreWindows())
			SwitchWindow(PreviousWindow());
		    break;
		case KEY_KILL:
		    KillWindow(ForeNum);
		    CheckWindows();
		    break;
		case KEY_QUIT:
		    Finit();
		    /*NOTREACHED*/
		case KEY_DETACH:
		    *pilen = 0;
		    Detach(0);
		    break;
		case KEY_REDISPLAY:
		    Activate();
		    break;
		case KEY_WINDOWS:
		    ShowWindows();
		    break;
		case KEY_VERSION:
		    Msg(0, "%s  %s", ScreenVersion, AnsiVersion);
		    break;
		case KEY_TIME:
		    ShowTime();
		    break;
		case KEY_INFO:
		    ShowInfo();
		    break;
		case KEY_OTHER:
		    if (MoreWindows())
			SwitchWindow(fore->WinLink);
		    break;
		case KEY_XON:
		    if (*polen < obuf_size) {
			*p++ = Ctrl('q');
			++*polen;
		    }
		    break;
		case KEY_XOFF:
		    if (*polen < obuf_size) {
			*p++ = Ctrl('s');
			++*polen;
		    }
		    break;
		case KEY_WIDTH:
		    if (Z0) {
			fore->width = maxwidth+minwidth - fore->width;
			Activate();
		    } else
			Msg(0,"Your termcap does not specify how to change the terminal's width.");
		    break;
		case KEY_LOGIN:
#ifdef UTMPOK
		    if (fore->slot == -1) {
#ifdef USRLIMIT
			(void) lseek(utmpf, (off_t)0, 0);
			UserCount = 0;
			while (read(utmpf, &utmpbuf, sizeof (struct utmp)) > 0) {
			    if (utmpbuf.ut_name[0] != '\0')
				UserCount++;
			}
			if (UserCount >= USRLIMIT)
			    Msg(0, "User limit reached.");
			else
#endif
			{
			    fore->slot = SetUtmp(fore->tty);
			    Msg(0, "This window is now logged in.");
			}
		    } else {
			RemoveUtmp(fore->slot);
			fore->slot = -1;
			Msg(0, "This window is no longer logged in.");
		    }
#else /* !UTMPOK */
		    Msg(0,"Unable to modify /etc/utmp.\n");
#endif
		    break;
		case KEY_AKA:
		    if (!ktab[*s].args)
			InputAKA();
		    else
			strncpy(fore->cmd+fore->akapos, ktab[*s].args[0], 20);
		    break;
		case KEY_LASTMSG:
		    Msg(0, "%s", LastMsg);
		    break;
		case KEY_SCREEN:
		    if ((n = DoScreen("key", ktab[*s].args)) != -1)
			SwitchWindow(n);
		    break;
		case KEY_CREATE:
		    if ((n = MakeWindow((char *)0, ktab[*s].args,
				0, flowctl, 0, (char *)0, loginflag)) != -1)
			SwitchWindow(n);
		    break;
		case KEY_WRAP:
		    fore->wrap = !fore->wrap;
		    Msg(0, "%cwrap", fore->wrap ? '+' : '-');
		    break;
		case KEY_FLOW:
		    ToggleFlow();
		    goto flow_msg;
		case KEY_AUTOFLOW:
		    if ((fore->autoflow = !fore->autoflow) != 0
		     && fore->flow == fore->keypad)
			ToggleFlow();
		flow_msg:
		    Msg(0, "%cflow%s", fore->flow ? '+' : '-',
			fore->autoflow ? "(auto)" : "");
		    break;
		case KEY_CLEAR:
		    if (fore->state == LIT)
			WriteString(fore, "\033[2J\033[H", 7);
		    break;
		case KEY_RESET:
		    if (fore->state == LIT)
			WriteString(fore, "\033c", 2);
		    break;
		case KEY_MONITOR:
		    if (fore->monitor == MON_OFF) {
			fore->monitor = MON_ON;
			Msg(0,
			  "Window %d is now being monitored for all activity.",
			  ForeNum);
		    } else {
			fore->monitor = MON_OFF;
			Msg(0,
			  "Window %d is no longer being monitored for activity.",
			  ForeNum);
		    }
		    break;
		case KEY_HELP:
		    display_help();
		    break;
		}
	    } else
		ESCseen = 1;
	    --*pilen; s++;
	    break;
	} else if (*polen < obuf_size) {
	    *p++ = *s;
	    ++*polen;
	}
    }
    return(s);
}

static SwitchWindow(n) {
    if (!wtab[n]) {
	ShowWindows();
	return;
    }
    if (wtab[n] == fore) {
	Msg(0, "This IS window %d.", n);
	return;
    }
    SetForeWindow(n);
    if (!Detached && !help_page)
	Activate();
}

static SetForeWindow(n) {
    /*
     * If we come from another window, make it inactive.
     */
    if (fore)
	fore->active = 0;
    ForeNum = n;
    fore = wtab[n];
    if (!Detached && !help_page)
	fore->active = 1;
    /*
     * Place the window at the head of the most-recently-used list.
     */
    if ((n = WinList) != ForeNum) {
	while (wtab[n]->WinLink != ForeNum)
	    n = wtab[n]->WinLink;
	wtab[n]->WinLink = fore->WinLink;
	fore->WinLink = WinList;
	WinList = ForeNum;
    }
}

static NextWindow() {
    register struct win **pp;

    for (pp = wtab+ForeNum+1; pp != wtab+ForeNum; ++pp) {
	if (pp == wtab+MAXWIN)
	    pp = wtab;
	if (*pp)
	    break;
    }
    return pp-wtab;
}

static PreviousWindow() {
    register struct win **pp;

    for (pp = wtab+ForeNum-1; pp != wtab+ForeNum; --pp) {
	if (pp < wtab)
	    pp = wtab+MAXWIN-1;
	if (*pp)
	    break;
    }
    return pp-wtab;
}

static MoreWindows() {
    if (fore->WinLink != -1)
	return 1;
    Msg(0, "No other window.");
    return 0;
}

void Free(p) register char *p; {
    if (p)
	free(p);
}

static FreeWindow(wp) struct win *wp; {
    register i;

#ifdef UTMPOK
    RemoveUtmp(wp->slot);
#endif
#ifdef SUIDROOT
    (void) chmod(wp->tty, 0666);
    (void) chown(wp->tty, 0, 0);
#endif
    close(wp->ptyfd);
    if (wp->logfp != NULL)
	fclose(wp->logfp);
    for (i = 0; i < rows; ++i) {
	Free(wp->image[i]);
	Free(wp->attr[i]);
	Free(wp->font[i]);
    }
    Free(wp->tabs);
    Free((char *)wp->image);
    Free((char *)wp->attr);
    Free((char *)wp->font);
    free((char *)wp);
}

static MakeWindow(prog, args, aflag, flowflag, StartAt, dir, lflag)
	char *prog, **args, *dir; {
    register struct win **pp, *p;
    register char **cp;
    register n, f;
    int tf;
    int mypid;
    char ebuf[10];

    pp = wtab+StartAt;
    do {
	if (*pp == 0)
	    break;
	if (++pp == wtab+MAXWIN)
	    pp = wtab;
    } while (pp != wtab+StartAt);
    if (*pp) {
	Msg(0, "No more windows.");
	return -1;
    }

#ifdef USRLIMIT
    /*
     *	Count current number of users, if logging windows in.
     */
    if (lflag == 1) {
	(void) lseek(utmpf, (off_t)0, 0);
	UserCount = 0;
	while (read(utmpf, &utmpbuf, sizeof (struct utmp)) > 0) {
	    if (utmpbuf.ut_name[0] != '\0')
			UserCount++;
	}
	if (UserCount >= USRLIMIT) {
	    Msg(0, "User limit reached.  Window will not be logged in.");
	    lflag = 0;
	}
    }
#endif

    n = pp - wtab;
    if ((f = OpenPTY()) == -1) {
	Msg(0, "No more PTYs.");
	return -1;
    }
    (void) fcntl(f, F_SETFL, FNDELAY);
    if ((p = (struct win *)malloc(sizeof (struct win))) == 0) {
	close(f);
	Msg(0, "Out of memory.");
	return -1;
    }
    bzero((char *)p, (int)sizeof (struct win));
    p->ptyfd = f;
    p->aflag = aflag;
    if (!flowflag)
	flowflag = flowctl;
    p->flow = (flowflag != 1);
    p->autoflow = (flowflag == 3);
    if (!prog)
	prog = Filename(args[0]);
    strncpy(p->cmd, prog, MAXSTR-1);
    if ((prog = rindex(p->cmd, '|')) != NULL) {
	*prog++ = '\0';
	prog += strlen(prog);
	strcpy(prog,"(init)");
	p->akapos = prog - p->cmd;
	p->autoaka = 0;
    } else
	p->akapos = 0;
    strncpy(p->tty, TtyName, MAXSTR-1);
#ifdef SUIDROOT
    (void) chown(TtyName, real_uid, real_gid);
# ifdef UTMPOK
    (void) chmod(TtyName, lflag ? TtyMode : (TtyMode & ~022));
# else
    (void) chmod(TtyName, TtyMode);
# endif
#endif
#ifdef UTMPOK
    if (lflag == 1)
	p->slot = SetUtmp(TtyName);
    else
	p->slot = -1;
#endif

    if ((p->image = (char **)malloc((unsigned)rows * sizeof (char *))) == 0) {
nomem:
	FreeWindow(p);
	Msg(0, "Out of memory.");
	return -1;
    }
    for (cp = p->image; cp < p->image+rows; ++cp) {
	if ((*cp = malloc((unsigned)maxwidth)) == 0)
	    goto nomem;
	bclear(*cp, maxwidth);
    }
    if ((p->attr = (char **)malloc((unsigned)rows * sizeof (char *))) == 0)
	goto nomem;
    for (cp = p->attr; cp < p->attr+rows; ++cp) {
	if ((*cp = malloc((unsigned)maxwidth)) == 0)
	    goto nomem;
	bzero(*cp, maxwidth);
    }
    if ((p->font = (char **)malloc((unsigned)rows * sizeof (char *))) == 0)
	goto nomem;
    for (cp = p->font; cp < p->font+rows; ++cp) {
	if ((*cp = malloc((unsigned)maxwidth)) == 0)
	    goto nomem;
	bzero(*cp, maxwidth);
    }
    /* tabs get maxwidth+1 because 0 <= x <= maxwidth */
    if ((p->tabs = malloc((unsigned)maxwidth+1)) == 0)
	goto nomem;
    p->width = default_width;
    ResetScreen(p);
    switch (p->wpid = fork()) {
    case -1:
	Msg(errno, "fork");
	FreeWindow(p);
	return -1;
    case 0:
	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	setuid(real_uid);
	setgid(real_gid);
	if (dir && chdir(dir) == -1) {
	    SendErrorMsg("Cannot chdir to %s: %s", dir, sys_errlist[errno]);
	    exit(1);
	}
	mypid = getpid();
	ioctl(DevTty, TIOCNOTTY, (char *)0);
	if ((tf = open(TtyName, O_RDWR)) == -1) {
	    SendErrorMsg("Cannot open %s: %s", TtyName, sys_errlist[errno]);
	    exit(1);
	}
	(void) dup2(tf, 0);
	(void) dup2(tf, 1);
	(void) dup2(tf, 2);
	for (f = getdtablesize() - 1; f > 2; f--)
	    close(f);
	ioctl(0, TIOCSPGRP, &mypid);
	(void) setpgrp(0, mypid);
	SetTTY(0, &OldMode);
	NewEnv[2] = MakeTermcap(aflag);
	sprintf(ebuf, "WINDOW=%d", n);
	NewEnv[3] = ebuf;
	execvpe(*args, args, NewEnv);
	SendErrorMsg("Cannot exec %s: %s", *args, sys_errlist[errno]);
	exit(1);
    }
    /*
     * Place the newly created window at the head of the most-recently-used
     * list.  Since this spot is reserved for the foreground window, you MUST
     * call SetForeWindow (with "n" or "ForeNum") when you finish creating
     * your windows.
     */
    *pp = p;
    p->WinLink = WinList;
    WinList = n;
    return n;
}

static execvpe(prog, args, env) char *prog, **args, **env; {
    register char *path, *p;
    char buf[1024];
    char *shargs[MAXARGS+1];
    register i, eaccess = 0;

    if (prog[0] == '/')
	path = "";
    else if ((path = getenv("PATH")) == 0)
	path = DefaultPath;
    do {
	p = buf;
	while (*path && *path != ':')
	    *p++ = *path++;
	if (p > buf)
	    *p++ = '/';
	strcpy(p, prog);
	if (*path)
	    ++path;
	execve(buf, args, env);
	switch (errno) {
	case ENOEXEC:
	    shargs[0] = DefaultShell;
	    shargs[1] = buf;
	    for (i = 1; shargs[i+1] = args[i]; ++i)
		;
	    execve(DefaultShell, shargs, env);
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

static WriteFile(dump) {   /* dump==0: create .termcap, dump==1: hardcopy */
    register i, j, k;
    register char *p;
    register FILE *f;
    char fn[1024];

    if (dump)
	sprintf(fn, "hardcopy.%d", ForeNum);
    else
#ifdef SOCKDIR
	sprintf(fn, "%s/S-%s/.termcap", SockDir, LoginName);
#else
	sprintf(fn, "%s/%s/.termcap", home, SockDir);
#endif
    if (UserContext() > 0) {
	if ((f = fopen(fn, "w")) == NULL)
	    UserReturn(0);
	else {
	    if (dump) {
		cols = fore->width;
		for (i = 0; i < rows; ++i) {
		    p = fore->image[i];
		    for (k = cols-1; k >= 0 && p[k] == ' '; --k)
			;
		    for (j = 0; j <= k; ++j)
			putc(p[j], f);
		    putc('\n', f);
		}
	    } else {
		if (p = index(MakeTermcap(fore->aflag), '=')) {
		    fputs(++p, f);
		    putc('\n', f);
		}
	    }
	    (void) fclose(f);
	    UserReturn(1);
	}
    }
    if (!UserStatus())
	Msg(0, "Cannot open \"%s\"", fn);
    else
	Msg(0, "%s written to \"%s\"", dump ? "Screen image" :
		"Termcap entry", fn);
}

#ifndef NOREUID
static LogToggle() {
    char buf[1024];
    int n;

    sprintf(buf, "screenlog.%d", ForeNum);

    if (fore->logfp != NULL) {
	Msg(0, "Logfile \"%s\" closed.", buf);
	fclose(fore->logfp);
	fore->logfp = NULL;
	return;
    }
    if (UserContext() > 0) {
	if ((fore->logfp = fopen(buf, "a")) == NULL)
	    UserReturn(0);
	else if (ftell(fore->logfp) == 0)
	    UserReturn(1);
	else
	    UserReturn(2);
    }
    if (!(n = UserStatus()))
	Msg(0, "Error opening logfile \"%s\"", buf);
    else
	Msg(0, "%s logfile \"%s\"", n == 1? "Creating" : "Appending to", buf);
}
#endif

#ifdef NOREUID
int UserPID;
#else
int UserSTAT;
#endif

int UserContext() {
#ifdef NOREUID
    switch (UserPID = fork()) {
    case -1:
	Msg(errno, "fork");
	return -1;
    case 0:
	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	setuid(real_uid);
	setgid(real_gid);
	return 1;
    default:
	return 0;
    }
#else
    setreuid(eff_uid,real_uid);
    setregid(eff_gid,real_gid);
    return 1;
#endif
}

UserReturn(val) {
#if defined(SUIDROOT) && defined(NOREUID)
    exit(val);
#else
    setreuid(real_uid,eff_uid);
    setregid(real_gid,eff_gid);
    UserSTAT = val;
#endif
}

int UserStatus() {
#ifdef NOREUID
    int i, s;

    if (UserPID < 0)
	return 0;
    while ((i = wait(&s)) != UserPID)
	if (i == -1)
	    return 0;
    return ((s >> 8) & 0377);
#else
    return UserSTAT;
#endif
}

static ShowWindows() {
    char buf[1024];
    register char *s;
    register struct win **pp, *p;
    register i, OtherNum = fore->WinLink;
    register char *cmd;

    for (i = 0, s = buf, pp = wtab; pp < wtab+MAXWIN; ++i, ++pp) {
	if ((p = *pp) == 0)
	    continue;

	if (p->akapos) {
	    if (*(p->cmd+p->akapos) && *(p->cmd+p->akapos-1) != ':')
		cmd = p->cmd + p->akapos;
	    else
		cmd = p->cmd + strlen(p->cmd)+1;
	} else
		cmd = p->cmd;
	if (s - buf + 5 + strlen(cmd) > fore->width-1)
	    break;
	if (s > buf) {
	    *s++ = ' '; *s++ = ' ';
	}
	*s++ = i + '0';
	if (i == ForeNum)
	    *s++ = '*';
	else if (i == OtherNum)
	    *s++ = '-';
	if (p->monitor == MON_DONE)
	    *s++ = '@';
	if (p->bell == BELL_DONE)
	    *s++ = '!';
#ifdef UTMPOK
	if (p->slot != -1)
	    *s++ = '$';
#endif
	if (p->logfp != NULL) {
	    strcpy(s, "(L)");
	    s += 3;
	}
	*s++ = ' ';
	strcpy(s, cmd);
	s += strlen(s);
    }
    *s++ = ' ';
    *s = '\0';
    Msg(0, "%s", buf);
}

static ShowTime() {
    char buf[512];
#ifdef LOADAV
    char *p;
#endif
    struct tm *tp;
    time_t now;

    (void) time(&now);
    tp = localtime(&now);
    sprintf(buf, "%2d:%02.2d:%02.2d %s", tp->tm_hour, tp->tm_min, tp->tm_sec,
	HostName);
#ifdef LOADAV
    if (avenrun && GetAvenrun()) {
	p = buf + strlen(buf);
#ifdef LOADAV_3LONGS
	sprintf(p, " %2.2f %2.2f %2.2f", (double)loadav[0]/FSCALE,
	    (double)loadav[1]/FSCALE, (double)loadav[2]/FSCALE);
#else
#ifdef LOADAV_4LONGS
	sprintf(p, " %2.2f %2.2f %2.2f %2.2f", (double)loadav[0]/100,
	    (double)loadav[1]/100, (double)loadav[2]/100,
	    (double)loadav[3]/100);
#else
	sprintf(p, " %2.2f %2.2f %2.2f", loadav[0], loadav[1], loadav[2]);
#endif
#endif
    }
#endif
    Msg(0, "%s", buf);
}

static ShowInfo() {
    char buf[512], *p;
    register struct win *wp = fore;
    register i;

    sprintf(buf, "(%d,%d) %dcol %cflow%s %cins %corg %cwrap %capp %clog %cmon",
	wp->y+1, wp->x+1, wp->width,
	wp->flow ? '+' : '-', wp->autoflow ? "(auto)" : "",
	wp->insert ? '+' : '-', wp->origin ? '+' : '-',
	wp->wrap ? '+' : '-', wp->keypad ? '+' : '-',
	(wp->logfp != NULL) ? '+' : '-',
	(wp->monitor != MON_OFF) ? '+' : '-');
    if (ISO2022) {
	p = buf + strlen(buf);
	sprintf(p, " G%1d [", wp->LocalCharset);
	for (i = 0; i < 4; i++)
	    p[i+5] = wp->charsets[i] ? wp->charsets[i] : 'B';
	p[9] = ']';
	p[10] = '\0';
    }
    Msg(0, "%s", buf);
}

#ifdef sequent

static OpenPTY() {
	char *m, *s;
	register f;
	f = getpseudotty(&s, &m);
	strncpy(PtyName, m, sizeof PtyName);
	strncpy(TtyName, s, sizeof TtyName);
	(void) ioctl(f, TIOCFLUSH, (char *)0);
#ifdef LOCKPTY
	(void) ioctl(f, TIOCEXCL, (char *)0);
#endif
	return(f);
}

#else

static OpenPTY() {
    register char *p, *l, *d;
    register i, f, tf;

    strcpy(PtyName, PtyProto);
    strcpy(TtyName, TtyProto);
    for (p = PtyName, i = 0; *p != 'X'; ++p, ++i)
	;
#ifdef sequent
    for (l = "p"; *p = *l; ++l) { /*}*/
	for (d = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; p[1] = *d; ++d) { /*}*/
#else
    for (l = "qpr"; *p = *l; ++l) {
	for (d = "0123456789abcdef"; p[1] = *d; ++d) {
#endif
	    if ((f = open(PtyName, O_RDWR)) != -1) {
		TtyName[i] = p[0];
		TtyName[i+1] = p[1];
		if ((tf = open(TtyName, O_RDWR)) != -1) {
		    close(tf);
#ifdef LOCKPTY
		    (void) ioctl(f, TIOCEXCL, (char *)0);
#endif
		    return f;
		}
		close(f);
	    }
	}
    }
    return -1;
}
#endif

static SetTTY(fd, mp) struct mode *mp; {
    ioctl(fd, TIOCSETP, &mp->m_ttyb);
    ioctl(fd, TIOCSETC, &mp->m_tchars);
    ioctl(fd, TIOCSLTC, &mp->m_ltchars);
    ioctl(fd, TIOCLSET, &mp->m_lmode);
    ioctl(fd, TIOCSETD, &mp->m_ldisc);
}

static GetTTY(fd, mp) struct mode *mp; {
    ioctl(fd, TIOCGETP, &mp->m_ttyb);
    ioctl(fd, TIOCGETC, &mp->m_tchars);
    ioctl(fd, TIOCGLTC, &mp->m_ltchars);
    ioctl(fd, TIOCLGET, &mp->m_lmode);
    ioctl(fd, TIOCGETD, &mp->m_ldisc);
}

static SetMode(op, np) struct mode *op, *np; {
    *np = *op;
    startc = op->m_tchars.t_startc;
    stopc = op->m_tchars.t_stopc;
    if (iflag)
	intrc = op->m_tchars.t_intrc;
    else
	intrc = np->m_tchars.t_intrc = -1;
    np->m_ttyb.sg_flags &= ~(CRMOD|ECHO);
    np->m_ttyb.sg_flags |= CBREAK;
    np->m_tchars.t_quitc = -1;
    if (flowctl == 1) {
	np->m_tchars.t_intrc = -1;
	np->m_tchars.t_startc = -1;
	np->m_tchars.t_stopc = -1;
    }
    np->m_ltchars.t_suspc = -1;
    np->m_ltchars.t_dsuspc = -1;
    np->m_ltchars.t_flushc = -1;
    np->m_ltchars.t_lnextc = -1;
}

SetFlow(on) {
    if (on) {
	NewMode.m_tchars.t_intrc  = intrc;
	NewMode.m_tchars.t_startc = startc;
	NewMode.m_tchars.t_stopc  = stopc;
    } else {
	NewMode.m_tchars.t_intrc  = -1;
	NewMode.m_tchars.t_startc = -1;
	NewMode.m_tchars.t_stopc  = -1;
    }
    ioctl(0, TIOCSETC, &NewMode.m_tchars);
}

static char *GetTtyName() {
    register char *p;
    register n;

    for (p = 0, n = 0; n <= 2 && !(p = ttyname(n)); n++)
	;
    if (!p || *p == '\0')
	Msg(0, "screen must run on a tty.");
    return p;
}

static Attach(how) {
    register s, lasts, found = 0;
    register DIR *dirp;
    register DIRENT *dp;
    struct msg m;
    struct stat st;
    char last[MAXNAMLEN+1];

    if (SockName) {
	if ((lasts = MakeClientSocket(0)) == -1)
	    if (how == MSG_CONT)
		Msg(0, "Attach failed.");
	    else
		Msg(0, "There is no screen to be resumed from %s.", SockName);
	if (stat(SockPath, &st) == -1)
	    Msg(errno, "stat");
	if ((st.st_mode & 0700) != 0600)
	    Msg(0, "That screen is not detached.");
    } else {
#ifdef NFS_HACK
	setreuid(eff_uid, real_uid);
#endif
	if ((dirp = opendir(SockPath)) == NULL)
	    Msg(0, "Cannot open %s", SockPath);
	while ((dp = readdir(dirp)) != NULL) {
	    SockName = dp->d_name;
	    if (SockName[0] == '.')
		continue;
	    if ((s = MakeClientSocket(0)) != -1) {
		if (stat(SockPath, &st) == -1)
		    Msg(errno, "stat");
		if ((st.st_mode & 0700) != 0600) {
		    close(s);
		    continue;
		}
		if (found == 0) {
		    strcpy(last, SockName);
		    lasts = s;
		} else {
		    if (found == 1) {
			printf("There are detached screens on:\n");
			printf("   %s\n", last);
			close(lasts);
		    }
		    printf("   %s\n", SockName);
		    close(s);
		}
		found++;
		if (rflag == 2)
		    break;
	    }
	}
	closedir(dirp);
#ifdef NFS_HACK
	setreuid(real_uid, eff_uid);
#endif
	if (found == 0) {
	    if (rflag == 2)
		return 0;
	    else
		Msg(0, "There is no screen to be resumed.");
	}
	if (found > 1)
	    Msg(0, "Type \"screen -r host.tty\" to resume one of them.");
	strcpy(SockNamePtr, last);
	SockName = SockNamePtr;
    }
    m.type = how;
    strcpy(m.m.attach.tty, GetTtyName());
    m.m.attach.apid = getpid();
    if (write(lasts, (char *)&m, sizeof m) != sizeof m)
	Msg(errno, "write");
    return 1;
}

static sig_t AttacherFinit() {
    exit(0);
}

static sig_t SigStop() {
    Suspended = 1;
    return;
}

static Attacher() {
    signal(SIGHUP, AttacherFinit);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SigStop);
    while (1) {
	pause();
	if (Suspended) {
	    signal(SIGTSTP, SIG_DFL);
	    kill(getpid(), SIGTSTP);

	    signal(SIGTSTP, SigStop);
	    (void) Attach(MSG_CONT);
	    Suspended = 0;
	}
    }
}

static Detach(suspend) {
#ifdef UTMPOK
    register n;
#endif

    if (Detached)
	return;
    if (status)
	RemoveStatus();
    signal(SIGHUP, SIG_IGN);
    SetTTY(0, &OldMode);
    FinitTerm();
    if (suspend) {
	(void) fflush(stdout);
	Kill(AttacherPid, SIGTSTP);
    } else {
#ifdef UTMPOK
	for (n = WinList; n != -1; n = wtab[n]->WinLink)
	    if (wtab[n]->slot != -1)
		RemoveUtmp(wtab[n]->slot);
#endif
	printf("\n[detached]\n");
	Kill(AttacherPid, SIGHUP);
	AttacherPid = 0;
    }
    close(0);
    close(1);
    close(2);
    ioctl(DevTty, TIOCNOTTY,(char *)0);
    close(DevTty);
    (void) chmod(SockPath, S_IFSOCK|0600);	/* Flag detached-ness */
    Detached = 1;
    Suspended = suspend;
    if (help_page)
	help_page = 0;
    fore->active = 0;
}

static Kill(pid, sig) {
    if (pid != 0)
	(void) kill(pid, sig);
    return;
}

static GetSockName() {
    register client;
    static char buf[2*MAXSTR];

    if (!mflag && (SockName = getenv("STY")) != 0 && *SockName != '\0') {
	client = 1;
	setuid(real_uid);
	setgid(real_gid);
    } else {
	sprintf(buf, "%s.%s", HostName, Filename(GetTtyName()));
	SockName = buf;
	client = 0;
    }
    return client;
}

static MakeServerSocket() {
    register s;
    struct sockaddr_un a;
    struct stat st;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, SockName);
    strcpy(a.sun_path, SockPath);
    if (connect(s, (struct sockaddr *)&a, strlen(SockPath)+2) != -1) {
	printf("There is already a screen running on %s.\n",Filename(SockPath));
	if (stat(SockPath, &st) == -1)
	    Msg(errno, "stat");
	if ((st.st_mode & 0700) == 0600)
	    Msg(0, "To resume it, use \"screen -r\"");
	else
	    Msg(0, "It is not detached.");
	/*NOTREACHED*/
    }
    (void) unlink(SockPath);
    if (bind(s, (struct sockaddr *)&a, strlen(SockPath)+2) == -1)
	Msg(errno, "bind");
    (void) chown(SockPath, real_uid, real_gid);
    (void) chmod(SockPath, S_IFSOCK|0700);
    if (listen(s, 5) == -1)
	Msg(errno, "listen");
    return s;
}

static MakeClientSocket(err) {
    register s;
    struct sockaddr_un a;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, SockName);
    strcpy(a.sun_path, SockPath);
    if (connect(s, (struct sockaddr *)&a, strlen(SockPath)+2) == -1) {
	if (err)
	    Msg(errno, "connect: %s", SockPath);
	else {
	    close(s);
	    return -1;
	}
    }
    return s;
}

static SendCreateMsg(s, ac, av, aflag, flowflag, lflag) char **av; {
    struct msg m;
    register char *p;
    register len, n;

    m.type = MSG_CREATE;
    p = m.m.create.line;
    for (n = 0; ac > 0 && n < MAXARGS-1; ++av, --ac, ++n) {
	len = strlen(*av) + 1;
	if (p + len >= m.m.create.line+MAXLINE-1)
	    break;
	strcpy(p, *av);
	p += len;
    }
    if (!ac && *av && p+strlen(*av)+1 < m.m.create.line+MAXLINE)
	strcpy(p, *av);
    else
	*p = '\0';
    m.m.create.nargs = n;
    m.m.create.aflag = aflag;
    m.m.create.flowflag = flowflag;
    m.m.create.lflag = lflag;
    if (getwd(m.m.create.dir) == 0)
	Msg(0, "%s", m.m.create.dir);
    if (write(s, (char *)&m, sizeof m) != sizeof m)
	Msg(errno, "write");
}

#ifdef USEVARARGS
/*VARARGS1*/
static SendErrorMsg(fmt, va_alist) char *fmt; va_dcl {
    static va_list ap = 0;
#else
/*VARARGS1*/
static SendErrorMsg(fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
#endif
    register s;
    struct msg m;

    s = MakeClientSocket(1);
    m.type = MSG_ERROR;
#ifdef USEVARARGS
    va_start(ap);
    (void) vsprintf(m.m.message, fmt, ap);
    va_end(ap);
#else
    sprintf(m.m.message, fmt, p1, p2, p3, p4, p5, p6);
#endif
    (void) write(s, (char *)&m, sizeof m);
    close(s);
    sleep(2);
}

static ReceiveMsg(s) {
    register ns;
    struct sockaddr_un a;
    int left, len = sizeof a;
    struct msg m;
    char *p;

    if ((ns = accept(s, (struct sockaddr *)&a, &len)) == -1) {
	Msg(errno, "accept");
	return;
    }
    p = (char *)&m;
    left = sizeof m;
    while (left > 0 && (len = read(ns, p, left)) > 0) {
	p += len;
	left -= len;
    }
    close(ns);
    if (len == -1)
	Msg(errno, "read");
    if (left > 0)
	return;
    switch (m.type) {
    case MSG_CREATE:
	if (!Detached)
	    ExecCreate(&m);
	break;
    case MSG_CONT:
	if (m.m.attach.apid != AttacherPid || !Detached)
	    break;	/* Intruder Alert */
	/*FALLTHROUGH*/
    case MSG_ATTACH:
	if (Detached) {
	    if (kill(m.m.attach.apid, 0) == 0 &&
		    open(m.m.attach.tty, O_RDWR|O_NDELAY) == 0) {
		(void) dup(0);
		(void) dup(0);
		GetTTY(0, &OldMode);
		SetMode(&OldMode, &NewMode);
		SetTTY(0, &NewMode);
		if ((DevTty = open("/dev/tty", O_RDWR|O_NDELAY)) == -1)
		    Msg(errno, "/dev/tty");
		signal(SIGHUP, SigHup);
		if (Suspended && m.type == MSG_ATTACH)
		    if (kill(AttacherPid, SIGHUP) == 0)
			kill(AttacherPid, SIGCONT);
		AttacherPid = m.m.attach.apid;
#ifdef UTMPOK
		if (!Suspended)
		    for (ns = WinList; ns != -1; ns = wtab[ns]->WinLink)
			if (wtab[ns]->slot != -1)
			    wtab[ns]->slot = SetUtmp(wtab[ns]->tty);
#endif
		(void) chmod(SockPath, S_IFSOCK|0700);
		Detached = 0;
		Suspended = 0;
		InitTerm();
		fore->active = 1;
		Activate();
	    }
	} else {
	    Kill(m.m.attach.apid, SIGHUP);
	    Msg(0, "Not detached.");
	}
	break;
    case MSG_ERROR:
	Msg(0, "%s", m.m.message);
	break;
    default:
	Msg(0, "Invalid message (type %d).", m.type);
    }
}

static ExecCreate(mp) struct msg *mp; {
    char *args[MAXARGS];
    register n;
    register char **pp = args, *p = mp->m.create.line;

    for (n = mp->m.create.nargs; n > 0; --n) {
	*pp++ = p;
	p += strlen(p) + 1;
    }
    *pp = 0;
    if (!*p)
	p = 0;
    if ((n = MakeWindow(p, args, mp->m.create.aflag, mp->m.create.flowflag,
			 0, mp->m.create.dir, mp->m.create.lflag)) != -1)
	SwitchWindow(n);
}

static FILE *fp = NULL;
static char *rc_name;

static StartRc() {
    register argc, len;
    register char *p, *cp;
    char buf[256];
    char *args[MAXARGS], *t;

    if (RcFileName)
	rc_name = RcFileName;
    else {
	sprintf(buf, "%s/.screenrc", home);
	rc_name = SaveStr(buf);
    }
    if (access(rc_name, R_OK) == -1 || (fp = fopen(rc_name, "r")) == NULL) {
	if (RcFileName)
	    Msg(0, "Unable to open \"%s\".", RcFileName);
	else
	    free(rc_name);
	return;
    }
    if ((t = getenv("TERM")) == 0)
	Msg(0, "No TERM in environment.");
    while (fgets(buf, sizeof buf, fp) != NULL) {
	if (p = rindex(buf, '\n'))
	    *p = '\0';
	if ((argc = Parse(buf, args)) == 0)
	    continue;
	if (strcmp(args[0], "termcap") == 0) {
	    if (argc < 3 || argc > 4)
		Msg(0, "%s: termcap: incorrect number of arguments.", rc_name);
	    for (p = args[1]; p && *p; p = cp) {
		if ((cp = index(p,'|')) != 0)
		    *cp++ = '\0';
		len = strlen(p);
		if (p[len-1] == '*') {
		    if (!len-1 || !strncmp(p,t,len-1))
			break;
		} else if (!strcmp(p,t))
		    break;
	    }
	    if (!p)
		continue;
	    extra_incap = CatStr(extra_incap, args[2]);
	    if (argc == 4)
		extra_outcap = CatStr(extra_outcap, args[3]);
	}
    }
    rewind(fp);
}

static FinishRc() {
    register char *p, **pp, **ap;
    register argc;
    char buf[256];
    char *args[MAXARGS];
    char key;

    if (fp == NULL)
	return;
    ap = args;
    while (fgets(buf, sizeof buf, fp) != NULL) {
	if (p = rindex(buf, '\n'))
	    *p = '\0';
	if ((argc = Parse(buf, ap)) == 0)
	    continue;
	if (strcmp(ap[0], "escape") == 0) {
	    if (argc != 2 || !ParseEscape(ap[1]))
		Msg(0, "%s: two characters required after escape.", rc_name);
	    if (Esc != MetaEsc)
		ktab[Esc].type = KEY_OTHER;
	    else
		ktab[Esc].type = KEY_IGNORE;
	} else if (strcmp(ap[0], "chdir") == 0) {
	    p = argc < 2 ? home : ap[1];
	    if (chdir(p) == -1)
		Msg(errno, "%s", p);
	} else if (strcmp(ap[0], "shell") == 0) {
	    if (argc != 2)
		Msg(0, "%s: shell: one argument required.", rc_name);
	    ShellProg = ShellArgs[0] = SaveStr(ap[1]);
	} else if (strcmp(ap[0], "shellaka") == 0) {
	    if (argc != 2)
		Msg(0, "%s: shellaka: one argument required.", rc_name);
	    shellaka = SaveStr(ap[1]);
	} else if (strcmp(ap[0], "screen") == 0) {
	    (void) DoScreen(rc_name, ap+1);
	} else if (strcmp(ap[0], "termcap") == 0) {
	    ;		/* Already handled */
	} else if (strcmp(ap[0], "bell") == 0) {
	    if (argc != 2)
		Msg(0, "%s: bell: one argument required.", rc_name);
	    BellString = SaveStr(ap[1]);
	} else if (strcmp(ap[0], "activity") == 0) {
	    if (argc != 2)
		Msg(0, "%s: activity: one argument required.", rc_name);
	    ActivityString = SaveStr(ap[1]);
	} else if (strcmp(ap[0], "login") == 0) {
#ifdef UTMPOK
	    ParseOnOff(argc,ap,&loginflag);
#endif
	} else if (strcmp(ap[0], "flow") == 0) {
	    if (argc == 3 && ap[2][0] == 'i') {
		iflag = 1;
		argc--;
	    }
	    if (argc == 2 && ap[1][0] == 'a')
		flowctl = 3;
	    else {
		ParseOnOff(argc,ap,&flowctl);
		flowctl++;
	    }
	} else if (strcmp(ap[0], "wrap") == 0) {
	    ParseOnOff(argc,ap,&wrap);
	} else if (strcmp(ap[0], "mode") == 0) {
	    if (argc != 2)
		Msg(0, "%s: mode: one argument required.", rc_name);
	    if (!IsNum(ap[1], 7))
		Msg(0, "%s: mode: octal number expected.", rc_name);
	    (void) sscanf(ap[1], "%o", &TtyMode);
	} else if (strcmp(ap[0], "autodetach") == 0) {
	    ParseOnOff(argc,ap,&auto_detach);
	} else if (strcmp(ap[0], "bind") == 0) {
	    p = ap[1];
	    if (argc < 2 || *p == '\0')
		Msg(0, "%s: key expected after bind.", rc_name);
	    if (!(p = ParseChar(p, &key)) || *p)
		Msg(0, "%s: bind: character, ^x, or (octal) \\032 expected.",
			rc_name);
	    if (ktab[key].type != KEY_IGNORE) {
		if (ktab[key].type == KEY_SCREEN
		 || ktab[key].type == KEY_CREATE
		 || (ktab[key].type == KEY_AKA && ktab[key].args))
		    command_bindings--;
		ktab[key].type = KEY_IGNORE;
		if ((pp = ktab[key].args) != NULL) {
		    while (*pp)
			free(*pp++);
		    free((char *)pp);
		}
	    }
	    if (argc > 2) {
		for (pp = KeyNames; *pp; ++pp)
		    if (strcmp(ap[2], *pp) == 0)
			break;
		if (*pp) {
		    ktab[key].type = pp-KeyNames+1;
		    if (argc > 3)
			ktab[key].args = SaveArgs(argc-3, ap+3);
		    else
			ktab[key].args = NULL;
		    if (ktab[key].type == KEY_SCREEN
		     || (ktab[key].type == KEY_AKA && ktab[key].args))
			command_bindings++;
		} else {
		    ktab[key].type = KEY_CREATE;
		    ktab[key].args = SaveArgs(argc-2, ap+2);
		    command_bindings++;
		}
	    }
	} else
	    Msg(0, "%s: unknown keyword \"%s\"", rc_name, ap[0]);
    }
    (void) fclose(fp);
    if (!RcFileName)
	free(rc_name);
}

static Parse(buf, args) char *buf, **args; {
    register char *p = buf, **ap = args;
    register delim, argc = 0;

    argc = 0;
    for (;;) {
	while (*p && (*p == ' ' || *p == '\t')) ++p;
	if (*p == '\0' || *p == '#') {
	    *p = '\0';
	    return argc;
	}
	if (argc > MAXARGS-1)
	    Msg(0, "%s: too many tokens.", rc_name);
	delim = 0;
	if (*p == '"' || *p == '\'')
	    delim = *p++;
	argc++;
	*ap = p; *++ap = 0;
	while (*p && !(delim ? *p == delim : (*p == ' ' || *p == '\t')))
	    ++p;
	if (*p == '\0') {
	    if (delim)
		Msg(0, "%s: Missing quote.", rc_name);
	    return argc;
	}
	*p++ = '\0';
    }
}

static int ParseEscape(p) char *p; {
    if (!(p = ParseChar(p, &Esc)) || !(p = ParseChar(p, &MetaEsc)) || *p)
	return 0;
    return 1;
}

static char *ParseChar(p, cp) char *p, *cp; {
    if (*p == '^') {
	if (*++p == '?')
	    *cp = '\177';
	else if (*p >= '@')
	    *cp = Ctrl(*p);
	else
	    return 0;
	++p;
    } else if (*p == '\\' && *++p <= '7' && *p >= '0') {
	*cp = 0;
	do
	    *cp = *cp * 8 + *p - '0';
	while (*++p <= '7' && *p >= '0');
    } else
	*cp = *p++;
    return p;
}

ParseOnOff(argc,ap,var) int argc; char *ap[]; int *var; {
    register num = -1;

    if (argc == 2 && ap[1][0] == 'o') {
	if (ap[1][1] == 'f')
	    num = 0;
	else if (ap[1][1] == 'n')
	    num = 1;
    }
    if (num < 0)
	Msg(0, "%s: %s: invalid argument.", rc_name, ap[0]);
    *var = num;
}

static char *CatStr(str1, str2) register char *str1, *str2; {
    register char *cp;
    register len1, len2, add_colon;

    len2 = strlen(str2);
    add_colon = (str2[len2-1] != ':');
    if (str1) {
	len1 = strlen(str1);
	cp = realloc(str1, (unsigned)len1 + len2 + add_colon + 1);
	str1 = cp + len1;
    } else {
	if (!len2)
	    return 0;
	cp = malloc((unsigned)len2 + add_colon + 1);
	str1 = cp;
    }
    if (cp == 0)
	Msg(0, "Out of memory.");
    strcpy(str1, str2);
    if (add_colon)
	strcat(str1, ":");

    return cp;
}

static char *SaveStr(str) register char *str; {
    register char *cp;

    if ((cp = malloc((unsigned)strlen(str) + 1)) == 0)
	Msg(0, "Out of memory.");
    strcpy(cp, str);

    return cp;
}

static char **SaveArgs(argc, argv) register argc; register char **argv; {
    register char **ap, **pp;

    if ((pp = ap = (char**)malloc((unsigned)(argc+1) * sizeof (char**))) == 0)
	Msg(0, "Out of memory.");
    while (argc--)
	*pp++ = SaveStr(*argv++);
    *pp = 0;
    return ap;
}

static MakeNewEnv() {
    register char **op, **np;
    static char buf[MAXSTR];

    for (op = environ; *op; ++op)
	;
    NewEnv = np = (char**)malloc((unsigned)(op-environ+4+1) * sizeof (char**));
    if (!NewEnv)
	Msg(0, "Out of memory.");
    if (strlen(SockName) > MAXSTR-5)
	SockName = "?";
    sprintf(buf, "STY=%s", SockName);
    *np++ = buf;
    *np++ = Term;
    np += 2;			/* leave room for TERMCAP and WINDOW */
    for (op = environ; *op; ++op) {
	if (!IsSymbol(*op, "TERM") && !IsSymbol(*op, "TERMCAP")
	 && !IsSymbol(*op, "STY") && !IsSymbol(*op, "WINDOW")
	 && !IsSymbol(*op, "SCREENCAP"))
	    *np++ = *op;
    }
    *np = 0;
}

static IsSymbol(e, s) register char *e, *s; {
    register char *p;
    register n;

    for (p = e; *p && *p != '='; ++p)
	;
    if (*p) {
	*p = '\0';
	n = strcmp(e, s);
	*p = '=';
	return n == 0;
    }
    return 0;
}

#ifdef USEVARARGS
/*VARARGS2*/
Msg(err, fmt, va_alist) int err; char *fmt; va_dcl {
    static va_list ap = 0;
#else
/*VARARGS2*/
Msg(err, fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
#endif
    char buf[1024];
    register char *p = buf;

    if (Detached)
	return;
#ifdef USEVARARGS
    va_start(ap);
    (void) vsprintf(p, fmt, ap);
    va_end(ap);
#else
    sprintf(p, fmt, p1, p2, p3, p4, p5, p6);
#endif
    if (err) {
	p += strlen(p);
	if (err > 0 && err < sys_nerr)
	    sprintf(p, ": %s", sys_errlist[err]);
	else
	    sprintf(p, ": Error %d", err);
    }
    if (HasWindow)
	MakeStatus(buf);
    else {
	printf("%s\r\n", buf);
	Kill(AttacherPid, SIGHUP);
	exit(1);
    }
}

bclear(p, n) char *p; {
    bcopy(blank, p, n);
}

static char *Filename(s) char *s; {
    register char *p;

    p = s + strlen(s) - 1;
    while (p >= s && *p != '/') --p;
    return ++p;
}

static IsNum(s, base) register char *s; register base; {
    for (base += '0'; *s; ++s)
	if (*s < '0' || *s > base)
	    return 0;
    return 1;
}

static char *MakeWinMsg(s,n) register char *s; {
    static char buf[MAXSTR];
    register char *p = buf;

    for (; *s && p < buf+MAXSTR-1; s++,p++)
	switch (*s) {
	case '%':
	    *p = n + '0';
	    break;
	case '~':
	    *p = BELL;
	    break;
	default:
	    *p = *s;
	    break;
	}
    *p = '\0';
    return buf;
}

#ifdef UTMPOK

static InitUtmp() {

    if ((utmpf = open(UtmpName, O_WRONLY)) == -1) {
	if (errno != EACCES)
	    Msg(errno, UtmpName);
	return;
    }
    utmp = 1;
}

static SetUtmp(name) char *name; {
    register char *p;
    register struct ttyent *tp;
    register slot = 1;
    struct utmp u;

    if (!utmp)
	return 0;
    if (p = rindex(name, '/'))
	++p;
    else p = name;
    setttyent();
    while ((tp = getttyent()) != NULL && strcmp(p, tp->ty_name) != 0)
	++slot;
    if (tp == NULL)
	return 0;
    strncpy(u.ut_line, p, 8);
    strncpy(u.ut_name, LoginName, 8);
    u.ut_host[0] = '\0';
    (void) time(&u.ut_time);
    (void) lseek(utmpf, (off_t)(slot * sizeof u), 0);
    (void) write(utmpf, (char *)&u, sizeof u);
    return slot;
}

static RemoveUtmp(slot) {
    struct utmp u;

    if (slot) {
	bzero((char *)&u, sizeof u);
	(void) lseek(utmpf, (off_t)(slot * sizeof u), 0);
	(void) write(utmpf, (char *)&u, sizeof u);
    }
}

#endif /* UTMPOK */

#ifndef GETTTYENT

static setttyent() {
    struct stat s;
    register f;
    register char *p, *ep;

    if (ttnext) {
	ttnext = tt;
	return;
    }
    if ((f = open(ttys, O_RDONLY)) == -1 || fstat(f, &s) == -1)
	Msg(errno, ttys);
    if ((tt = malloc((unsigned)s.st_size + 1)) == 0)
	Msg(0, "Out of memory.");
    if (read(f, tt, s.st_size) != s.st_size)
	Msg(errno, ttys);
    close(f);
    for (p = tt, ep = p + s.st_size; p < ep; ++p)
	if (*p == '\n') *p = '\0';
    *p = '\0';
    ttnext = tt;
}

static struct ttyent *getttyent() {
    static struct ttyent t;

    if (*ttnext == '\0')
	return NULL;
    t.ty_name = ttnext + 2;
    ttnext += strlen(ttnext) + 1;
    return &t;
}

#endif /* GETTTYENT */

#ifdef LOADAV

static InitKmem() {
    if ((kmemf = open(KmemName, O_RDONLY)) == -1)
	return;
    nl[0].n_name = AvenrunSym;
    nlist(UnixName, nl);
    if (nl[0].n_type == 0 || nl[0].n_value == 0)
	return;
    avenrun = 1;
}

static GetAvenrun() {
    if (lseek(kmemf, (off_t)nl[0].n_value, 0) == (off_t)-1)
	return 0;
    if (read(kmemf, (char *)loadav, sizeof loadav) != sizeof loadav)
	return 0;
    return 1;
}

#endif /* LOADAV */

#ifndef USEBCOPY
bcopy(s1, s2, len) register char *s1, *s2; register len; {
    if (s1 < s2 && s2 < s1 + len) {
	s1 += len; s2 += len;
	while (len-- > 0)
	    *--s2 = *--s1;
    } else
	while (len-- > 0)
	    *s2++ = *s1++;
}
#endif /* USEBCOPY */

DoScreen(fn, av) char *fn, **av; {
    register flowflag, num, lflag = loginflag;
    register char *aka = NULL;
    char *args[2];

    flowflag = flowctl;
    while (av && *av && av[0][0] == '-') {
	switch (av[0][1]) {
	case 'n':			/* backward compatibility */
	    flowflag = 1;
	    break;
	case 'f':
	    switch (av[0][2]) {
	    case 'n':
	    case '0':
		flowflag = 1;
		break;
	    case 'y':
	    case '1':
	    case '\0':
		flowflag = 2;
		break;
	    case 'a':
		flowflag = 3;
		break;
	    default:
		break;
	    }
	    break;
	case 'k':
	    if (av[0][2])
		aka = &av[0][2];
	    else if (*++av)
		aka = *av;
	    else
		--av;
	    break;
	case 'l':
	    switch (av[0][2]) {
	    case 'n':
	    case '0':
		lflag = 0;
		break;
	    case 'y':
	    case '1':
	    case '\0':
		lflag = 1;
		break;
	    default:
		break;
	    }
	    break;
	default:
	    Msg(0, "%s: screen: invalid option -%c.", fn, av[0][1]);
	    break;
	}
	++av;
    }
    num = 0;
    if (av && *av && IsNum(*av, 10)) {
	num = atoi(*av);
	if (num < 0 || num > MAXWIN-1)
	    Msg(0, "%s: illegal screen number %d.", fn, num);
	++av;
    }
    if (!av || !*av) {
	av = args;
	av[0] = ShellProg;
	av[1] = NULL;
	if (!aka)
	    aka = shellaka;
    }
    return MakeWindow(aka, av, 0, flowflag, num, (char *)0, lflag);
}

/* Esc-char is not consumed. All others are. Esc-char, space, and return end */
process_help_input(ppbuf,plen)
char **ppbuf;
int *plen;
{
    int done = 0;
    
    while (!done && *plen > 0) {
	switch (**ppbuf) {
	case ' ':
	    if ((help_page-1)*16 < command_bindings) {
		display_help();
		break;
	    }
	    /*FALLTHROUGH*/
	case '\r':
	case '\n':
	    done = 1;
	    break;
	default:
	    if (**ppbuf == Esc) {
		done = 1;
		continue;
	    }
	    break;
	}
	++*ppbuf;
	--*plen;
    }
    if (done) {
	help_page = 0;
	fore->active = 1;	/* Turn the foreground window back on. */
	Activate();
    }
}

display_help() {
    int row, col, maxrow, num_names, n, key;
    char buf[256], Esc_buf[5];

    if (!help_page++)
	fore->active = 0;

    /* Display the help screen */
    init_help_screen();

    printf("\n                 SCREEN Key Bindings, page %d of %d.\r\n\n",
	help_page,(command_bindings+15)/16+1);

    if (Esc == '"')
	strcpy(Esc_buf,"'");
    else
	strcpy(Esc_buf,"\"");
    add_key_to_buf(Esc_buf,Esc);
    Esc_buf[strlen(Esc_buf)-1] = Esc_buf[0];
    if (help_page == 1) {
	if (MetaEsc == '"')
	    strcpy(buf,"'");
	else
	    strcpy(buf,"\"");
	add_key_to_buf(buf,MetaEsc);
	buf[strlen(buf)-1] = buf[0];
	printf("          Command key:  %s          Literal %s:  %s\r\n\n",
		Esc_buf,Esc_buf,buf);

	maxrow = ((num_names = sizeof KeyNames / sizeof (char *) - 2) + 2) / 3;
	for (row = 0; row < maxrow; row++) {
	    for (col = 0; col < 3 && (n = maxrow*col+row) < num_names; col++) {
		buf[0] = '\0';
		for (key = 0; key < 128; key++)
		    if (ktab[key].type == n + 2
		     && (n + 2 != KEY_AKA || !ktab[key].args))
			add_key_to_buf(buf,key);
		buf[14] = '\0';
		/* Format is up to 9 chars of name, 2 spaces, 14 chars of key
		 * bindings, and a space.
		 */
		printf("%-9s  %-14s ", KeyNames[n+1], buf);
	    }
	    printf("\r\n");
	}
	putchar('\n');
	command_search = 0;
    } else {
	char **pp, *cp;

	cols = fore->width;
	maxrow = (help_page-1)*16;
	for (row = maxrow-16; row < maxrow; row++) {
	    if (row < command_bindings) {
		while ((key = ktab[command_search].type) != KEY_CREATE
			&& key != KEY_SCREEN
			&& (key != KEY_AKA || !ktab[command_search].args) )
		    command_search++;
		buf[0] = '\0';
		add_key_to_buf(buf,command_search);
		printf("%-4s",buf);
		col = 4;
		if (key != KEY_CREATE) {
		    col += strlen(KeyNames[key-1])+1;
		    printf("%s ",KeyNames[key-1]);
		}
		pp = ktab[command_search++].args;
		while (pp && (cp = *pp) != NULL) {
		    if (!*cp || index(cp, ' ')) {
			if (index(cp, '\''))
			    *buf = '"';
			else
			    *buf = '\'';
			sprintf(buf+1, "%s%c", cp, *buf);
			cp = buf;
		    }
		    if ((col += strlen(cp)+1) >= cols) {
			col = cols - (col - (strlen(cp)+1)) - 2;
			if (col >= 0) {
			    n = cp[col];
			    cp[col] = '\0';
			    printf("%s$",*pp);
			    cp[col] = (char)n;
			}
			break;
		    }
		    printf("%s%c", cp, (cols-col != 1 || !pp[1])? ' ' : '$' );
		    pp++;
		}
		printf("\r\n");
	    } else
		putchar('\n');
	}
    }

    printf("\n[Press Space %s Return to end; %s to begin a command.]\r\n",
	(help_page-1)*16 < command_bindings ? "for next page;" : "or",Esc_buf);
}

add_key_to_buf(buf,key) char *buf; int key; {
    switch (key) {
    case ' ':
	strcat(buf,"sp ");
	break;
    case 0x7f:
	strcat(buf,"^? ");
	break;
    default:
	if (key < ' ')
	    sprintf(buf + strlen(buf),"^%c ",(key | 0x40));
	else
	    sprintf(buf + strlen(buf),"%c ",key);
	break;
    }
}
