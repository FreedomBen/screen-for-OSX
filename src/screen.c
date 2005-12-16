
/* Copyright (c) 1987,1988 Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 *
 * Modified (Dec90) by hackers@immd4: copy & paste added
 * Power-detach added.
 * tgoto resize bug fixed.
 * lock now in attacher
 * intelligent password
 */

#ifndef NO_SCCS_ID
static char sccs_id[] = "@(#)screen.c   1.3 20:15:27 91/02/25";
#endif /* NO_SCCS_ID */

#define RETRY_WHEN_R_FAILED /**/

static char ScreenVersion[] = "screen 2.1 25-Feb-91"; /*was 3.3.89.jw.*/

#include "screen.h"
#include <stdio.h>
#ifdef TCGETA
# ifdef hpux
# undef TCGETA /* how nice .jw. */
# endif
#include <sys/termio.h>
#else
#include <sgtty.h>
#endif

#ifdef hpux
#include <sys/bsdtty.h>
#include <sys/ioctl.h>
#endif hpux
#include <signal.h>
#include <errno.h>
#include <ctype.h>
/* #include <sys/types.h> */
/* #include <utmp.h> * screen.h already includes that */
#include <pwd.h>
#include <nlist.h>
#include <fcntl.h>
#include <sys/time.h>

#include <sys/file.h> 
#include <sys/wait.h>
#ifndef NAMEDPIPE
#include <sys/socket.h>
#endif NAMEDPIPE
#include <sys/un.h>
#include <sys/stat.h>
#ifdef NEEDNDIR
#include <ndir.h>
#else
#include <sys/dir.h>
#endif
#if defined(SUNLOADAV) || defined(SYSV)
#include <sys/param.h>
#include <sys/ioctl.h>
#ifdef KVMLIB
#include <kvm.h>
#endif /* KVMLIB */
#endif

#ifdef GETTTYENT
#   include <ttyent.h>
#else
    static struct ttyent { char *ty_name; } *getttyent();
    static char *tt, *ttnext;
    static char ttys[] = "/etc/ttys";
#endif

#define MAXWIN     10
#define MSGWAIT     5


#define Ctrl(c) ((c)&037)

extern char *copybuffer, *blank, Term[], **environ;
extern struct { int rows, cols; } TS[2];
extern copylen, trows, tcols;
extern ISO2022;
extern status;
extern time_t TimeDisplayed;
extern char AnsiVersion[];
extern char CopyPasteVersion[];
extern flowctl;
extern errno;
extern sys_nerr;
extern char *sys_errlist[];
extern char *index(), *rindex(), *malloc(), *getenv(), *MakeTermcap();
extern char *getlogin(), *ttyname();
extern MarkRoutine(), ChangeTermSize(), ChangeWindowSize(), ChangeTerm();
extern struct win * SetAnsiCurr();
extern void bzero();
extern int  NWokay;
#ifdef UTMP5
static struct utmp *SetUtmp();
#else
static SetUtmp();
#endif /* UTMP5 */
extern struct passwd *getpwuid();
extern char *getlogin(), *getpass();
static void AttacherFinit();
static void SigHandler(), SigHup(), SigChld(), CoreDump(), Finit();
static DoWait(), KillWindow(), CheckWindows(), InitKeytab();
static ProcessInput(), SwitchWindow(), SetCurrWindow(), NextWindow();
static PreviousWindow(), MoreWindows(), FreeWindow(), MakeWindow();
static execvpe(), ReadFile(), WriteFile(), ShowWindows(), ShowInfo(), OpenPTY();
static SetTTY(), GetTTY(), SetMode(), Attach(), Attacher(), Detach();
static Kill(), GetSockName(), MakeServerSocket(), MakeClientSocket();
static SendCreateMsg(), SendErrorMsg(), ReceiveMsg(), ExecCreate();
static ReadRc(), Parse(), MakeNewEnv(), IsSymbol(), IsNum(), InitUtmp();
static RemoveUtmp(), InitKmem(), setttyent();
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
static void SigWinch();
#endif
static char *MakeBellMsg(), *Filename(), **SaveArgs(), *GetTtyName();
#ifdef LOCK
static void LockTerminal();
static char LockEnd[] = "Welcome back to screen !!\n";
#endif LOCK

static char PtyName[32], TtyName[32];
static char *ShellProg;
static char *ShellArgs[2];
static char inbuf[IOSIZE];
static inlen;
static pastelen;
static char *pastebuffer;
static ESCseen;
static GotSignal;
#ifdef LOCK
#ifndef LOCKCMD
#define LOCKCMD "/local/bin/lck" /* /usr/local no more.jw.*/
#endif /* LOCKCMD */
static char DefaultLock[] = LOCKCMD;
#endif LOCK
static char DefaultShell[] = "/bin/csh";

#ifndef DEFAULTPATH
#define DEFAULTPATH ":/usr/ucb:/bin:/usr/bin"
#endif /* DEFAULTPATH */
static char DefaultPath[] = DEFAULTPATH;
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
#ifndef PTYCHAR1
#define PTYCHAR1 "pqrst"
#endif PTYCHAR1
#ifndef PTYCHAR2
#define PTYCHAR2 "01213456789abcdef"
#endif PTYCHAR2
static int TtyMode = 0622;
static char SockPath[512];
#ifdef SOCKDIR
static char SockDir[] = SOCKDIR;
#else
static char SockDir[] = ".screen";
#endif
static char *SockNamePtr, *SockName;
static ServerSocket;
static char *NewEnv[MAXARGS];
static char Esc = Ctrl('a');
static char MetaEsc = 'a';
static char home[2048];
static HasWindow;
static utmp;
#ifndef UTMP5
static utmpf;
static char UtmpName[] = "/etc/utmp";
#endif /* UTMP5 */
static char LoginName[20];
/* static char LoginUid[20]; * unused */
static char Password[20];
static int  CheckPassword = 0;
static char *BellString = "Bell in window %";
static mflag, nflag, Dflag, dflag, fflag, rflag;
static char HostName[MAXSTR];
static Detached;
static AttacherPid;     /* Non-Zero in child if we have an attacher */
#ifndef SYSV
static DevTty;
#endif SYSV
#ifdef LOADAVSYSCALL
static int *loadav;
#endif LOADAVSYSCALL
#ifdef LOADAV
static GetAvenrun();
#ifdef KVMLIB
    static kvm_t *kmemf;
#else /* KVMLIB */
    static char KmemName[] = "/dev/kmem";
#ifndef KERNELNAME
#define KERNELNAME "/vmunix"
#endif KERNELNAME
    static char UnixName[] = KERNELNAME;
    static kmemf;
#endif KVMLIB
#ifdef alliant
    static char AvenrunSym[] = "_Loadavg";
#else
#ifdef hpux
    static char AvenrunSym[] = "avenrun";
#else hpux
    static char AvenrunSym[] = "_avenrun";
#endif hpux
#endif
    static struct nlist nl[2];
    static avenrun;
#ifdef SUNLOADAV
    long loadav[3];
#else
#ifdef alliant
    long loadav[4];
#else
    double loadav[3];
#endif
#endif

#endif

struct mode {
#ifdef TIOCSWINSZ
    struct winsize ws;
#endif
#ifdef TCGETA
    struct termio tio;
#else
    struct sgttyb m_ttyb;
    struct tchars m_tchars;
    struct ltchars m_ltchars;
    int m_ldisc;
    int m_lmode;
#endif
} OldMode, NewMode;

static struct win *curr;
struct win *other;
static CurrNum, OtherNum;
struct win *wtab[MAXWIN];
#define BUFFERFILE  "/tmp/screenbuffer"

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
#define KEY_DETACH        26
#define KEY_LOGIN         27
#define KEY_UNLOGIN       28
#define KEY_LASTMSG       29
#define KEY_SIZE80        30
#define KEY_SIZE132       31
#define KEY_SIZETOGGLE    32
#define KEY_COPY          33
#define KEY_PASTE         34
#define KEY_BANGBANG      35
#define KEY_POW_DETACH    36
#define KEY_WRITE_BUFFER  37
#define KEY_READ_BUFFER   38
#define KEY_VBELL         39

#ifdef LOCK
#define KEY_LOCK          40
#define KEY_CREATE        41        /* KEY_CREATE must be the last one */
#else
#define KEY_CREATE        40        /* KEY_CREATE must be the last one */
#endif

#ifndef LOGINFLAG
#define LOGINFLAG 1
#endif LOGINFLAG

struct attr defattr = { 24, 80, LOGINFLAG, 0, 0, "screen", "local"};

struct key {
    int type;
    struct attr *attr;
    char **args;
} ktab[256];

char *KeyNames[] = {
    "hardcopy", "suspend", "shell", "next", "prev", "kill", "redisplay",
    "windows", "version", "other", "select0", "select1", "select2", "select3",
    "select4", "select5", "select6", "select7", "select8", "select9",
    "xon", "xoff", "info", "termcap", "quit", "detach",
    "login", "unlogin", "lastmsg",
    "size80", "size132", "sizetoggle",
    "copy","paste","bangbang","detach_kill",
#ifdef LOCK
    "lockscreen",
#endif /* LOCK */
    0
};

extern  int Screen_Columns[];
extern  int Default_Size;

#ifdef hpux
/*
 * hpux has berkeley signal semantics if we use sigvector,
 * but not, if we use signal, so we define our own signal() routine.
 */
void (*signal(sig, func))() void (*func)();
int sig;
{
    struct sigvec osv, sv;

    sv.sv_handler = func;
    sv.sv_mask = sigmask( sig);
    sv.sv_flags = SV_BSDSIG;
    if( sigvector(sig, &sv, &osv) < 0)
    return(BADSIG);
    return( osv.sv_handler);
}
#endif hpux

#ifndef USEBCOPY
void bcopy (s1, s2, len)
register char *s1, *s2; register len;
{
    if (s1 < s2 && s2 < s1 + len)
    {   s1 += len; s2 += len;
        while (len-- > 0)
        {   *--s2 = *--s1;
        }
    } else
    {   while (len-- > 0)
        {   *s2++ = *s1++;
        }
    }
}
#endif

void bclear (p, n)
char *p;
{
    bcopy (blank, p, n);
}

main (ac, av)
char **av;
{
    register n, len;
    register struct win **pp, *p;
    char *ap;
    struct passwd *ppp = 0;
    int s, r, w, x = 0;
    int aflag = 0;
    struct timeval tv;
    time_t now;
/* jw & rk */
#if defined _MODE_T
    mode_t old_umask;
#else
    int old_umask;
#endif
    char buf[IOSIZE], *myname = (ac == 0) ? "screen" : av[0];
    char *lcp, rc[256];
    struct stat st;

    while (ac > 0)
    {   ap = *++av;
        if (--ac > 0 && *ap == '-')
        {   switch (ap[1])
/* well, we break margins here... jw. */
{
    case 'c':        /* Compatibility with older versions. */
        break;
    case 'a':
        aflag = 1;
        break;
    case 'l':
        defattr.lflag  = 1;
        break;
    case 'u':
        defattr.lflag = 0;
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
    case 'v':
        if (ap[2] == 'b')
        {   defattr.visualbell = 1;
            break;
        }
        else
            goto help;
        break;
    case 'D':
        Dflag = 1; /* no break ?? */
        break;
    case 'd':
        dflag = 1; /* no break ?? */
        break;
    case 'r':
        rflag = 1;
        if (ap[2])
        {   SockName = ap+2;
            if (ac != 1) goto help;
        }
        else if (--ac == 1)
        {   SockName = *++av;
        }
        else if (ac != 0)
            goto help;
        break;
    case 'e':
        if (ap[2])
        {   ap += 2;
        }
        else
        {   if (--ac == 0) goto help;
            ap = *++av;
        }
        if (strlen (ap) != 2)
            Msg (0, "Two characters are required with -e option.");
        Esc = ap[0];
        MetaEsc = ap[1];
        break;
    default:
    help:
        Msg (0, "Usage: %s [-a] [-f] [-l | -u] [-m] [-n] [-vb] [-e xy] [cmd args]\n\
 or: %s [-d | -D] -r [host.tty]", myname, myname);
}
/* back in margin again... jw. */
        } else break;
    }
    if (nflag && fflag)
        Msg (0, "-f and -n are conflicting options.");
    if ((ShellProg = getenv ("SHELL")) == 0)
        ShellProg = DefaultShell;
    ShellArgs[0] = ShellProg;
    if (ac == 0)
    {   ac = 1;
        av = ShellArgs;
    }
    if (getenv ("HOME") != (char *) 0)
    strcpy( home, getenv("HOME"));

    lcp = getlogin();
    if( (lcp == (char *) 0) || (lcp[0] == '\0') )
    {   if((ppp = getpwuid(getuid())) == (struct passwd *) 0)
        {   Msg(0,"An alarm sounds through the dungeon ... \nWarning, the kops are coming.");
            exit(1);
        }
        else
        {   lcp = ppp->pw_name;
        }
    }
    if (((ppp!=(struct passwd *) 0) && (lcp == ppp->pw_name)) ||
        ((ppp = getpwnam(lcp)) != (struct passwd *) 0))
    {   if( ppp->pw_uid != getuid())
        {   Msg(0,"You cannot resist to mimic a pile of gold.\nThe system ignores you.");
            exit(1);
        }
    }
/*  else
    {   we did not even get a response from getpwnam .....
    }
*/
    strncpy (LoginName, lcp, sizeof LoginName);
    strncpy (Password, ppp->pw_passwd, sizeof Password);
    if( *home  == '\0')
        strcpy( home, ppp->pw_dir);

#if defined _MODE_T
    old_umask=umask(0);
#else    
    if ((old_umask=umask(0)) == -1)
        Msg (errno, "Cannot change umask to zero");
#endif
#ifdef SOCKDIR
    if (stat (SOCKDIR, &st) == -1)
    {   if (errno == ENOENT)
        {   if (mkdir (SOCKDIR, geteuid() ? 0777 : 0755 ) == -1)
                Msg (errno, "Cannot make directory %s", SOCKDIR);
            (void) chown (SOCKDIR, 0, 0);
        }
        else
            Msg (errno, "Cannot get status of %s", SOCKDIR);
    }
    else
    {   if ((st.st_mode & S_IFMT) != S_IFDIR)
            Msg (0, "%s is not a directory.", SOCKDIR);
        if ( geteuid() && ((st.st_mode & 0777) != 0777))
            Msg (0, "Directory %s must have mode 777.", SOCKDIR);
    }
    sprintf (SockPath, "%s/%s", SockDir, LoginName);
#else
    sprintf (SockPath, "%s/%s", home, SockDir);
#endif
    if (stat (SockPath, &st) == -1)
    {   if (errno == ENOENT)
        {   if (mkdir (SockPath, 0700) == -1)
                Msg (errno, "Cannot make directory %s", SockPath);
            (void) chown (SockPath, getuid (), getgid ());
        }
        else
            Msg (errno, "Cannot get status of %s", SockPath);
    }
    else
    {   if ((st.st_mode & S_IFMT) != S_IFDIR)
            Msg (0, "%s is not a directory.", SockPath);
        if ((st.st_mode & 0777) != 0700)
            Msg (0, "Directory %s must have mode 700.", SockPath);
        if (st.st_uid != getuid ())
            Msg (0, "You are not the owner of %s.", SockPath);
    }
    (void) gethostname (HostName, MAXSTR);
    HostName[MAXSTR-1] = '\0';
    if (ap = index (HostName, '.'))
        *ap = '\0';

	strncpy(defattr.sname,HostName,20);

    strcat (SockPath, "/");
    SockNamePtr = SockPath + strlen (SockPath);
    GetTTY (0, &OldMode);
    if (rflag) 
#ifdef RETRY_WHEN_R_FAILED
    {   if (!Attach(MSG_ATTACH))
        {   Attacher ();
            /*NOTREACHED*/
        }
/*      else
 *          we continue, as if there were no -r specified...jw.
 */
    }
#else
    {   (void) Attach (MSG_ATTACH);
        Attacher();
        /*NOTREACHED*/
    }
#endif
    if (GetSockName ())
    {   s = MakeClientSocket (1);
        SendCreateMsg (s, ac, av, aflag, &defattr);
        close (s);
        exit (0);
    }
#ifndef SYSV
    if ((DevTty = open ("/dev/tty", O_RDWR|O_NDELAY)) == -1)
    Msg (errno, "/dev/tty");
#endif SYSV
/* jw: 'hope it's correct to restore umask here.
 *  (we need to restore it somewhere ...)
 */
    (void)umask(old_umask);
    switch (fork ())
    {   case -1:
            Msg (errno, "fork");
            /*NOTREACHED*/
        case 0:
            break;
        default:
            Attacher ();
            /*NOTREACHED*/
    }
    AttacherPid = getppid ();
    ServerSocket = s = MakeServerSocket ();
    InitTerm ();
    if (fflag)
        flowctl = 1;
    else if (nflag)
        flowctl = 0;
    MakeNewEnv ();
    InitUtmp ();
#ifdef LOADAV
    InitKmem ();
#endif
    signal (SIGHUP, SigHup);
    signal (SIGINT, Finit);
    signal (SIGQUIT, Finit);
    signal (SIGTERM, Finit);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    if( getuid() != geteuid())
    {
/*
 * if running with s-bit, we must install a special
 * signal handler routine that resets the s-bit, so
 * that we get a core file anyway.
 */
        signal (SIGBUS, CoreDump);
        signal (SIGSEGV, CoreDump);
    }
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
    signal (SIGWINCH, SigWinch);
#endif
    sprintf (rc, "%.*s/.screenrc", 245, home);
    InitKeytab ();
    ReadRc (rc);
    if ((n = MakeWindow (*av, av, aflag, 0, (char *)0, &defattr)) == -1)
    {   SetTTY (0, &OldMode);
        FinitTerm ();
        Kill (AttacherPid, SIGHUP);
        exit (1);
    }
    SetCurrWindow (n);
    HasWindow = 1;
    SetMode (&OldMode, &NewMode);
    SetTTY (0, &NewMode);
#ifdef SYSV
    signal (SIGCLD, SigChld);
#else SYSV
    signal (SIGCHLD, SigChld);
#endif SYSV
    tv.tv_usec = 0;
    while (1)
    {   /*
         *  check to see if message line should be removed
         */
        if (status)
        {   time (&now);
            if (now - TimeDisplayed < MSGWAIT) 
                tv.tv_sec = MSGWAIT - (now - TimeDisplayed);
            else
                RemoveStatus (curr);
        }
        /*
         *  check for I/O on all available I/O descriptors
         */
        r = 0;
        w = 0;
        if (inlen||pastelen)
            w |= 1 << curr->ptyfd;
        else
            r |= 1 << 0;
        for (pp = wtab; pp < wtab+MAXWIN; ++pp) 
        {   if (!(p = *pp))
                continue;
            if ((*pp)->active && status)
                continue;
            if ((*pp)->outlen > 0)
                continue;
            r |= 1 << (*pp)->ptyfd;
        }
        r |= 1 << s;
        (void) fflush (stdout);
        if (GotSignal && !status) 
        {   SigHandler ();
            continue;
        }
        if (select (32, &r, &w, &x, status ? &tv : (struct timeval *)0) == -1) 
        {   if (errno == EINTR)
            {   errno = 0;
                continue;
            }
            HasWindow = 0;
            Msg (errno, "select");
            /*NOTREACHED*/
        }
        /*
         *  handle signals to main process
         */
        if (GotSignal && !status) 
        {   SigHandler ();
            continue;
        }
        /*
         *  messages from current window removes the status line
         */
        if (r & 1 << s)
        {   RemoveStatus (curr);
            ReceiveMsg (s);
        }
        /*
         *  user input removes the status line
         *  read from real terminal
         */
        if (r & 1 << 0)
        {   RemoveStatus (curr);
            if (ESCseen)
            {   inbuf[0] = Esc;
                inlen = read (0, inbuf+1, IOSIZE-1) + 1;
                if (inlen <= 0)
                {   perror("read");
                    Finit();
                }
                ESCseen = 0;
            }
            else
            {   inlen = read (0, inbuf, IOSIZE);
                if (inlen <= 0)
                {   perror("read");
                    Finit();
                }
            }
            if (inlen > 0)
                if ((inlen = ProcessInput (inbuf, inlen)) < 0)
                    Msg(0, "negative ProcessInput return");
            if (inlen > 0)
                continue;
        }
        /*
         *  handle signals again
         */
        if (GotSignal && !status)
        {   SigHandler ();
            continue;
        }
        /*
         *  output to real terminal
         */
        if (w & 1 << curr->ptyfd && pastelen > 0)
        {   if ((len = write (curr->ptyfd, pastebuffer, pastelen>IOSIZE?IOSIZE:pastelen)) > 0)
            {   pastelen -= len;
                pastebuffer+=len;
            }
        }
        else if (w & 1 << curr->ptyfd && inlen > 0)
        {   if ((len = write (curr->ptyfd, inbuf, inlen)) > 0)
            {   inlen -= len;
                bcopy (inbuf+len, inbuf, inlen);
            }
        }
        /*
         *  handle signals again
         */
        if (GotSignal && !status)
        {   SigHandler ();
            continue;
        }
        /*
         *  handle output from background windows
         */
        for (pp = wtab; pp < wtab+MAXWIN; ++pp)
        {   if (!(p = *pp))
                continue;
            if (p->outlen)
            {   WriteString (p, p->outbuf, p->outlen);
            }
            else if (r & 1 << p->ptyfd)
            {   if ((len = read (p->ptyfd, buf, IOSIZE)) == -1)
                {   if (errno == EWOULDBLOCK)
                        len = 0;
                }
                if (len > 0)
                    WriteString (p, buf, len);
            }
            if (p->bell)
            {   if (p->bell == 1)
                    Msg (0, MakeBellMsg (pp-wtab));
                else
                {
                    Msg (0, "   Wuff,  Wuff!!  ");
                    RemoveStatus(p);
                }
                p->bell = 0;
            }
        }
        if (GotSignal && !status)
            SigHandler ();
    }
    /*NOTREACHED*/
}

static void SigHandler () {
    while (GotSignal) {
    GotSignal = 0;
#ifdef SYSV
    signal (SIGCLD, SigChld);
#endif
    DoWait ();
    }
}

static void SigChld ()
{
    GotSignal = 1;
}

static void SigHup () {
    Detach (0);
}

static void CoreDump(sig) 
int sig;
{
    fprintf(stderr, "\r\n[screen caught signal %d, (core dumped)]\r\n", sig);
    fflush( stderr);
    Kill (AttacherPid, SIGHUP);
    setgid( getgid());
    setuid( getuid());
    unlink("core");
    abort();
}

#if defined(SIGWINCH) && defined(TIOCGWINSZ)
static void SigWinch () {
    struct winsize ws;

    ioctl (0, TIOCGWINSZ, &ws);
    trows = ws.ws_row;
    if( trows == 0)
    trows = TS[0].rows;
    tcols = ws.ws_col;
    if( tcols == 0)
    tcols = TS[0].cols;
/* Msg(0,"SigWinch: %d, %d",tcols,trows); /**/
    if(HasWindow) {
        ChangeWindowSize( curr, trows, tcols);
/*activate (wtab[CurrNum]); */
    }
}
#endif

static DoWait () {
    register pid;
    register struct win **pp;
    union wait wstat;

    while ((pid = wait3 (&wstat, WNOHANG|WUNTRACED, NULL)) > 0)
    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
        if (*pp && pid == (*pp)->wpid) {
#ifdef hpux
            if (WIFSTOPPED (wstat.w_status)) {
/* }are you sure it works this way under hpux? jw */
#else  hpux
            if (WIFSTOPPED (wstat)) {
#endif hpux
                Msg (0, "Child has been stopped, restarting.");
#ifdef hpux
                (void) kill ( -1 * getpgrp2 ((*pp)->wpid), SIGCONT);
#else  hpux
                (void) killpg (getpgrp ((*pp)->wpid), SIGCONT);
#endif hpux
            } else {
                KillWindow (pp);
            }
        }
    }
    CheckWindows ();
}

static KillWindow (pp)
struct win **pp;
{
    if (*pp == curr) {
    curr = 0;
    }
    if (*pp == other)
    other = 0;
    FreeWindow (*pp);
    *pp = 0;
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

static void Finit () {
    register struct win *p, **pp;

    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
    if (p = *pp)
        FreeWindow (p);
    }
    HasWindow=0;
    SetTTY (0, &OldMode);
    FinitTerm ();
    printf ("[screen is terminating]\n");
    Kill (AttacherPid, SIGHUP);
    unlink(SockPath);
    exit (0);
}

static InitKeytab () {
    register i;

    ktab['h'].type = ktab[Ctrl('h')].type = KEY_HARDCOPY;
    ktab['z'].type = ktab[Ctrl('z')].type = KEY_SUSPEND;
    ktab['c'].type = ktab[Ctrl('c')].type = KEY_SHELL;
    ktab['c'].attr = ktab[Ctrl('c')].attr = &defattr;
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
    ktab['m'].type = ktab[Ctrl('m')].type = KEY_LASTMSG;
    ktab['{'].type = KEY_BANGBANG;
    ktab['L'].type = KEY_LOGIN;
    ktab['U'].type = KEY_UNLOGIN;
    ktab['W'].type = KEY_SIZE132;
    ktab['N'].type = KEY_SIZE80;
    ktab['T'].type = KEY_SIZETOGGLE;
    ktab['['].type = ktab[Ctrl('[')].type = KEY_COPY;
    ktab[']'].type = ktab[Ctrl(']')].type = KEY_PASTE;

#ifdef LOCK
    ktab['x'].type = ktab[Ctrl('x')].type = KEY_LOCK;
#endif /* LOCK */
    ktab['.'].type = KEY_TERMCAP;
    ktab[Ctrl('\\')].type = KEY_QUIT;
    ktab[Ctrl('G')].type = KEY_VBELL;
    ktab['d'].type = ktab[Ctrl('d')].type = KEY_DETACH;
    ktab['>'].type = KEY_WRITE_BUFFER;
    ktab['<'].type = KEY_READ_BUFFER;
    ktab['D'].type = KEY_POW_DETACH;
    ktab[Esc].type = KEY_OTHER;
    for (i = 0; i <= 9; i++)
        ktab[i+'0'].type = KEY_0+i;
}

static ProcessInput (buf, len)
char *buf;
{
    register n, k;
    register char *s, *p;
    /* register struct win **pp; */
    

    for (s = p = buf; len > 0; len--, s++) {
    if (*s == Esc && ! curr->locked ) {
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
            Detach (1);
            break;
        case KEY_SHELL:
            p = buf;
            if ((n = MakeWindow (ShellProg, ShellArgs,
                0, 0, (char *)0, ktab[*s].attr)) != -1)
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
            Finit ();
            /*NOTREACHED*/
        case KEY_DETACH:
            p = buf;
            Detach (0);
            break;
        case KEY_POW_DETACH:
            p = buf;
            Msg(0,"D");
            fflush(stdin);
            while(!read(0,buf,1));
            RemoveStatus(curr);
            if(*buf == 'D')
                Detach (3); /* detach and kill parent shell. jw. */
            else 
            {   write(1,"\007",1);
                Msg(0,"The ray of disintegration whizzes by you!");
            }
            break;
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
            Msg (0, "%s  %s  %s",
                ScreenVersion, AnsiVersion, CopyPasteVersion);
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
            Detach (5);
            break;
#endif /* LOCK */
        case KEY_SIZE80:
        case KEY_SIZE132:
        case KEY_SIZETOGGLE:
            p = buf;
            ResizeTerm(ktab[*s].type);
            break;
        case KEY_LOGIN:
            p = buf;
#ifdef UTMP5
            if (curr->slot == (struct utmp *)(-1)) {
#else
            if (curr->slot == -1) {
#endif
#ifdef USRLIMIT
            if (UserCount() >= USRLIMIT) {
                Msg (0, "User limit reached.");
            }
            else    {
                curr->slot = SetUtmp (curr->tty);
                Msg (0, "This window is now logged in.");
            }
#else
            curr->slot = SetUtmp (curr->tty);
            Msg (0, "This window is now logged in.");
#endif
            } 
            else {
            Msg (0, "This window is already logged in.");
            }
            break;
        case KEY_UNLOGIN:
            p = buf;
#ifdef UTMP5
            if (curr->slot != (struct utmp *)(-1))
            {   RemoveUtmp (curr->slot);
                curr->slot = (struct utmp *)(-1);
#else
            if (curr->slot != -1)
            {   RemoveUtmp (curr->slot);
                curr->slot = -1;
#endif
                Msg (0, "This window is no longer logged in.");
            }
            else
                Msg (0, "This window is not logged in.");
            break;
        case KEY_LASTMSG:
            p = buf;
            PrevMsg (curr);
            break;
        case KEY_COPY:
            p = buf;
            SetAnsiCurr(curr); 
            (void) MarkRoutine(PLAIN);
            /* SetAnsiCurr(oldcurr); */
            break;
        case KEY_BANGBANG:
            p = buf;
            SetAnsiCurr(curr); 
            if(MarkRoutine(TRICKY))
                if(copybuffer != NULL)
                {   pastelen=copylen;
                    pastebuffer=copybuffer;
                }
            break;
        case KEY_PASTE:
            p = buf;
            if(copybuffer == NULL)
            {
                Msg (0, "No tooth - no paste.");
                copylen=0;
                break;
            }
            pastelen=copylen;
            pastebuffer=copybuffer;
            break;
        case KEY_WRITE_BUFFER:
            p = buf;
            if(copybuffer == NULL)
            {
                Msg (0, "No tooth - no paste in /tmp.");
                copylen=0;
                break;
            }
            WriteFile(2);
            break;
        case KEY_READ_BUFFER:
            p = buf;
            ReadFile();
            break;
        case KEY_VBELL:
            p = buf;
            if (curr->visualbell)
            {   curr->visualbell = 0;
                Msg(0,":set novbell");
            }
            else
            {   curr->visualbell = 1;
                Msg(0,":set vbell");
            }
            break;
        case KEY_CREATE:
            p = buf;
            if ((n = MakeWindow (ktab[*s].args[0], ktab[*s].args,
                0, 0, (char *)0, ktab[*s].attr)) != -1)
            SwitchWindow (n);
            break;
        }
        } else ESCseen = 1;
    } else *p++ = *s;
    }
    return p - buf;
}

static SwitchWindow (n) {
    /*
     *  if I try to select a non-existant window, 
     *  don't just *sit there*, show me what windows there are
     */
    if (!wtab[n]) {
    ShowWindows();  
    return;
    }
    /*
     *  If I try to select the current window, let me know,
     *  don't let me think I was somewhere else
     */
    if (wtab[n] == curr) {
    Msg (0, "This IS window %d.", n);
    return;
    }
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

    if(wp->lflag)
        RemoveUtmp (wp->slot);
    (void) chmod (wp->tty, 0666);
    (void) chown (wp->tty, 0, 0);
    close (wp->ptyfd);
    for (i = 0; i < wp->rows; ++i) {
    Free (wp->image[i]);
    Free (wp->attr[i]);
    Free (wp->font[i]);
    }
    Free (wp->image);
    Free (wp->attr);
    Free (wp->font);
    Free (wp);
}

static MakeWindow (prog, args, aflag, StartAt, dir, attr)
char *prog, **args, *dir;
struct attr *attr;
{
    register struct win **pp, *p;
    register char **cp;
    register n, f;
    int tf;
    int mypid, rows, cols;
    char ebuf[10];

    if (attr->lflag == -1)
    attr->lflag = defattr.lflag;
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

#ifdef USRLIMIT
    /*
     *  Count current number of users.
     */
    if (attr->lflag == 1) {
    if (UserCount() >= USRLIMIT) {
        Msg (0, "User limit reached.  Window will not be logged in.");
        attr->lflag = 0;
    }
    }
#endif USRLIMIT

    n = pp - wtab;
    if ((f = OpenPTY ()) == -1) {
    Msg (0, "No more PTYs.");
    return -1;
    }
    (void) fcntl (f, F_SETFL, FNDELAY);
    if ((p = *pp = (struct win *)malloc (sizeof (struct win))) == 0) {
nomem:
    Msg (0, "Out of memory.");
    return -1;
    }
    rows = attr->rows ? attr->rows : trows;
    cols = attr->cols ? attr->cols : tcols;

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
    if ((p->font = (char **)malloc (rows * sizeof (char *))) == 0)
    goto nomem;
    for (cp = p->font; cp < p->font+rows; ++cp) {
    if ((*cp = malloc (cols)) == 0)
        goto nomem;
    bzero (*cp, cols);
    }
    if ((p->tabs = malloc (cols+1)) == 0)  /* +1 because 0 <= x <= cols */
    goto nomem;
    p->aflag = aflag;
    p->active = 0;
    p->bell = 0;
    p->outlen = 0;
    p->ptyfd = f;
    p->rows = rows;
    p->cols = cols;
    strncpy (p->term , attr->term, sizeof p->term);
    p->lflag = attr->lflag;
    p->locked = attr->locked;
    p->visualbell = attr->visualbell;
    if (attr->sname[0] != '\0')
        strncpy (p->cmd, attr->sname,MAXSTR-1);
    else
        strncpy (p->cmd, Filename (args[0]), MAXSTR-1);
    p->cmd[MAXSTR-1] = '\0';
    strncpy (p->tty, TtyName, MAXSTR-1);
    ResetScreen (p);
    (void) chown (TtyName, getuid (), getgid ());
    if (attr->lflag == 1) {
    p->slot = SetUtmp (TtyName);
    (void) chmod (TtyName, TtyMode);
    }
    else {
#ifdef UTMP5
    p->slot = (struct utmp *)(-1);
#else
    p->slot = -1;
#endif
    (void) chmod (TtyName, 0600);
    }
    switch (p->wpid = fork ()) {
    case -1:
    Msg (errno, "fork");
    Free (p);
    return -1;
    case 0:
    signal (SIGHUP, SIG_DFL);
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTERM, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
#ifdef TIOCGWINSZ
    signal (SIGWINCH, SIG_DFL);
#endif
    setuid (getuid ());
    setgid (getgid ());
    if (dir && chdir (dir) == -1) {
        SendErrorMsg ("Cannot chdir to %s: %s", dir, sys_errlist[errno]);
        exit (1);
    }
    mypid = getpid ();
    brktty();
    if ((tf = open (TtyName, O_RDWR)) == -1) {
        SendErrorMsg ("Cannot open %s: %s", TtyName, sys_errlist[errno]);
        exit (1);
    }
    (void) dup2 (tf, 0);
    (void) dup2 (tf, 1);
    (void) dup2 (tf, 2);
#ifdef SYSV
    for (f = NOFILE - 1; f > 2; f--)
#else SYSV
    for (f = getdtablesize () - 1; f > 2; f--)
#endif SYSV
        close (f);
    ioctl (0, TIOCSPGRP, &mypid);
    dofg();
    SetTTY (0, &OldMode);
    NewEnv[2] = MakeTermcap (p);
    sprintf (ebuf, "WINDOW=%d", n);
    NewEnv[3] = ebuf;
    execvpe (prog, args, NewEnv);
    SendErrorMsg ("Cannot exec %s: %s", prog, sys_errlist[errno]);
    exit (1);
    }
    return n;
}

static execvpe (prog, args, env) char *prog, **args, **env; {
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

static ReadFile()
{   int i,l;
    struct stat s;
    if (stat(BUFFERFILE,&s))
    {   Msg(errno,"no %s -- no slurp!",BUFFERFILE);
        return;
    }
    Free(copybuffer);
    copylen = 0;
    if((copybuffer=malloc(s.st_size))==NULL)
    {   Msg(0, "Not enough memoooh!... Sorry.");
        return 0;
    }
    if((i=open(BUFFERFILE,O_RDONLY)) < 0)
    {   Msg(errno,"no good %s -- no slurp at all!",BUFFERFILE);
        return;
    }
    errno=0;
    if((l=read(i,copybuffer,s.st_size)) != s.st_size)
    {   copylen = (l>0)?l:0;
        Msg(errno,"You choke on your food: %d bytes",copylen);
        close(i);
        return;
    }
    copylen = l;
    Msg(0, "Slurped %d characters into buffer", copylen);
    close(i);
    return;
}
    
static WriteFile (dump)
{   /* dump==0: create .termcap,
     * dump==1: hardcopy
     * dump==2: /tmp/screenbuffer
     */
    register i, j, k;
    register char *p;
    register FILE *f;
    char fn[1024];
    int pid, s;

    switch (dump)
    {   case 0:
#ifdef SOCKDIR
            sprintf (fn, "%s/%s/.termcap", SockDir, LoginName);
#else
            sprintf (fn, "%s/%s/.termcap", home, SockDir);
#endif
            break;
        case 1:
            sprintf (fn, "hardcopy.%d", CurrNum);
            break;
        case 2:
            sprintf (fn, BUFFERFILE);
            break;
        default:
            Msg(0,"WriteFile(%d) unknown option.",dump);
            return;
    }
    switch (pid = fork ()) {
    case -1:
    Msg (errno, "fork");
    return;
    case 0:
    setuid (getuid ());
    setgid (getgid ());
    switch (dump)
    {   case 0:
            if ((f = fopen (fn, "w")) == NULL)
                exit (1);
            if (p = index (MakeTermcap (curr), '='))
            {   fputs (++p, f);
                putc ('\n', f);
            }
            break;
        case 1:
            if ((f = fopen (fn, "w")) == NULL)
                exit (1);
            for (i = 0; i < curr->rows; ++i)
            {   p = curr->image[i];
                for (k = curr->cols-1; k >= 0 && p[k] == ' '; --k) ;
                for (j = 0; j <= k; ++j)
                    putc (p[j], f);
                putc ('\n', f);
            }
            break;
        case 2:
            umask(0);
            if ((f = fopen (fn, "w")) == NULL)
                exit (1);
            p = copybuffer;
            for (i=0;i<copylen;i++)
                putc (*p++, f);
            break;
    }
    (void) fclose (f);
    exit(0);
    default:
		errno = 0;
		while ( ((i=wait(&s)) != pid) ||
			((i < 0) && (errno == EINTR))) errno=0;
	   	if (errno)
		{	Msg(errno,"Child not dead");
			return;
		}
    if ((s >> 8) & 0377)
        Msg (0, "Cannot open \"%s\".", fn);
    else
    {
        switch (dump)
        {   case 0:
                Msg (0, "Termcap entry written to \"%s\".", fn);
                break;
            case 1:
                Msg (0, "Screen image written to \"%s\".", fn);
                break;
            case 2:
                Msg (0, "Copybuffer written to \"%s\".", fn);
        }
    }
    }
}

static ShowWindows () {
    char buf[1024];
    register char *s;
    register struct win **pp, *p;

    for (s = buf, pp = wtab; pp < wtab+MAXWIN; ++pp) {
    if ((p = *pp) == 0)
        continue;
    if (s - buf + 5 + strlen (p->cmd) > curr->cols-1)
        break;
    if (s > buf) {
        *s++ = ' '; *s++ = ' ';
    }
    *s++ = pp - wtab + '0';
    if (p == curr)
        *s++ = '*';
    else if (p == other)
        *s++ = '-';
#ifdef UTMP5
    if (p->slot != (struct utmp *)(-1))
#else
    if (p->slot != -1)
#endif
        *s++ = '$';
    *s++ = ' ';
    strcpy (s, p->cmd);
    s += strlen (s);
    }
    Msg (0, buf);
}

static ShowInfo () {
    char buf[1024], *p;
    register struct win *wp = curr;
    register i;
    struct tm *tp;
    time_t now;

    time (&now);
    tp = localtime (&now);
    sprintf (buf, "%2d:%02.2d:%02.2d %s", tp->tm_hour, tp->tm_min, tp->tm_sec,
    HostName);
#ifdef LOADAV 
    if (avenrun && GetAvenrun ()) {
    p = buf + strlen (buf);
#ifdef SUNLOADAV
    sprintf (p, " %2.2f %2.2f %2.2f", (double)loadav[0]/FSCALE,
        (double)loadav[1]/FSCALE, (double)loadav[2]/FSCALE);
#else
#ifdef alliant
    sprintf (p, " %2.2f %2.2f %2.2f %2.2f", (double)loadav[0]/100,
        (double)loadav[1]/100, (double)loadav[2]/100,
        (double)loadav[3]/100);
#else
    sprintf (p, " %2.2f %2.2f %2.2f", loadav[0], loadav[1], loadav[2]);
#endif
#endif
    }
#endif
#ifdef LOADAVSYSCALL
    p = buf + strlen (buf);
    loadav = (int *) getloadav();
    sprintf (p, " %2.2f %2.2f %2.2f", (double)loadav[0]/100,
        (double)loadav[1]/100, (double)loadav[2]/100);
#endif LOADAVSYSCALL
    p = buf + strlen (buf);
    sprintf (p, " (%d,%d) %cflow %cins %corg %cwrap %cpad", wp->y, wp->x,
    flowctl ? '+' : '-',
    wp->insert ? '+' : '-', wp->origin ? '+' : '-',
    wp->wrap ? '+' : '-', wp->keypad ? '+' : '-');
    if (ISO2022) {
    p = buf + strlen (buf);
    sprintf (p, " G%1d [", wp->LocalCharset);
    for (i = 0; i < 4; i++)
        p[i+5] = wp->charsets[i] ? wp->charsets[i] : 'B';
    p[9] = ']';
    p[10] = '\0';
    }
    Msg (0, buf);
}

#ifdef sequent
static OpenPTY () {
    char *m, *s;
    register f;
    f = getpseudotty (&s, &m);
    strncpy (PtyName, m, sizeof (PtyName));
    strncpy (TtyName, s, sizeof (TtyName));
    ioctl (f, TIOCFLUSH, (char *)0);
    return (f);
}

#else

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
#endif

static SetTTY (fd, mp) struct mode *mp; {
#ifdef TIOCSWINSZ
    ioctl (fd, TIOCSWINSZ, &mp->ws);
#endif
#ifdef TCGETA
    ioctl (fd, TCSETA, &mp->tio);
#else
    ioctl (fd, TIOCSETP, &mp->m_ttyb);
    ioctl (fd, TIOCSETC, &mp->m_tchars);
    ioctl (fd, TIOCSLTC, &mp->m_ltchars);
    ioctl (fd, TIOCLSET, &mp->m_lmode);
    ioctl (fd, TIOCSETD, &mp->m_ldisc);
#endif
}

static GetTTY (fd, mp) struct mode *mp; {
#ifdef TIOCSWINSZ
    ioctl (fd, TIOCGWINSZ, &mp->ws);
    if( mp->ws.ws_row == 0)
    mp->ws.ws_row = trows;
    if( mp->ws.ws_col == 0)
    mp->ws.ws_col = tcols;
#endif
#ifdef TCGETA
    ioctl (fd, TCGETA, &mp->tio);
#else
    ioctl (fd, TIOCGETP, &mp->m_ttyb);
    ioctl (fd, TIOCGETC, &mp->m_tchars);
    ioctl (fd, TIOCGLTC, &mp->m_ltchars);
    ioctl (fd, TIOCLGET, &mp->m_lmode);
    ioctl (fd, TIOCGETD, &mp->m_ldisc);
#endif
}

static SetMode (op, np) struct mode *op, *np; {
    *np = *op;
#ifdef TCGETA
    np->tio.c_iflag &= ~ ICRNL;
    np->tio.c_oflag &= ~ ONLCR;
    np->tio.c_lflag &= ~ ( ISIG | ICANON | ECHO );

    np->tio.c_cc[VMIN] = 1;
    np->tio.c_cc[VTIME] = 1;
    if (!flowctl) {
    np->tio.c_iflag &= ~IXON;
    }
#else
    np->m_ttyb.sg_flags &= ~(CRMOD|ECHO);
    np->m_ttyb.sg_flags |= CBREAK;
    np->m_tchars.t_intrc = -1;
    np->m_tchars.t_quitc = -1;
    if (!flowctl) {
    np->m_tchars.t_startc = -1;
    np->m_tchars.t_stopc = -1;
    }
    np->m_ltchars.t_suspc = -1;
    np->m_ltchars.t_dsuspc = -1;
    np->m_ltchars.t_flushc = -1;
    np->m_ltchars.t_lnextc = -1;
#endif
}

static char *GetTtyName () {
    register char *p;
    register n;

    for (p = 0, n = 0; n <= 2 && !(p = ttyname (n)); n++)
    ;
    if (!p || *p == '\0')
    Msg (0, "screen must run on a tty.");
    return p;
}

static int Attach (how) {
    register s, lasts, found = 0;
    register DIR *dirp;
    register struct direct *dp;
    register char *cp;
    struct msg m;
    char last[MAXNAMLEN+1];
    char t[1024];

    if (SockName)
    {   if ((lasts = MakeClientSocket (0)) == -1)
            if (how == MSG_CONT)
Msg (0, "This screen has already been continued from elsewhere.");
            else
Msg (0, "There is no screen to be resumed from %s.", SockName);
    }
    else
    {   if ((dirp = opendir (SockPath)) == NULL)
            Msg (0, "Cannot open %s", SockPath);
        while ((dp = readdir (dirp)) != NULL)
        {   SockName = dp->d_name;
            if (SockName[0] == '.')
                continue;
            if ((s = MakeClientSocket (0)) != -1)
            {   if (found == 0)
                {   strcpy (last, SockName);
                    lasts = s;
                }
                else
                {   if (found == 1)
                    {   printf ("There are detached screens on:\n");
                        printf ("   %s\n", last);
                        close (lasts);
                    }
                    printf ("   %s\n", SockName);
                    close (s);
                }
                found++;
            }
        }
        if (found == 0)
        {
#ifdef RETRY_WHEN_R_FAILED
            printf("There is no screen to be resumed...\n");
            printf("... but wait, there is a mimic hiding under the screen\n");
            sleep (4);
            return (1); 
#else
/* Msg() exits, when there is no screen running . jw.
 */
            Msg (0, "There is no screen to be resumed.");
#endif
        }
        if (found > 1)
            Msg(0,"Type \"screen -r host.tty\" to resume one of them.");
        closedir (dirp);
        strcpy (SockNamePtr, last);
        SockName = SockNamePtr;
    }
    if (how == MSG_ATTACH)
    {
        if (dflag || Dflag)
        {   strcpy (m.m.detach.tty, GetTtyName ());
            m.m.detach.dpid = getpid ();
            if (Dflag)
                m.type = MSG_POW_DETACH;
            else
                m.type = MSG_DETACH;
			trysend(lasts,&m,m.m.detach.passwd);
            close(lasts);
            if ((lasts = MakeClientSocket (0)) == -1)
                Msg (0, "Cannot contact screen again.");
        }
    }
    m.type = how;
    strcpy (m.m.attach.tty, GetTtyName ());
    if( ( cp = getenv("TERM")) == (char *) NULL)
        Msg (0, "No TERM in environment");
    strncpy( m.m.attach.term, cp, sizeof( m.m.attach.term) - 1);
    m.m.attach.term[sizeof( m.m.attach.term) -1] = '\0';
    if ( tgetent(t, m.m.attach.term) != 1)
        Msg (0, "Cannot find termcap entry for %s.", m.m.attach.term);
    m.m.attach.apid = getpid ();
	trysend(lasts,&m,m.m.attach.passwd);
    close(lasts);
    return(0);
}

static trysendstat;

static trysendok()
{
	trysendstat = 1; 
}

static trysendfail()
{
	trysendstat = -1;
}

static trysend(fd,m,pwto)
struct msg *m;
char *pwto;
{
	static char *pw="";
	for(;;)
	{	strcpy(pwto,pw);
		trysendstat = 0;
		signal(SIGCONT,trysendok);
		signal(SIGHUP,trysendfail);
		if (write (fd, (char *)m, sizeof (*m)) != sizeof (*m))
			Msg (errno, "write");
		while(trysendstat==0) pause();
		if (trysendstat>0)
		{	signal(SIGCONT,SIG_DFL);
			signal(SIGHUP,SIG_DFL);
			return;
		}
		if (*pw||*(pw=getpass("Password:"))==0)
			Msg (0, "Password incorrect");
	}
}

	
static void AttacherFinit () {
    exit (0);
}

static void AttacherFinitBye () {
    int ppid;
#ifdef hpux
    printf("\ncome getty, come!\n");
#endif
    close (0);
    close (1);
    close (2);
    brktty();
    setuid(getuid()); setgid(getgid());
    if((ppid=getppid()) != 1) /* we dont want to disturb init,eh? jw. */
        Kill(ppid,SIGHUP);
    exit (0);
}

static void ReAttach () {
    (void) Attach (MSG_CONT);
}

static LockPlease;

static DoLock()
{
	LockPlease=1;
}

static Attacher () 
{	signal (SIGUSR1, AttacherFinitBye);
	signal (SIGUSR2, DoLock);
	signal (SIGHUP, AttacherFinit);
	signal (SIGCONT, ReAttach);
	signal (SIGINT, SIG_IGN);
	while(1) 
	{	pause();
		if (LockPlease) 
		{	LockPlease=0;
			LockTerminal();
#ifdef SYSV
			signal (SIGUSR2, DoLock);
#endif
			ReAttach();
		}
	}
}

static Detach (mode) {
    register struct win **pp;
    int power_delayed_detach=0;

    if (Detached)
    return;
    signal (SIGHUP, SIG_IGN);
    SetTTY (0, &OldMode);
    FinitTerm ();
    switch (mode)
	{
		case 0: 
		case 3:
			for (pp = wtab; pp < wtab+MAXWIN; ++pp) 
			{
#ifdef UTMP5
				if (*pp && ((*pp)->slot != (struct utmp *)(-1))) 
				{ 
#else
				if (*pp && ((*pp)->slot != -1)) 
				{ /* } */
#endif
					RemoveUtmp ((*pp)->slot);
				}
			}
			if (mode==3)
			{   printf ("\n[power detached]\n");
				power_delayed_detach=1;
			}
			else
			{   printf ("\n[detached]\n");
				Kill (AttacherPid, SIGHUP);
					AttacherPid = 0;
			}
			break;
		case 1: Kill (AttacherPid, SIGTSTP);
			break;
		case 5: Kill (AttacherPid, SIGUSR2); 
			/* tell attacher to lock terminal with a lockprg. mls. */
			break;
		case 2:
		case 4:
			if (mode==4)
			{   power_delayed_detach=1;
				printf ("\n[remote power detached]\n");
			}
			else
			{   Kill (AttacherPid, SIGHUP); 
				printf ("\n[remote detached]\n");
				AttacherPid = 0;
			}
			break;
	}
    close (0);
    close (1);
    close (2);
    brktty();
    if (power_delayed_detach)
    {   
        /* tell father to kill grandpa, then himself. jw. 
         * We do that after we freed the tty, thus getty feels 
         * more confortable on hpux.
         */
        Kill (AttacherPid, SIGUSR1);
        AttacherPid = 0;
    }
    Detached = 1;

    do {
    ReceiveMsg (ServerSocket); 
    } while (Detached);
#ifndef SYSV
    if ((DevTty = open ("/dev/tty", O_RDWR|O_NDELAY)) == -1)
    Msg (errno, "/dev/tty");
#endif SYSV
    if (mode == 1 || mode == 5)
    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
#ifdef UTMP5
        if (*pp && ((*pp)->slot != (struct utmp *)(-1))) { /* } */
#else
        if (*pp && ((*pp)->slot != -1)) {
#endif
            (*pp)->slot = SetUtmp ((*pp)->tty);
        }
    }
    signal (SIGHUP, SigHup);
}

static Kill (pid, sig) {
    if (pid != 0)
    (void) kill (pid, sig);
}

static GetSockName () {
    register client;
    static char buf[2*MAXSTR];

    if (!mflag && (SockName = getenv ("STY")) != 0 && *SockName != '\0') {
    client = 1;
    setuid (getuid ());
    setgid (getgid ());
    } else {
    sprintf (buf, "%s.%s", HostName, Filename (GetTtyName ()));
    SockName = buf;
    client = 0;
    }
    return client;
}

#ifdef NAMEDPIPE
static MakeServerSocket () {
    register s;
    char *p;

    strcpy (SockNamePtr, SockName);

    if ( (s = open(SockPath, O_WRONLY | O_NDELAY)) >= 0) {
    p = Filename (SockPath);
    Msg (0, "You have already a screen running on %s.\n\
If it has been detached, try \"screen -r\".", p);
        close(s);
    }

    (void) unlink (SockPath);
    if (mknod (SockPath, S_IFIFO | S_IWRITE | S_IREAD, 0)) 
    Msg (errno, "mknod fifo");
    if (chown (SockPath, getuid (), getgid ()))
    Msg (errno, "chown fifo");
    if ( (s = open(SockPath, O_RDWR | O_NDELAY)) < 0)
    Msg (errno, "open fifo");

    return s;
}

static MakeClientSocket (err) {
    register s;

    strcpy (SockNamePtr, SockName);
    if ( (s = open(SockPath, O_WRONLY | O_NDELAY)) < 0)
    {   if (err) 
        {   Msg (errno, "open: %s (but continuing...)", SockPath);
            return -1; /* oh boy, oh boy, jw. */
        }
        else
        {   close (s);
            return -1;
        }
    }
    else
    {   (void) fcntl(s, F_SETFL, 0);
        return s;
    }
}


#else NAMEDPIPE
static MakeServerSocket () {
    register s;
    struct sockaddr_un a;
    char *p;

    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    Msg (errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy (SockNamePtr, SockName);
    strcpy (a.sun_path, SockPath);
    if (connect (s, (struct sockaddr *)&a, strlen (SockPath)+2) != -1) {
    p = Filename (SockPath);
    Msg (0, "You have already a screen running on %s.\n\
If it has been detached, try \"screen -r\".", p);
    /*NOTREACHED*/
    }
    (void) unlink (SockPath);
    if (bind (s, (struct sockaddr *)&a, strlen (SockPath)+2) == -1)
    Msg (errno, "bind");
    (void) chown (SockPath, getuid (), getgid ());
    if (listen (s, 5) == -1)
    Msg (errno, "listen");
    return s;
}

static MakeClientSocket (err) {
    register s;
    struct sockaddr_un a;

    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    Msg (errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy (SockNamePtr, SockName);
    strcpy (a.sun_path, SockPath);
    if (connect (s, (struct sockaddr *)&a, strlen (SockPath)+2) == -1) {
    if (err) {
        Msg (errno, "connect: %s", SockPath);
    } else {
        close (s);
        return -1;
    }
    }
    return s;
}

#endif NAMEDPIPE

static SendCreateMsg (s, ac, av, aflag, attr)
char **av;
struct attr *attr;
{
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
    m.m.create.attr = *attr; 

#ifdef SYSV
    if (getcwd (m.m.create.dir, sizeof( m.m.create.dir) ) == 0)
#else  SYSV
    if (getwd (m.m.create.dir) == 0)
#endif SYSV
    Msg (0, "%s", m.m.create.dir);
    if (write (s, (char *)&m, sizeof (m)) != sizeof (m))
    Msg (errno, "write");
}

/*VARARGS1*/
static SendErrorMsg (fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
    register s; struct msg m;

    s = MakeClientSocket (1);
    m.type = MSG_ERROR;
    sprintf (m.m.message, fmt, p1, p2, p3, p4, p5, p6);
    (void) write (s, (char *)&m, sizeof (m));
    close (s);
    sleep (2);
}

static ReceiveMsg (s) {
    int left, len;
    struct msg m;
    char *p;

#ifdef NAMEDPIPE
    /*
     * We may be called if there are no pending messages,
     * so we will have to block on first read.
     */
    if( fcntl( s, F_SETFL, 0) == -1) {
    Msg (errno, "fcntl no O_NDELAY");
    exit(1);
    }
    p = (char *)&m;
    left = sizeof (m);

    while (left > 0 && (len = read (s, p, left)) > 0) {

/*  if( p == (char *)&m) {
            if( fcntl( s, F_SETFL, O_NDELAY) == -1) {
            Msg (errno, "fcntl O_NDELAY");
            return;
            }
    }
*/

    p += len;
    left -= len;
    }
#else  NAMEDPIPE
    register ns;
    struct sockaddr_un a;

    len = sizeof (a);
    if ((ns = accept (s, (struct sockaddr *)&a, &len)) == -1) {
    Msg (errno, "accept");
    return;
    }
    p = (char *)&m;
    left = sizeof (m);
    while (left > 0 && (len = read (ns, p, left)) > 0) {
    p += len;
    left -= len;
    }
    close (ns);
#endif NAMEDPIPE

    if (len == -1)
    Msg (errno, "read");
    if (left > 0)
    return;
    switch (m.type) {
    case MSG_CREATE:
    if (!Detached)
        ExecCreate (&m);
    break;
    case MSG_CONT:
    if (m.m.attach.apid != AttacherPid || !Detached)
        break;  /* Intruder Alert */
    /*FALLTHROUGH*/
    case MSG_ATTACH:
    if( ! CheckPasswd( m.m.attach.passwd, m.m.attach.apid, m.m.attach.tty))
        return;
    if (Detached) {
        if (kill (m.m.attach.apid, 0) == 0 &&
            open (m.m.attach.tty, O_RDWR|O_NDELAY) == 0) {
#ifdef hpux_and_it_still_doesnt_work
        /*
         * Though hpux is SYSV, we can emulate BSD behaviour
         * for process groups. (HACK, HACK, HACK)
         */

        {   int pgrp,zero = 0;
             
            ioctl( 0, TIOCGPGRP, &pgrp); /* save old pgrp from tty */

            ioctl( 0, TIOCSPGRP, &zero); /* detach tty from proc grp */
            setpgrp(); /* make me process group leader */

            close( 0); /* reopen tty, now we should get it as our tty */
            (void)open (m.m.attach.tty, O_RDWR|O_NDELAY); /* */

            setpgrp2( 0, pgrp);
            ioctl( 0, TIOCSPGRP, &pgrp);
        }
#endif hpux_and_it_still_doesnt_work
        (void) dup (0);
        (void) dup (0);
        AttacherPid = m.m.attach.apid;
        Detached = 0;
        GetTTY (0, &OldMode);
#ifdef SIOCGWINSZ
        /*
         * reenable this signal so that we can get SIGWINCH again
         */
        signal( SIGWINCH, SigWinch);
#endif
        ChangeTerm (m.m.attach.term);
        SetMode (&OldMode, &NewMode);
        SetTTY (0, &NewMode);
        SetCurrWindow(CurrNum);
        Activate (wtab[CurrNum]);
        }
    } else {
        Kill (m.m.attach.apid, SIGHUP);
        Msg (0, "Not detached.");
    }
    break;
    case MSG_ERROR:
    Msg (0, "%s", m.m.message);
    break;
    case MSG_DETACH:
    case MSG_POW_DETACH:
    if( ! CheckPasswd( m.m.detach.passwd, m.m.detach.dpid, m.m.detach.tty))
        return;
    if (m.type == MSG_POW_DETACH)
        Detach(4);
    else
        Detach(2);
    break;
    default:
    Msg (0, "Invalid message (type %d).", m.type);
    }
}

static CheckPasswd( pwd, pid, tty)
char *pwd, *tty;
int pid;
{
    char buf[256];


	if (CheckPassword)
		if(strcmp( crypt( pwd, Password), Password))
		{
			if (*pwd)
			{
				sprintf( buf, "Illegal reattach attempt from terminal %s, \"%s\"", tty, pwd);
				Msg(0, buf);
			}
			Kill (pid,SIGHUP);
			return 0;
		}
	Kill(pid,SIGCONT);
    return 1;
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
        mp->m.create.dir, &mp->m.create.attr)) != -1)
    SwitchWindow (n);
}

static ReadRc (fn)
char *fn;
{
    FILE *f;
    register char *p, **pp, **ap;
    register argc, num, c;
    char buf[256];
    char *args[MAXARGS];
    int key;
    struct attr currattr;

    currattr = defattr;
    currattr.rows = trows;
    currattr.cols = tcols;
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
            ktab[Esc].type = KEY_OTHER;
    } else if (strcmp (ap[0], "lock") == 0 ) {
        currattr.locked = 1;
    } else if (strcmp (ap[0], "unlock") == 0) {
        currattr.locked = 0;
    } else if (strcmp (ap[0], "login") == 0) {
        currattr.lflag = 1;
    } else if (strcmp (ap[0], "unlogin") == 0) {
        currattr.lflag = 0;
    } else if (strcmp (ap[0], "nologin") == 0) {
        currattr.lflag = 0;
    } else if (strcmp (ap[0], "chdir") == 0) {
        p = argc < 2 ? home : ap[1];
        if (chdir (p) == -1)
        Msg (errno, "%s", p);
    } else if (strcmp (ap[0], "novbell") == 0) {
        currattr.visualbell = 0;
        defattr.visualbell = 0;
    } else if (strcmp (ap[0], "novisualbell") == 0) {
        currattr.visualbell = 0;
        defattr.visualbell = 0;
    } else if (strcmp (ap[0], "vbell") == 0) {
        currattr.visualbell = 1;
        defattr.visualbell = 1;
    } else if (strcmp (ap[0], "visualbell") == 0) {
        currattr.visualbell = 1;
        defattr.visualbell = 1;
    } else if (strcmp (ap[0], "mode") == 0) {
        if (argc != 2) {
        Msg (0, "%s: mode: one argument required.", fn);
        } else if (!IsNum (ap[1], 7)) {
        Msg (0, "%s: mode: octal number expected.", fn);
        } else (void) sscanf (ap[1], "%o", &TtyMode);
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
        if (argc > 1 && IsNumColon (ap[1], 10, currattr.sname)) {
        num = atoi (ap[1]);
        if (num < 0 || num > MAXWIN-1)
            Msg (0, "%s: illegal screen number %d.", fn, num);
        --argc; ++ap;
        }
        if (argc < 2) {
        ap[1] = ShellProg; argc = 2;
        }
        ap[argc] = 0;
        (void) MakeWindow (ap[1], ap+1, 0, num, (char *)0, &currattr);
    } else if (strcmp (ap[0], "password") == 0) {
        CheckPassword = 1;
        if( argc >= 2 )
        strncpy (Password, ap[1], sizeof Password);
        else 
		{   strncpy (Password, getpass("New screen password:"),
			sizeof Password);
			if( strcmp( Password, getpass("Retype new password:"))) 
			{   Msg(0,"Passwords don't match - checking turned off");
				CheckPassword = 0;
			}
			strncpy (Password, crypt( Password, Password),
			sizeof Password);
		}
    } else if (strcmp (ap[0], "term") == 0) {
        if (argc != 2 && argc != 4) {
        Msg (0, "%s: term: one or three arguments required.", fn);
        }
        strncpy (currattr.term, ap[1], sizeof currattr.term);
        if (argc == 4) {
        currattr.rows = atoi( ap[2]);
        currattr.cols = atoi( ap[3]);
        if( currattr.rows == 0)
            currattr.rows =defattr.rows;
        if( currattr.cols == 0)
            currattr.cols =defattr.cols;
        }
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
            ktab[key].attr = (struct attr *) NULL;
        } else {
            ktab[key].type = KEY_CREATE;
            ktab[key].args = SaveArgs (argc-2, ap+2);
                ktab[key].attr = (struct attr *) malloc(sizeof (struct attr));
                if (ktab[key].attr  == (struct attr *) NULL) {
                Msg (1, "malloc");
/*
 *              return 1;
 * warning: function ReadRc has return(e); and return;
 * why? jw
 */
                return;
                }
                *ktab[key].attr = currattr;
        }
        }
    } else Msg (0, "%s: unknown keyword \"%s\".", fn, ap[0]);
    }
    (void) fclose (f);
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
Msg (err, fmt, p1, p2, p3, p4, p5, p6)
char *fmt;
{
    char buf[1024];
    register char *p = buf;

    if (Detached)
    return;
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
    AddMsg (buf);
    } else {
    printf ("%s\r\n", buf);
    Kill (AttacherPid, SIGHUP);
    exit (1);
    }
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

static IsNumColon (s, base, p)
int base;
char *s, *p;
{   char *q;
    if ((q=rindex(s,':'))) 
    {   strncpy(p,q+1,20);
        *q='\0';
    }
    else *p='\0';
    return IsNum(s,base);
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

#ifndef UTMP5
    if ((utmpf = open (UtmpName, O_WRONLY)) == -1) {
    if (errno != EACCES)
        Msg (errno, UtmpName);
    return;
    }
#endif /* UTMP5 */

    utmp = 1;
}

#ifdef UTMP5

#ifdef USRLIMIT
int UserCount() {
    register count = 0;
    struct utmp *u;

    setutent();
    while( (u = getutent()) != (struct utmp *) NULL) {
        if (u->ut_type == USER_PROCESS)
        count++;
    }
    return count;
}
#endif

static struct utmp *
SetUtmp (name) char *name; {
    register char *p;
    struct utmp *u;

    if (p = rindex (name, '/'))
    ++p;
    else p = name;
    if ( (u = (struct utmp *) malloc(sizeof(struct utmp)) )
        == (struct utmp *) NULL )
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
}

static RemoveUtmp (slot)
  struct utmp *slot;
{
    if (slot)
      {
        slot->ut_type = DEAD_PROCESS;
        pututline(slot);
      }
}

#else /* !UTMP5 */

#ifdef USRLIMIT
int UserCount() {
    register count = 0;
    struct utmp utmpbuf;

    (void) lseek (utmpf, 0, 0);
    count = 0;
    while (read(utmpf, &utmpbuf, sizeof(struct utmp)) > 0) {
        if (utmpbuf.ut_name[0] != '\0')
            count++;
    }
    return count;
}
#endif

static SetUtmp (name) char *name; {
    register char *p;
    register struct ttyent *tp;
    register slot = 1;
    struct utmp u;

    if (!utmp)
    return 0;
    if (p = rindex (name, '/'))
    ++p;
    else p = name;
    setttyent ();
    while ((tp = getttyent()) != NULL && strcmp (p, tp->ty_name) != 0)
    ++slot;
    if (tp == NULL)
    return 0;
    strncpy (u.ut_line, p, 8);
    strncpy (u.ut_name, LoginName, 8);
    u.ut_host[0] = '\0';
    time (&u.ut_time);
    (void) lseek (utmpf, (long)(slot * sizeof (u)), 0);
    (void) write (utmpf, (char *)&u, sizeof (u));
    return slot;
}

static RemoveUtmp (slot) {
    struct utmp u;

    if (slot) {
    bzero ((char *)&u, sizeof (u));
    (void) lseek (utmpf, (long)(slot * sizeof (u)), 0);
    (void) write (utmpf, (char *)&u, sizeof (u));
    }
}

#endif /* !UTMP5 */

#ifdef LOCK

/* ADDED by Rainer Pruy 10/15/87 */
/* REFRESHED by mls. */

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
  printf("\n");

  if ( (prg = getenv("LOCKPRG")) == NULL)
        prg = DefaultLock;
  if ( (pid = fork()) == 0)
    {
      /* Child */
        setuid(getuid()); setgid(getgid());
        execl(prg,"SCREEN-LOCK",NULL);
        exit(errno);
    }
  if (pid == -1)
    {
      Msg(errno,"Cannot lock terminal - fork failed");
    }
  else
    {
      union wait status;
      int wret, saverrno;
#ifdef hpux
      void (*sigi)();

      sigi = signal(SIGCLD, SIG_DFL);
#endif
      errno = 0;
      while ( ((wret=wait(&status)) != pid) ||
          ((wret == -1) && (errno == EINTR))
        ) errno = 0;
#ifdef hpux
      signal(SIGCLD, sigi);
#endif

      if (errno) {perror("Lock");sleep(2);}
      else if (status.w_T.w_Termsig != 0)
        {
        fprintf(stderr,"Lock: %s: Killed by signal: %d%s\n",prg,
          status.w_T.w_Termsig,status.w_T.w_Coredump?" (Core dumped)":"");
        sleep(2);
        }
      else if (status.w_T.w_Retcode)
        {
        fprintf(stderr,"Lock: %s: %d\n", prg, status.w_T.w_Retcode);
        sleep(2);
        }
      else printf(LockEnd);
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
    avenrun = 0;
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
    avenrun = 1;
}



static GetAvenrun () {
#ifdef KVMLIB
    if (kvm_read( kmemf, nl[0].n_value, loadav, sizeof (loadav)) != sizeof(loadav))
    return 0;
#else KVMLIB
    if (lseek (kmemf, nl[0].n_value, 0) == -1)
    return 0;
    if (read (kmemf, loadav, sizeof (loadav)) != sizeof (loadav))
    return 0;
#endif KVMLIB
    return 1;
}

#endif


ResizeTerm( key)
int key;
{
    extern int lastsize;
    int i;
    
    switch( key) {
        case KEY_SIZE132:
        i = 1;
        break;
        case KEY_SIZE80:
            i = 0;
        break;
        case KEY_SIZETOGGLE:
        i = (lastsize < 0) ? 0 : (1 - lastsize);
        break;
    }
    if( ChangeTermSize( TS[i].rows, TS[i].cols)) {
        ChangeWindowSize( curr, TS[i].rows, TS[i].cols);
        Activate(wtab[CurrNum]);
        lastsize = i;
    } else 
    Msg( 0, "This terminal does not support two different display sizes.");
}


dofg()
{	int mypid;
	mypid=getpid();
#ifdef SYSV
	ioctl(0,TIOCSPGRP,&mypid);
	setpgrp2(mypid,mypid);
#else  SYSV
	ioctl( 0, TIOCSPGRP, &mypid);
	setpgrp(0,mypid);
#endif SYSV
}

brktty()
{
#ifdef SYSV
setpgrp();  /* will break terminal affiliation */
#else  SYSV
if (DevTty) {
    ioctl (DevTty, TIOCNOTTY, (char *)0);
    close(DevTty);
    DevTty=0;
}
#endif SYSV
}

