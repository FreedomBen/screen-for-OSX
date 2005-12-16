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
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")


#include <sys/types.h>
#include <ctype.h>

#include <fcntl.h>
#ifdef sgi
# include <sys/sysmacros.h>
#endif /* sgi */

#ifdef ISC
# include <sys/bsdtypes.h>
#endif
#include <sys/stat.h>
#ifndef sgi
# include <sys/file.h>
#endif /* sgi */
#ifndef sun
# include <sys/ioctl.h>
#endif /* sun */

#include <signal.h>

#include "config.h"

#ifdef SVR4
# include <sys/stropts.h>
#endif

#ifdef SYSV
# include <sys/utsname.h>
#endif

#if defined(sequent) || defined(SVR4)
# include <sys/resource.h>
#endif /* sequent || SVR4 */

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif /* ISC */

#include "screen.h"

#include "patchlevel.h"

#include <pwd.h>
#ifdef SHADOWPW
# include <shadow.h>
#endif /* SHADOWPW */


#ifdef DEBUG
FILE *dfp;
#endif


extern char *blank, *null, Term[], screenterm[], **environ, Termcap[];
int force_vt = 1, assume_LP = 0;
int VBellWait, MsgWait, MsgMinWait, SilenceWait;

extern struct plop plop_tab[];
extern struct user *users;
extern struct display *displays, *display; 
extern struct layer BlankLayer;

/* tty.c */
extern int intrc;


extern int use_hardstatus;
#ifdef COPY_PASTE
extern char mark_key_tab[];
#endif
extern char version[];
extern char DefaultShell[];


char *ShellProg;
char *ShellArgs[2];

extern struct NewWindow nwin_undef, nwin_default, nwin_options;

static char *MakeWinMsg __P((char *, int));
static void  SigChldHandler __P((void));
static sig_t SigChld __P(SIGPROTOARG);
static sig_t SigInt __P(SIGPROTOARG);
static sig_t CoreDump __P((int));
static void  DoWait __P((void));
static void  WindowDied __P((struct win *));


#ifdef PASSWORD
extern char Password[];
#endif


/* the attacher */
struct passwd *ppp;
char *attach_tty;
char *attach_term;
char *LoginName;
struct mode attach_Mode;


#ifdef SOCKDIR
char *SockDir = SOCKDIR;
#else
char *SockDir = ".iscreen";
#endif
extern char SockPath[], *SockNamePtr, *SockName;
int ServerSocket = -1;
char **NewEnv = NULL;

char *RcFileName = NULL;
extern char Esc;
char *home;

char *screenlogdir = NULL;
char *hardcopydir = NULL;
char *BellString;
char *VisualBellString;
char *ActivityString;
#ifdef COPY_PASTE
char *BufferFile;
#endif
#ifdef POW_DETACH
char *PowDetachString;
#endif
int auto_detach = 1;
int iflag, rflag, dflag, lsflag, quietflag, wipeflag, xflag;
int adaptflag;

time_t Now;

#ifdef MULTIUSER
char *multi;
char *multi_home;
int multi_uid;
int own_uid;
int multiattach;
int tty_mode;
int tty_oldmode = -1;
#endif

char HostName[MAXSTR];
int MasterPid;
int real_uid, real_gid, eff_uid, eff_gid;
int default_startup;
int slowpaste;
int ZombieKey;

#ifdef NETHACK
int nethackflag = 0;
#endif


struct win *fore = NULL;
struct win *windows = NULL;
struct win *console_window;



/*
 * Do this last
 */
#include "extern.h"


#ifdef NETHACK
char strnomem[] = "Who was that Maude person anyway?";
#else
char strnomem[] = "Out of memory.";
#endif


static int InterruptPlease = 0;
static int GotSigChld;


static void
mkfdsets(rp, wp)
fd_set *rp, *wp;
{
  register struct win *p;

  FD_ZERO(rp);
  FD_ZERO(wp);
  for (display = displays; display; display = display->_d_next)
    {
      if (d_obufp != d_obuf)
	FD_SET(d_userfd, wp);

      FD_SET(d_userfd, rp);	/* Do that always */

      /* read from terminal if there is room in the destination buffer
       *
       * Removed, so we can always input a command sequence
       *
       * if (d_fore == 0)
       *   continue;
       * if (W_UWP(d_fore))
       *   {
       *      check pseudowin buffer
       *      if (d_fore->w_pwin->p_inlen < sizeof(d_fore->w_pwin->p_inbuf))
       *      FD_SET(d_userfd, rp);
       *   }
       * else
       *   {
       *     check window buffer
       *     if (d_fore->w_inlen < sizeof(d_fore->w_inbuf))
       *     FD_SET(d_userfd, rp);
       *   }
       */
    }
  for (p = windows; p; p = p->w_next)
    {
      if (p->w_ptyfd < 0)
        continue;
#ifdef COPY_PASTE
      if (p->w_pastelen)
        {
	  /* paste to win/pseudo */
# ifdef PSEUDOS
	  FD_SET(W_UWP(p) ? p->w_pwin->p_ptyfd : p->w_ptyfd, wp);
# else
	  FD_SET(p->w_ptyfd, wp);
# endif
	}
#endif
      /* query window buffer */
      if (p->w_inlen > 0)
	FD_SET(p->w_ptyfd, wp);
#ifdef PSEUDOS
      /* query pseudowin buffer */
      if (p->w_pwin && p->w_pwin->p_inlen > 0)
        FD_SET(p->w_pwin->p_ptyfd, wp);
#endif

      display = p->w_display;
      if (p->w_active && d_status && !d_status_bell && !(use_hardstatus && HS))
	continue;
      if (p->w_outlen > 0)
	continue;
      if (p->w_lay->l_block)
	continue;
    /* 
     * Don't accept input from window or pseudowin if there is to much 
     * output pending on display .
     */
      if (p->w_active && (d_obufp - d_obuf) > d_obufmax)
	{
	  debug1("too much output pending, window %d\n", p->w_number);
	  continue;  
	}
#ifdef PSEUDOS
      if (W_RW(p))
	{
	  /* Check free space if we stuff window output in pseudo */
	  if (p->w_pwin && W_WTOP(p) && (p->w_pwin->p_inlen >= sizeof(p->w_pwin->p_inbuf)))
	    {
	      debug2("pseudowin %d buffer full (%d bytes)\n", p->w_number, p->w_pwin->p_inlen);
	    }
	  else
            FD_SET(p->w_ptyfd, rp);
	}
      if (W_RP(p))
	{
	  /* Check free space if we stuff pseudo output in window */
	  if (W_PTOW(p) && p->w_inlen >= sizeof(p->w_inbuf))
	    {
	      debug2("window %d buffer full (%d bytes)\n", p->w_number, p->w_inlen);
	    }
	  else
            FD_SET(p->w_pwin->p_ptyfd, rp);
	}
#else /* PSEUDOS */
      FD_SET(p->w_ptyfd, rp);
#endif /* PSEUDOS */
    }
  FD_SET(ServerSocket, rp);
}

void
main(ac, av)
int ac;
char **av;
{
  register int n, len;
  register struct win *p;
  char *ap;
  char *av0;
  char socknamebuf[2 * MAXSTR];
  fd_set r, w;
  int mflag = 0;
  struct timeval tv;
  int nsel;
  char buf[IOSIZE], *myname = (ac == 0) ? "screen" : av[0];
  struct stat st;
  int buflen, tmp;
#ifdef _MODE_T			/* (jw) */
  mode_t oumask;
#else
  int oumask;
#endif
#ifdef SYSV
  struct utsname utsnam;
#endif
  struct NewWindow nwin;
  int detached = 0;		/* start up detached */
  struct display *ndisplay;
#ifdef MULTIUSER
  char *sockp;
#endif

  /*
   *  First, close all unused descriptors
   *  (otherwise, we might have problems with the select() call)
   */
  closeallfiles(0);
#ifdef DEBUG
    {
      char buf[255];

      sprintf(buf, "%s/screen.%d", DEBUGDIR, getpid());
      (void) mkdir(DEBUGDIR, 0777);
      if ((dfp = fopen(buf, "w")) == NULL)
	dfp = stderr;
      else
	(void) chmod(buf, 0666);
    }
#endif
  sprintf(version, "%d.%.2d.%.2d%s (%s) %s", REV, VERS,
	  PATCHLEVEL, STATE, ORIGIN, DATE);
  debug2("-- screen debug started %s (%s)\n", *av, version);
#ifdef POSIX
  debug("POSIX\n");
#endif
#ifdef TERMIO
  debug("TERMIO\n");
#endif
#ifdef SYSV
  debug("SYSV\n");
#endif
#ifdef SYSVSIGS
  debug("SYSVSIGS\n");
#endif
#ifdef NAMEDPIPE
  debug("NAMEDPIPE\n");
#endif
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
  debug("Window changing enabled\n");
#endif
#ifdef NOREUID
  debug("NOREUID\n");
#endif
#ifdef hpux
  debug("hpux\n");
#endif
#ifdef USEBCOPY
  debug("USEBCOPY\n");
#endif
#ifdef UTMPOK
  debug("UTMPOK\n");
#endif
#ifdef LOADAV
  debug("LOADAV\n");
#endif
#ifdef NETHACK
  debug("NETHACK\n");
#endif
#ifdef TERMINFO
  debug("TERMINFO\n");
#endif
#ifdef SHADOWPW
  debug("SHADOWPW\n");
#endif
#ifdef NAME_MAX
  debug1("NAME_MAX = %d\n", NAME_MAX);
#endif

  BellString = SaveStr("Bell in window %");
  VisualBellString = SaveStr("   Wuff,  Wuff!!  ");
  ActivityString = SaveStr("Activity in window %");
#ifdef COPY_PASTE
  BufferFile = SaveStr(DEFAULT_BUFFERFILE);
#endif
  ShellProg = NULL;
#ifdef POW_DETACH
  PowDetachString = 0;
#endif
  default_startup = (ac > 1) ? 0 : 1;
  adaptflag = 0;
  slowpaste = 0;
  VBellWait = VBELLWAIT;
  MsgWait = MSGWAIT;
  MsgMinWait = MSGMINWAIT;
  SilenceWait = SILENCEWAIT;
#ifdef COPY_PASTE
  CompileKeys((char *)NULL, mark_key_tab);
#endif
  nwin = nwin_undef;
  nwin_options = nwin_undef;

  av0 = *av;
  /* if this is a login screen, assume -R */
  if (*av0 == '-')
    {
      rflag = 2;
#ifdef MULTI
      xflag = 1;
#endif
      ShellProg = SaveStr(DefaultShell); /* to prevent nasty circles */
    }
  while (ac > 0)
    {
      ap = *++av;
      if (--ac > 0 && *ap == '-')
	{
	  if (ap[1] == '-' && ap[2] == 0)
	    {
	      av++;
	      ac--;
	      break;
	    }
	  switch (ap[1])
	    {
	    case 'a':
	      nwin_options.aflag = 1;
	      break;
	    case 'A':
	      adaptflag = 1;
	      break;
	    case 'c':
	      if (ap[2])
		RcFileName = ap + 2;
	      else
		{
		  if (--ac == 0)
		    exit_with_usage(myname);
		  RcFileName = *++av;
		}
	      break;
	    case 'e':
	      if (ap[2])
		ap += 2;
	      else
		{
		  if (--ac == 0)
		    exit_with_usage(myname);
		  ap = *++av;
		}
	      if (ParseEscape(NULL, ap))
		Panic(0, "Two characters are required with -e option.");
	      break;
	    case 'f':
	      switch (ap[2])
		{
		case 'n':
		case '0':
		  nwin_options.flowflag = FLOW_NOW * 0;
		  break;
		case 'y':
		case '1':
		case '\0':
		  nwin_options.flowflag = FLOW_NOW * 1;
		  break;
		case 'a':
		  nwin_options.flowflag = FLOW_AUTOFLAG;
		  break;
		default:
		  exit_with_usage(myname);
		}
	      break;
            case 'h':
	      if (ap[2])
		nwin_options.histheight = atoi(ap + 2);
	      else
		{
		  if (--ac == 0)
		    exit_with_usage(myname);
		  nwin_options.histheight = atoi(*++av);
		}
	      if (nwin_options.histheight < 0)
		exit_with_usage(myname);
	      break;
	    case 'i':
	      iflag = 1;
	      break;
	    case 't': /* title is a synonym for AkA */
	    case 'k':
	      if (ap[2])
		nwin_options.aka = ap + 2;
	      else
		{
		  if (--ac == 0)
		    exit_with_usage(myname);
		  nwin_options.aka = *++av;
		}
	      break;
	    case 'l':
	      switch (ap[2])
		{
		case 'n':
		case '0':
		  nwin_options.lflag = 0;
		  break;
		case 'y':
		case '1':
		case '\0':
		  nwin_options.lflag = 1;
		  break;
		case 's':
		case 'i':
		  lsflag = 1;
		  if (ac > 1)
		    {
		      SockName = *++av;
		      ac--;
		    }
		  break;
		default:
		  exit_with_usage(myname);
		}
	      break;
	    case 'w':
	      lsflag = 1;
	      wipeflag = 1;
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
	    case 'T':
              if (ap[2])
		{
		  if (strlen(ap+2) < 20)
                    strcpy(screenterm, ap + 2);
		}
              else
                {
                  if (--ac == 0)
                    exit_with_usage(myname);
		  if (strlen(*++av) < 20)
                    strcpy(screenterm, *av);
                }
	      nwin_options.term = screenterm;
              break;
	    case 'q':
	      quietflag = 1;
	      break;
	    case 'r':
	    case 'R':
#ifdef MULTI
	    case 'x':
#endif
	      if (ap[2])
		{
		  SockName = ap + 2;
		  if (ac != 1)
		    exit_with_usage(myname);
		}
	      else if (ac > 1 && *av[1] != '-')
		{
		  SockName = *++av;
		  ac--;
		}
#ifdef MULTI
	      if (ap[1] == 'x')
		xflag = 1;
	      else
#endif
	        rflag = (ap[1] == 'r') ? 1 : 2;
	      break;
#ifdef REMOTE_DETACH
	    case 'd':
	      dflag = 1;
	      /* FALLTHROUGH */
	    case 'D':
	      if (!dflag)
		dflag = 2;
	      if (ap[2])
		SockName = ap + 2;
	      if (ac == 2)
		{
		  if (*av[1] != '-')
		    {
		      SockName = *++av;
		      ac--;
		    }
		}
	      break;
#endif
	    case 's':
	      if (ap[2])
		{
		  if (ShellProg)
		    free(ShellProg);
		  ShellProg = SaveStr(ap + 2);
		}
	      else
		{
		  if (--ac == 0)
		    exit_with_usage(myname);
		  if (ShellProg)
		    free(ShellProg);
		  ShellProg = SaveStr(*++av);
		}
	      debug1("ShellProg: '%s'\n", ShellProg);
	      break;
	    case 'S':
	      if (ap[2])
		SockName = ap + 2;
	      else
		{
		  if (--ac == 0)
		    exit_with_usage(myname);
		  SockName = *++av;
		  if (!*SockName)
		    exit_with_usage(myname);
		}
	      break;
	    case 'v':
	      Panic(0, "Screen version %s", version);
	      /* NOTREACHED */
	    default:
	      exit_with_usage(myname);
	    }
	}
      else
	break;
    }
  if (dflag && mflag && SockName && !(rflag || xflag))
    detached = 1;
  nwin = nwin_options;
  if (ac)
    nwin.args = av;
  real_uid = getuid();
  real_gid = getgid();
  eff_uid = geteuid();
  eff_gid = getegid();
  if (eff_uid != real_uid)
    {		
      /* if running with s-bit, we must install a special signal
       * handler routine that resets the s-bit, so that we get a
       * core file anyway.
       */
#ifdef SIGBUS /* OOPS, linux has no bus errors ??? */
      signal(SIGBUS, CoreDump);
#endif /* SIGBUS */
      signal(SIGSEGV, CoreDump);
    }
  if (!ShellProg)
    {
      register char *sh;

      sh = getenv("SHELL");
      ShellProg = SaveStr(sh ? sh : DefaultShell);
    }
  ShellArgs[0] = ShellProg;
#ifdef NETHACK
  nethackflag = (getenv("NETHACKOPTIONS") != NULL);
#endif
#ifdef MULTIUSER
  own_uid = multi_uid = real_uid;
  if (SockName && (sockp = index(SockName, '/')))
    {
      if (eff_uid)
        Panic(0, "Must run suid root for multi support.");
      *sockp = 0;
      multi = SockName;
      SockName = sockp + 1;
      if (*multi)
	{
	  struct passwd *mppp;
	  if ((mppp = getpwnam(multi)) == (struct passwd *) 0)
	    Panic(0, "Cannot identify account '%s'.", multi);
	  multi_uid = mppp->pw_uid;
	  multi_home = SaveStr(mppp->pw_dir);
#ifdef MULTI
	  if (rflag || lsflag)
	    {
	      xflag = 1;
	      rflag = 0;
	    }
#endif
	  detached = 0;
	  multiattach = 1;
	}
    }
  if (SockName && *SockName == 0)
    SockName = 0;
#endif
  if ((LoginName = getlogin()) && LoginName[0] != '\0')
    {
      if ((ppp = getpwnam(LoginName)) != (struct passwd *) 0)
	if (ppp->pw_uid != real_uid)
	  ppp = (struct passwd *) 0;
    }
  if (ppp == 0)
    {
      if ((ppp = getpwuid(real_uid)) == 0)
        {
#ifdef NETHACK
          if (nethackflag)
	    Panic(0, "An alarm sounds through the dungeon...\nWarning, the kops are coming.");
	  else
#endif
	  Panic(0, "getpwuid() can't identify your account!");
	  exit(1);
        }
      LoginName = ppp->pw_name;
    }
  home = getenv("HOME");	/* may or may not return a result. jw. */
#if !defined(SOCKDIR) && defined(MULTIUSER)
  if (multi && !multiattach)
    {
      if (home && strcmp(home, ppp->pw_dir))
        Panic(0, "$HOME must match passwd entry for multi screens");
    }
#endif
  if (home == 0 || *home == '\0')
    home = ppp->pw_dir;
  if (strlen(LoginName) > 20)
    Panic(0, "LoginName too long - sorry.");
  if (strlen(home) > MAXPATHLEN - 25)
    Panic(0, "$HOME too long - sorry.");
#ifdef PASSWORD
  strcpy(Password, ppp->pw_passwd);
#endif

  if (!detached && !lsflag)
    {
      /* ttyname implies isatty */
      if (!(attach_tty = ttyname(0)))
	{
#ifdef NETHACK
	  if (nethackflag)
	    Panic(0, "You must play from a terminal.");
	  else
#endif
	  Panic(0, "Must be connected to a terminal.");
	  exit(1);
	}
      if (strlen(attach_tty) >= MAXPATHLEN)
	Panic(0, "TtyName too long - sorry.");
      if (stat(attach_tty, &st))
	Panic(errno, "Cannot access '%s'", attach_tty);
#ifdef MULTIUSER
      tty_mode = st.st_mode & 0777;
#endif
      if ((n = secopen(attach_tty, O_RDWR, 0)) < 0)
	Panic(0, "Cannot open '%s' - please check.", attach_tty);
      close(n);
      debug1("attach_tty is %s\n", attach_tty);
      if ((attach_term = getenv("TERM")) == 0)
	Panic(0, "Please set a terminal type.");
      if (strlen(attach_term) > sizeof(d_termname) - 1)
	Panic(0, "$TERM too long - sorry.");
      GetTTY(0, &attach_Mode);
#ifdef DEBUGGGGGGGGGGGGGGG
      DebugTTY(&attach_Mode);
#endif /* DEBUG */
    }
  
#ifdef _MODE_T
  oumask = umask(0);		/* well, unsigned never fails? jw. */
#else
  if ((oumask = umask(0)) == -1)
    Panic(errno, "Cannot change umask to zero");
#endif
  if ((SockDir = getenv("ISCREENDIR")) == NULL)
    SockDir = getenv("SCREENDIR");
  if (SockDir)
    {
      if (strlen(SockDir) >= MAXPATHLEN - 1)
	Panic(0, "Ridiculously long $(I)SCREENDIR - try again.");
#ifdef MULTIUSER
      if (multi)
	Panic(0, "No $(I)SCREENDIR with multi screens, please.");
#endif
    }
#ifdef MULTIUSER
  if (multiattach)
    {
# ifndef SOCKDIR
      sprintf(SockPath, "%s/.iscreen", multi_home);
      SockDir = SockPath;
# else
      SockDir = SOCKDIR;
      sprintf(SockPath, "%s/S-%s", SockDir, multi);
# endif
    }
  else
#endif
    {
#ifndef SOCKDIR
      if (SockDir == 0)
	{
	  sprintf(SockPath, "%s/.iscreen", home);
	  SockDir = SockPath;
	}
#endif
      if (SockDir)
	{
	  if (access(SockDir, F_OK))
	    {
	      debug1("SockDir '%s' missing ...\n", SockDir);
	      if (UserContext() > 0)
		{
		  if (mkdir(SockDir, 0700))
		    UserReturn(0);
		  UserReturn(1);
		}
	      if (UserStatus() <= 0)
		Panic(0, "Cannot make directory '%s'.", SockDir);
	    }
	  if (SockDir != SockPath)
	    strcpy(SockPath, SockDir);
	}
#ifdef SOCKDIR
      else
	{
	  SockDir = SOCKDIR;
	  if (stat(SockDir, &st))
	    {
	      if (mkdir(SockDir, eff_uid ? 0777 : 0755) == -1)
		Panic(errno, "Cannot make directory '%s'", SockDir);
	    }
	  else
	    {
	      n = eff_uid ? 0777 : 0755;
	      if ((st.st_mode & 0777) != n)
		Panic(0, "Directory '%s' must have mode %03o.", SockDir, n);
	    }
	  sprintf(SockPath, "%s/S-%s", SockDir, LoginName);
	  if (access(SockPath, F_OK))
	    {
	      if (mkdir(SockPath, 0700) == -1)
		Panic(errno, "Cannot make directory '%s'", SockPath);
	      (void) chown(SockPath, real_uid, real_gid);
	    }
	}
#endif
    }

  if (stat(SockPath, &st) == -1)
    {
      Panic(errno, "Cannot access %s", SockPath);
    }
  else
    {
#ifdef _POSIX_SOURCE
      if (S_ISDIR(st.st_mode) == 0)
#else
      if ((st.st_mode & S_IFMT) != S_IFDIR)
#endif
	Panic(0, "%s is not a directory.", SockPath);
#ifdef MULTIUSER
      if (multi)
	{
	  if (st.st_uid != multi_uid)
	    Panic(0, "%s is not the owner of %s.", multi, SockPath);
	}
      else
#endif
	{
	  if (st.st_uid != real_uid)
	    Panic(0, "You are not the owner of %s.", SockPath);
	}
      if ((st.st_mode & 0777) != 0700)
	Panic(0, "Directory %s must have mode 700.", SockPath);
    }
  strcat(SockPath, "/");
  SockNamePtr = SockPath + strlen(SockPath);
  (void) umask(oumask);
  debug2("SockPath: %s  SockName: %s\n", SockPath, SockName ? SockName : "NULL");

#if defined(SYSV) && !defined(ISC)
  if (uname(&utsnam) == -1)
    Panic(0, "uname() failed, errno = %d.", errno);
  else
    {
      strncpy(HostName, utsnam.nodename, MAXSTR);
      HostName[(sizeof(utsnam.nodename) <= MAXSTR) ? 
               sizeof(utsnam.nodename) : MAXSTR] = '\0';
    }
#else
  (void) gethostname(HostName, MAXSTR);
#endif
  HostName[MAXSTR - 1] = '\0';
  if ((ap = index(HostName, '.')) != NULL)
    *ap = '\0';

  if (lsflag)
    {
      int i;

#ifdef MULTIUSER
      if (multi)
	real_uid = multi_uid;
      setuid(real_uid);
      setgid(real_gid);
      eff_uid = real_uid;
      eff_gid = real_gid;
#endif
      i = FindSocket(0, (int *)NULL);
      /* MakeClientSocket appended the last (Sock)Name there: */
      *SockNamePtr = '\0';
      if (i == 0)
	{
#ifdef NETHACK
          if (nethackflag)
	    Panic(0, "This room is empty (%s).\n", SockPath);
          else
#endif /* NETHACK */
          Panic(0, "No Sockets found in %s.\n", SockPath);
        }
      Panic(0, "%d Socket%s in %s.\n", i, i > 1 ? "s" : "", SockPath);
      /* NOTREACHED */
    }
  signal(SIG_BYE, AttacherFinit);	/* prevent races */
  if (rflag || xflag)
    {
      debug("screen -r: - is there anybody out there?\n");
#ifdef SHADOWPW
      setspent();  /* open shadow file while we are still root */
#endif /* SHADOWPW */
      if (Attach(MSG_ATTACH))
	{
	  Attacher();
	  /* NOTREACHED */
	}
      debug("screen -r: backend not responding -- still crying\n");
    }
  else if (dflag && !mflag)
    {
      (void) Attach(MSG_DETACH);
      Msg(0, "[%s %sdetached.]\n", SockName, (dflag > 1 ? "power " : ""));
      eexit(0);
      /* NOTREACHED */
    }
  if (!SockName && !mflag)
    {
      register char *sty;
      int s;

      if ((sty = getenv("STY")) != 0 && *sty != '\0')
	{
	  setuid(real_uid);
	  setgid(real_gid);
	  eff_uid = real_uid;
	  eff_gid = real_gid;
	  if ((s = MakeClientSocket(1, sty)) > 0)
	    {
	      nwin_options.args = av;
	      SendCreateMsg(s, &nwin);
	      close(s);
	    }
	  exit(0);
	  /* NOTREACHED */
	}
    }
  nwin_compose(&nwin_default, &nwin_options, &nwin_default);
  if (SockName && !*SockName)
    SockName = NULL;
  switch (MasterPid = fork())
    {
    case -1:
      Panic(errno, "fork");
      /* NOTREACHED */
#ifdef FORKDEBUG
    default:
      break;
    case 0:
      MasterPid = getppid();
#else
    case 0:
      break;
    default:
#endif
      if (detached)
        exit(0);
      if (SockName)
	{
	  /* user started us with -S option */
	  sprintf(socknamebuf, "%d.%s", MasterPid, SockName);
	}
      else
	{
	  sprintf(socknamebuf, "%d.%s.%s", MasterPid, stripdev(attach_tty),
		  HostName);
	}
      for (ap = socknamebuf; *ap; ap++)
	if (*ap == '/')
	  *ap = '-';
      SockName = socknamebuf;
#ifdef SHADOWPW
      setspent();  /* open shadow file while we are still root */
#endif /* SHADOWPW */
      setuid(real_uid);
      setgid(real_gid);
      eff_uid = real_uid;
      eff_gid = real_gid;
      Attacher();
      /* NOTREACHED */
    }

  ap = av0 + strlen(av0) - 1;
  while (ap >= av0)
    {
      if (!strncmp("screen", ap, 6))
	{
	  strncpy(ap, "SCREEN", 6); /* name this process "SCREEN-BACKEND" */
	  break;
	}
      ap--;
    }
  if (ap < av0)
    *av0 = 'S';

#ifdef DEBUG
  {
    char buf[256];

    if (dfp && dfp != stderr)
      fclose(dfp);
    sprintf(buf, "%s/SCREEN.%d", DEBUGDIR, getpid());
    if ((dfp = fopen(buf, "w")) == NULL)
      dfp = stderr;
    else
      (void) chmod(buf, 0666);
  }
#endif
  if (!detached)
    n = dup(0);
  else
    n = -1;
  freopen("/dev/null", "r", stdin);
  freopen("/dev/null", "w", stdout);
#ifdef DEBUG
  if (dfp != stderr)
#endif
  freopen("/dev/null", "w", stderr);
  debug("-- screen.back debug started\n");

  if (!detached)
    {
#ifdef FORKDEBUG
      if (MakeDisplay(LoginName, attach_tty, attach_term, n, MasterPid, &attach_Mode) == 0)
#else
      if (MakeDisplay(LoginName, attach_tty, attach_term, n, getppid(), &attach_Mode) == 0)
#endif
	Panic(0, "Could not alloc display");
    }

  if (SockName)
    {
      /* user started us with -S option */
      sprintf(socknamebuf, "%d.%s", getpid(), SockName);
    }
  else
    {
      sprintf(socknamebuf, "%d.%s.%s", getpid(), stripdev(attach_tty),
	      HostName);
    }
  for (ap = socknamebuf; *ap; ap++)
    if (*ap == '/')
      *ap = '-';
  SockName = socknamebuf;
  ServerSocket = MakeServerSocket();
#ifdef ETCSCREENRC
# ifdef ALLOW_SYSSCREENRC
  if ((ap = getenv("SYSSCREENRC")))
    StartRc(ap);
  else
# endif
    StartRc(ETCSCREENRC);
#endif
  StartRc(RcFileName);
# ifdef UTMPOK
#  ifndef UTNOKEEP
  InitUtmp(); 
#  endif /* UTNOKEEP */
# endif /* UTMPOK */
  if (display)
    {
      if (InitTermcap(0, 0))
	{
	  debug("Could not init termcap - exiting\n");
	  fcntl(d_userfd, F_SETFL, 0);	/* Flush sets NDELAY */
	  freetty();
	  if (d_userpid)
	    Kill(d_userpid, SIG_BYE);
	  eexit(1);
	}
      InitTerm(0);
#ifdef UTMPOK
      RemoveLoginSlot();
#endif
    }
  else
    {
      MakeTermcap(1);
    }
#ifdef LOADAV
  InitLoadav();
#endif /* LOADAV */
  MakeNewEnv();
  signal(SIGHUP, SigHup);
  signal(SIGINT, Finit);
  signal(SIGQUIT, Finit);
  signal(SIGTERM, Finit);
#ifdef BSDJOBS
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
#endif
  InitKeytab();
  if (display)
    {
      brktty(d_userfd);
      SetMode(&d_OldMode, &d_NewMode);
      /* Note: SetMode must be called _before_ FinishRc. */
      SetTTY(d_userfd, &d_NewMode);
      if (fcntl(d_userfd, F_SETFL, FNDELAY))
	Msg(errno, "Warning: NDELAY fcntl failed");
    }
  else
    brktty(-1);		/* just try */
#ifdef ETCSCREENRC
# ifdef ALLOW_SYSSCREENRC
  if ((ap = getenv("SYSSCREENRC")))
    FinishRc(ap);
  else
# endif
    FinishRc(ETCSCREENRC);
#endif
  FinishRc(RcFileName);

  debug2("UID %d  EUID %d\n", getuid(), geteuid());
  if (windows == NULL)
    {
      debug("We open one default window, as screenrc did not specify one.\n");
      if (MakeWindow(&nwin) == -1)
	{
	  AddStr("Sorry, could not find a PTY.");
	  sleep(2);
	  Finit(0);
	  /* NOTREACHED */
	}
    }
  if (default_startup)
    display_copyright();
  signal(SIGCHLD, SigChld);
  signal(SIGINT, SigInt);
  tv.tv_usec = 0;
  if (rflag == 2)
    {
#ifdef NETHACK
      if (nethackflag)
        Msg(0, "I can't seem to find a... Hey, wait a minute!  Here comes a screen now.");
      else
#endif
      Msg(0, "New screen...");
      rflag = 0;
    }

  Now = time((time_t *)0);

  for (;;)
    {
      tv.tv_sec = 0;
      /*
       * check for silence
       */
      for (p = windows; p; p = p->w_next)
        {
	  int time_left;

	  if (p->w_tstamp.seconds == 0)
	    continue;
	  debug1("checking silence win %d\n", p->w_number);
	  time_left = p->w_tstamp.lastio + p->w_tstamp.seconds - Now;
	  if (time_left > 0)
	    {
	      if (tv.tv_sec == 0 || time_left < tv.tv_sec)
	        tv.tv_sec = time_left;
	    }
	  else
	    {
	      for (display = displays; display; display = display->_d_next)
	        if (p != d_fore)
		  Msg(0, "Window %d: silence for %d seconds", 
		      p->w_number, p->w_tstamp.seconds);
	      p->w_tstamp.lastio = Now;
	    }
	}

      /*
       * check to see if message line should be removed
       */
      for (display = displays; display; display = display->_d_next)
	{
	  int time_left;

	  if (d_status == 0)
	    continue;
	  debug("checking status...\n");
	  time_left = d_status_time + (d_status_bell?VBellWait:MsgWait) - Now;
	  if (time_left > 0)
	    {
	      if (tv.tv_sec == 0 || time_left < tv.tv_sec)
	        tv.tv_sec = time_left;
	      debug(" not yet.\n");
	    }
	  else
	    {
	      debug(" removing now.\n");
	      RemoveStatus();
	    }
	}
      /*
       * check for I/O on all available I/O descriptors
       */
#ifdef DEBUG
      if (tv.tv_sec)
        debug1("select timeout %d seconds\n", tv.tv_sec);
#endif
      mkfdsets(&r, &w);
      if (GotSigChld && !tv.tv_sec)
	{
	  SigChldHandler();
	  continue;
	}
      if ((nsel = select(FD_SETSIZE, &r, &w, (fd_set *)0, tv.tv_sec ? &tv : (struct timeval *) 0)) < 0)
	{
	  debug1("Bad select - errno %d\n", errno);
	  if (errno != EINTR)
	    Panic(errno, "select");
	  errno = 0;
	  nsel = 0;
	}
#ifdef SELECT_BROKEN
      /* 
       * Sequents select emulation counts an descriptor which is
       * readable and writeable only as one. waaaaa.
       */
      if (nsel)
	nsel = 2 * FD_SETSIZE;
#endif
      if (GotSigChld && !tv.tv_sec)
	{
	  SigChldHandler();
	  continue;
	}
      if (InterruptPlease)
	{
	  debug("Backend received interrupt\n");
	  if (fore)
	    {
	      char ibuf;
	      ibuf = intrc;
#ifdef PSEUDOS
	      write(W_UWP(fore) ? fore->w_pwin->p_ptyfd : fore->w_ptyfd, 
		    &ibuf, 1);
	      debug1("Backend wrote interrupt to %d", fore->w_number);
	      debug1("%s\n", W_UWP(fore) ? " (pseudowin)" : "");
#else
	      write(fore->w_ptyfd, &ibuf, 1);
	      debug1("Backend wrote interrupt to %d\n", fore->w_number);
#endif
	    }
	  InterruptPlease = 0;
	}

      /*
       *   Process a client connect attempt and message
       */
      if (nsel && FD_ISSET(ServerSocket, &r))
	{
          nsel--;
	  debug("Knock - knock!\n");
	  ReceiveMsg();
	  continue;
	}

      /*
       * Write the (already processed) user input to the window
       * descriptors first. We do not want to choke, if he types fast.
       */
      if (nsel)
	{
	  for (p = windows; p; p = p->w_next)
	    {
	      int pastefd = -1;

	      if (p->w_ptyfd < 0)
	        continue;
#ifdef COPY_PASTE
	      if (p->w_pastelen)
		{
		  /*
		   *  Write the copybuffer contents first, if any.
		   */
#ifdef PSEUDOS
		  pastefd = W_UWP(p) ? p->w_pwin->p_ptyfd : p->w_ptyfd;
#else
		  pastefd = p->w_ptyfd;
#endif
		  if (FD_ISSET(pastefd, &w))
		    {
		      debug1("writing pastebuffer (%d)\n", p->w_pastelen);
		      len = write(pastefd, p->w_pasteptr, 
				  (slowpaste > 0) ? 1 :
				  (p->w_pastelen > IOSIZE ? 
				   IOSIZE : p->w_pastelen));
		      if (len < 0)	/* Problems... window is dead */
			p->w_pastelen = 0;
		      if (len > 0)
			{
			  p->w_pasteptr += len;
			  p->w_pastelen -= len;
			}
		      debug1("%d bytes pasted\n", len);
		      if (p->w_pastelen == 0)
			{
			  if (p->w_pastebuf)
			    free(p->w_pastebuf);
			  p->w_pastebuf = 0;
			  p->w_pasteptr = 0;
			  pastefd = -1;
			}
		      if (slowpaste > 0)
			{
			  struct timeval t;

			  debug1("slowpaste %d\n", slowpaste);
			  t.tv_sec = (long) (slowpaste / 1000);
			  t.tv_usec = (long) ((slowpaste % 1000) * 1000);
			  select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t);
			}
		      if (--nsel == 0)
		        break;
		    }
		}
#endif

#ifdef PSEUDOS
	      if (p->w_pwin && p->w_pwin->p_inlen > 0)
	        {
		  /* stuff w_pwin->p_inbuf into pseudowin */
		  tmp = p->w_pwin->p_ptyfd;
		  if (tmp != pastefd && FD_ISSET(tmp, &w))
		    {
		      if ((len = write(tmp, p->w_pwin->p_inbuf, 
				       p->w_pwin->p_inlen)) > 0)
		        {
			  if ((p->w_pwin->p_inlen -= len))
			    bcopy(p->w_pwin->p_inbuf + len, p->w_pwin->p_inbuf,
			    	  p->w_pwin->p_inlen);
			}
		      if (--nsel == 0)
		        break;
		    }
		}
#endif
	      if (p->w_inlen > 0)
		{
		  /* stuff w_inbuf buffer into window */
		  tmp = p->w_ptyfd;
		  if (tmp != pastefd && FD_ISSET(tmp, &w))
		    {
		      if ((len = write(tmp, p->w_inbuf, p->w_inlen)) > 0)
			{
			  if ((p->w_inlen -= len))
			    bcopy(p->w_inbuf + len, p->w_inbuf, p->w_inlen);
			}
		      if (--nsel == 0)
			break;
		    }
		}
	    }
	}
      
      Now = time((time_t *)0);

      if (nsel)
	{
	  for (display = displays; display; display = ndisplay)
	    {
	      int maxlen;

	      ndisplay = display->_d_next;
	      /* 
	       * stuff d_obuf into user's tty
	       */
	      if (FD_ISSET(d_userfd, &w)) 
		{
		  int size = OUTPUT_BLOCK_SIZE;

		  len = d_obufp - d_obuf;
		  if (len < size)
		    size = len;
		  ASSERT(len >= 0);
		  size = write(d_userfd, d_obuf, size);
		  if (size >= 0) 
		    {
		      len -= size;
		      if (len)
		        {
			  bcopy(d_obuf + size, d_obuf, len);
		          debug2("ASYNC: wrote %d - remaining %d\n", size, len);
			}
		      d_obufp -= size;
		      d_obuffree += size;
		    } 
		  else
		    {
		      if (errno != EINTR)
# ifdef EWOULDBLOCK
			if (errno != EWOULDBLOCK)
# endif
			Msg(errno, "Error writing output to display");
		    }
		  if (--nsel == 0)
		    break;
		}
	      /*
	       * O.k. All streams are fed, now look what comes back
	       * to us. First of all: user input.
	       */
	      if (! FD_ISSET(d_userfd, &r))
		continue;
	      if (d_status && !(use_hardstatus && HS))
		RemoveStatus();
	      if (d_fore == 0)
		maxlen = IOSIZE;
	      else
		{
#ifdef PSEUDOS
		  if (W_UWP(d_fore))
		    maxlen = sizeof(d_fore->w_pwin->p_inbuf) - d_fore->w_pwin->p_inlen;
		  else
#endif
		    maxlen = sizeof(d_fore->w_inbuf) - d_fore->w_inlen;
		}
	      if (maxlen > IOSIZE)
		maxlen = IOSIZE;
	      if (maxlen <= 0)
		maxlen = 1;	/* Allow one char for command keys */
	      if (d_ESCseen)
		{
		  if (maxlen == 1)
		    maxlen = 2;	/* Allow one char for command keys */
		  buf[0] = d_user->u_Esc;
		  buflen = read(d_userfd, buf + 1, maxlen - 1) + 1;
		  d_ESCseen = 0;
		}
	      else
		buflen = read(d_userfd, buf, maxlen);
	      if (buflen < 0)
		{
		  if (errno == EINTR)
		    continue;
		  debug1("Read error: %d - SigHup()ing!\n", errno);
		  SigHup(SIGARG);
		  sleep(1);
		}
	      else if (buflen == 0)
		{
		  debug("Found EOF - SigHup()ing!\n");
		  SigHup(SIGARG);
		  sleep(1);
		}
	      else
		{
	          /* This refills inbuf or p_inbuf */
	          ProcessInput(buf, buflen);
		}
	      if (--nsel == 0)
		break;
	    }
	}
	
      /* 
       * Read and process the output from the window descriptors 
       */ 
      for (p = windows; p; p = p->w_next) 
	{
	  if (p->w_lay->l_block)
	    continue;
	  display = p->w_display;
	  if (p->w_outlen)
	    WriteString(p, p->w_outbuf, p->w_outlen);
	  else if (p->w_ptyfd >= 0)
	    {
#ifdef PSEUDOS
	      /* gather pseudowin output */
	      if (W_RP(p) && nsel && FD_ISSET(p->w_pwin->p_ptyfd, &r))
	        {
		  nsel--;
		  n = 0;
		  if (W_PTOW(p))
		    {
		      /* Window wants a copy of the pseudowin output */
		      tmp = sizeof(p->w_inbuf) - p->w_inlen;
		      ASSERT(tmp > 0);
		      n++;
		    }
		  else
		    tmp = IOSIZE;
		  if ((len = read(p->w_pwin->p_ptyfd, buf, tmp)) <= 0)
		    {
		      if (errno != EINTR)
#ifdef EWOULDBLOCK
		        if (errno != EWOULDBLOCK)
#endif
			  {
			    debug2("Window %d: pseudowin read error (errno %d) -- removing pseudowin\n", p->w_number, len ? errno : 0);
			    FreePseudowin(p);
			  }
		    }
/* HERE WE ASSUME THAT THERE IS NO PACKET MODE ON PSEUDOWINS */
		  else
		    {
		      if (n)
			{
			  bcopy(buf, p->w_inbuf + p->w_inlen, len);
			  p->w_inlen += len;
			}
		      WriteString(p, buf, len);
		    }
		}
#endif /* PSEUDOS */
	      /* gather window output */
	      if (nsel && FD_ISSET(p->w_ptyfd, &r))
		{
		  nsel--;
#ifdef PSEUDOS
		  n = 0;
		  ASSERT(W_RW(p));
		  if (p->w_pwin && W_WTOP(p))
		    {
		      /* Pseudowin wants a copy of the window output */
		      tmp = sizeof(p->w_pwin->p_inbuf) - p->w_pwin->p_inlen;
		      ASSERT(tmp > 0);
		      n++;
		    }
		  else
#endif
		    tmp = IOSIZE;
		  if ((len = read(p->w_ptyfd, buf, tmp)) <= 0)
		    {
		      if (errno == EINTR)
			continue;
#ifdef EWOULDBLOCK
		      if (errno == EWOULDBLOCK)
			continue;
#endif
		      debug2("Window %d: read error (errno %d) - killing window\n", p->w_number, len ? errno : 0);
		      WindowDied(p);
		      nsel = 0;	/* KillWindow may change window order */
		      break;	/* so we just break */
		    }
#ifdef TIOCPKT
		  if ((p->w_t.flags & TTY_FLAG_PLAIN) == 0)
		    {
		      if (buf[0])
			{
			  debug1("PAKET %x\n", buf[0]);
			  if (buf[0] & TIOCPKT_NOSTOP)
			    NewAutoFlow(p, 0);
			  if (buf[0] & TIOCPKT_DOSTOP)
			    NewAutoFlow(p, 1);
			}
		      if (len > 1)
			{
#ifdef PSEUDOS
			  if (n)
			    {
			      bcopy(buf + 1, 
				    p->w_pwin->p_inbuf + p->w_pwin->p_inlen,
				    len - 1);
			      p->w_pwin->p_inlen += len - 1;
			    }
#endif
			  WriteString(p, buf + 1, len - 1);
			}
		    }
		  else
#endif /* TIOCPKT */
		    {
		      if (len > 0)
			{
#ifdef PSEUDOS
			  if (n)
			    {
			      bcopy(buf, p->w_pwin->p_inbuf + p->w_pwin->p_inlen,
				    len);
			      p->w_pwin->p_inlen += len;
			    }
#endif
			  WriteString(p, buf, len);
			}
		    }
		}
	    }
	  if (p->w_bell == BELL_ON)
	    {
	      p->w_bell = BELL_MSG;
	      for (display = displays; display; display = display->_d_next)
	        Msg(0, MakeWinMsg(BellString, p->w_number));
	      if (p->w_monitor == MON_FOUND)
		p->w_monitor = MON_DONE;
	    }
	  else if (p->w_bell == BELL_VISUAL)
	    {
	      if (display && !d_status_bell)
		{
		  /*
		   * Stop the '!' appearing in the ^A^W display if it is an 
		   * active at the time of the bell. (Tim MacKenzie)
		   */
		  p->w_bell = BELL_OFF; 
		  Msg(0, VisualBellString);
		  if (d_status)
		    d_status_bell = 1;
		}
	    }
	  if (p->w_monitor == MON_FOUND)
	    {
	      p->w_monitor = MON_MSG;
	      for (display = displays; display; display = display->_d_next)
	        Msg(0, MakeWinMsg(ActivityString, p->w_number));
	    }
	}
#if defined(DEBUG) && !defined(_SEQUENT_)
      if (nsel)
	debug1("*** Left over nsel: %d\n", nsel);
#endif
    }
  /* NOTREACHED */
}

static void
WindowDied(p)
struct win *p;
{
  if (ZombieKey)
    {
      char buf[100];
      struct tm *tp;
      time_t now;

      (void) time(&now);
      tp = localtime(&now);
      debug3("window %d (%s) going into zombie state fd %d",
	     p->w_number, p->w_title, p->w_ptyfd);
#ifdef UTMPOK
      RemoveUtmp(p);
#endif
      (void) chmod(p->w_tty, 0666);
      (void) chown(p->w_tty, 0, 0);
      close(p->w_ptyfd);
      p->w_ptyfd = -1;
      p->w_pid = 0;
      ResetWindow(p);
      p->w_y = p->w_bot;
      sprintf(buf, "\n=== Window terminated at %2d:%02d:%02d ===", tp->tm_hour, tp->tm_min, tp->tm_sec);
      WriteString(p, buf, strlen(buf));
    }
  else
    KillWindow(p);
}

static void
SigChldHandler()
{
  struct stat st;
  while (GotSigChld)
    {
      GotSigChld = 0;
      DoWait();
#ifdef SYSVSIGS
      signal(SIGCHLD, SigChld);
#endif
    }
  if (stat(SockPath, &st) == -1)
    {
      debug1("SigChldHandler: Yuck! cannot stat '%s'\n", SockPath);
      if (!RecoverSocket())
	{
	  debug("SCREEN cannot recover from corrupt Socket, bye\n");
	  Finit(1);
	}
      else
	debug1("'%s' reconstructed\n", SockPath);
    }
  else
    debug2("SigChldHandler: stat '%s' o.k. (%03o)\n", SockPath, st.st_mode);
}

static sig_t
SigChld(SIGDEFARG)
{
  debug("SigChld()\n");
  GotSigChld = 1;
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

sig_t
SigHup(SIGDEFARG)
{
  if (display == 0)
    return;
  debug("SigHup()\n");
  if (d_userfd >= 0)
    {
      close(d_userfd);
      d_userfd = -1;
    }
  if (auto_detach || displays->_d_next)
    Detach(D_DETACH);
  else
    Finit(0);
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

/* 
 * the backend's Interrupt handler
 * we cannot d_insert the intrc directly, as we never know
 * if fore is valid.
 */
static sig_t
SigInt(SIGDEFARG)
{
#if HAZARDOUS
  char buf[1];

  debug("SigInt()\n");
  *buf = (char) intrc;
  if (fore)
    fore->w_inlen = 0;
  if (fore)
    write(fore->w_ptyfd, buf, 1);
#else
  signal(SIGINT, SigInt);
  debug("SigInt() careful\n");
  InterruptPlease = 1;
#endif
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

static sig_t
CoreDump(sig)
int sig;
{
  struct display *disp;
  char buf[80];

#ifdef SYSVSIGS
  signal(sig, SIG_IGN);
#endif /* SYSV */
  setgid(getgid());
  setuid(getuid());
  unlink("core");
  sprintf(buf, "\r\n[screen caught signal %d.%s]\r\n", sig,
#if defined(SHADOWPW) && !defined(DEBUG) && !defined(DUMPSHADOW)
              ""
#else /* SHADOWPW  && !DEBUG */
              " (core dumped)"
#endif /* SHADOWPW  && !DEBUG */
              );
  for (disp = displays; disp; disp = disp->_d_next)
    {
      fcntl(disp->_d_userfd, F_SETFL, 0);
      write(disp->_d_userfd, buf, strlen(buf));
      Kill(disp->_d_userpid, SIG_BYE);
    }
#if defined(SHADOWPW) && !defined(DEBUG) && !defined(DUMPSHADOW)
  eexit(sig);
#else /* SHADOWPW && !DEBUG */
  abort();
#endif /* SHADOWPW  && !DEBUG */
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

static void
DoWait()
{
  register int pid;
  struct win *p, *next;
#ifdef BSDWAIT
  union wait wstat;
#else
  int wstat;
#endif

#ifdef BSDJOBS
# ifndef BSDWAIT
  while ((pid = waitpid(-1, &wstat, WNOHANG | WUNTRACED)) > 0)
# else
# ifdef USE_WAIT2
  /* 
   * From: rouilj@sni-usa.com (John Rouillard) 
   * note that WUNTRACED is not documented to work, but it is defined in
   * /usr/include/sys/wait.h, so it may work 
   */
  while ((pid = wait2(&wstat, WNOHANG | WUNTRACED )) > 0)
#  else /* USE_WAIT2 */
  while ((pid = wait3(&wstat, WNOHANG | WUNTRACED, (struct rusage *) 0)) > 0)
#  endif /* USE_WAIT2 */
# endif
#else	/* BSDJOBS */
  while ((pid = wait(&wstat)) < 0)
    if (errno != EINTR)
      break;
  if (pid > 0)
#endif	/* BSDJOBS */
    {
      for (p = windows; p; p = next)
	{
	  next = p->w_next;
	  if (pid == p->w_pid)
	    {
#ifdef BSDJOBS
	      if (WIFSTOPPED(wstat))
		{
		  debug3("Window %d pid %d: WIFSTOPPED (sig %d)\n", p->w_number, p->w_pid, WSTOPSIG(wstat));
#ifdef SIGTTIN
		  if (WSTOPSIG(wstat) == SIGTTIN)
		    {
		      Msg(0, "Suspended (tty input)");
		      continue;
		    }
#endif
#ifdef SIGTTOU
		  if (WSTOPSIG(wstat) == SIGTTOU)
		    {
		      Msg(0, "Suspended (tty output)");
		      continue;
		    }
#endif
		  /* Try to restart process */
# ifdef NETHACK	
                  if (nethackflag)
		    Msg(0, "You regain consciousness.");
		  else
# endif /* NETHACK */
		  Msg(0, "Child has been stopped, restarting.");
		  if (killpg(p->w_pid, SIGCONT))
		    kill(p->w_pid, SIGCONT);
		}
	      else
#endif
		{
		  WindowDied(p);
		}
	      break;
	    }
#ifdef PSEUDOS
	  if (p->w_pwin && pid == p->w_pwin->p_pid)
	    {
	      debug2("pseudo of win Nr %d died. pid == %d\n", p->w_number, p->w_pwin->p_pid);
	      FreePseudowin(p);
	      break;
	    }
#endif
	}
      if (p == 0)
	{
	  debug1("pid %d not found - hope that's ok\n", pid);
	}
    }
}


sig_t
Finit(i)
int i;
{
  struct win *p, *next;

  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  debug1("Finit(%d);\n", i);
  for (p = windows; p; p = next)
    {
      next = p->w_next;
      FreeWindow(p);
    }
  if (ServerSocket != -1)
    {
      debug1("we unlink(%s)\n", SockPath);
#ifndef NOREUID
      setreuid(eff_uid, real_uid);
      setregid(eff_gid, real_gid);
#endif
      (void) unlink(SockPath);
#ifndef NOREUID
      setreuid(real_uid, eff_uid);
      setregid(real_gid, eff_gid);
#endif
    }
  for (display = displays; display; display = display->_d_next)
    {
      if (d_status)
	RemoveStatus();
      FinitTerm();
#ifdef UTMPOK
      RestoreLoginSlot();
#endif
      AddStr("[screen is terminating]\r\n");
      Flush();
      SetTTY(d_userfd, &d_OldMode);
      fcntl(d_userfd, F_SETFL, 0);
      freetty();
      Kill(d_userpid, SIG_BYE);
    }
  /*
   * we _cannot_ call eexit(i) here, 
   * instead of playing with the Socket above. Sigh.
   */
  exit(i);
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

void
eexit(e)
int e;
{
  if (ServerSocket != -1)
    {
      debug1("we unlink(%s)\n", SockPath);
      setuid(real_uid);
      setgid(real_gid);
      (void) unlink(SockPath);
    }
  exit(e);
}


/*
 * Detach now has the following modes:
 *	D_DETACH	SIG_BYE		detach backend and exit attacher
 *	D_STOP		SIG_STOP	stop attacher (and detach backend)
 *	D_REMOTE	SIG_BYE		remote detach -- reattach to new attacher
 *	D_POWER 	SIG_POWER_BYE 	power detach -- attacher kills his parent
 *	D_REMOTE_POWER	SIG_POWER_BYE	remote power detach -- both
 *	D_LOCK		SIG_LOCK	lock the attacher
 * (jw)
 * we always remove our utmp slots. (even when "lock" or "stop")
 * Note: Take extra care here, we may be called by interrupt!
 */
void
Detach(mode)
int mode;
{
  int sign = 0, pid;
#ifdef UTMPOK
  struct win *p;
#endif

  if (display == 0)
    return;
  signal(SIGHUP, SIG_IGN);
  debug1("Detach(%d)\n", mode);
  if (d_status)
    RemoveStatus();
  FinitTerm();
  switch (mode)
    {
    case D_DETACH:
      AddStr("[detached]\r\n");
      sign = SIG_BYE;
      break;
#ifdef BSDJOBS
    case D_STOP:
      sign = SIG_STOP;
      break;
#endif
#ifdef REMOTE_DETACH
    case D_REMOTE:
      AddStr("[remote detached]\r\n");
      sign = SIG_BYE;
      break;
#endif
#ifdef POW_DETACH
    case D_POWER:
      AddStr("[power detached]\r\n");
      if (PowDetachString) 
	{
	  AddStr(expand_vars(PowDetachString));
	  AddStr("\r\n");
	}
      sign = SIG_POWER_BYE;
      break;
#ifdef REMOTE_DETACH
    case D_REMOTE_POWER:
      AddStr("[remote power detached]\r\n");
      if (PowDetachString) 
	{
	  AddStr(expand_vars(PowDetachString));
	  AddStr("\r\n");
	}
      sign = SIG_POWER_BYE;
      break;
#endif
#endif
    case D_LOCK:
      ClearDisplay();
      sign = SIG_LOCK;
      /* tell attacher to lock terminal with a lockprg. */
      break;
    }
#ifdef UTMPOK
  if (displays->_d_next == 0)
    {
      for (p = windows; p; p = p->w_next)
	if (p->w_slot != (slot_t) -1)
	  {
	    RemoveUtmp(p);
	    /*
	     * Set the slot to 0 to get the window
	     * logged in again.
	     */
	    p->w_slot = (slot_t) 0;
	  }
      if (console_window)
	{
	  if (TtyGrabConsole(console_window->w_ptyfd, 0, "detach"))
	    {
	      debug("could not release console - killing window\n");
	      KillWindow(console_window);
	      display = displays;
	    }
	}
    }
  RestoreLoginSlot();
#endif
  if (d_fore)
    {
      ReleaseAutoWritelock(display, d_fore);
      if (d_fore->w_tstamp.seconds)
        d_fore->w_tstamp.lastio = Now;
      d_fore->w_active = 0;
      d_fore->w_display = 0;
      d_lay = &BlankLayer;
      d_layfn = d_lay->l_layfn;
      d_user->u_detachwin = d_fore->w_number;
    }
  while (d_lay != &BlankLayer)
    ExitOverlayPage();
  if (d_userfd >= 0)
    {
      Flush();
      SetTTY(d_userfd, &d_OldMode);
      fcntl(d_userfd, F_SETFL, 0);
    }
  freetty();
  pid = d_userpid;
  debug2("display: %#x displays: %#x\n", (unsigned int)display, (unsigned int)displays);
  FreeDisplay();
  if (displays == 0)
    /* Flag detached-ness */
    (void) chsock();
  /*
   * tell father to father what to do. We do that after we
   * freed the tty, thus getty feels more comfortable on hpux
   * if it was a power detach.
   */
  Kill(pid, sign);
  debug2("Detach: Signal %d to Attacher(%d)!\n", sign, pid);
  debug("Detach returns, we are successfully detached.\n");
  signal(SIGHUP, SigHup);
}

static int
IsSymbol(e, s)
register char *e, *s;
{
  register int l;

  l = strlen(s);
  return strncmp(e, s, l) == 0 && e[l] == '=';
}

void
MakeNewEnv()
{
  register char **op, **np;
  static char stybuf[MAXSTR];

  for (op = environ; *op; ++op)
    ;
  if (NewEnv)
    free(NewEnv);
  NewEnv = np = (char **) malloc((unsigned) (op - environ + 7 + 1) * sizeof(char **));
  if (!NewEnv)
    Panic(0, strnomem);
  SockName = SockNamePtr;
  if (strlen(SockNamePtr) > MAXSTR - 5)
    SockName = "?";
  sprintf(stybuf, "STY=%s", SockNamePtr);
  *np++ = stybuf;	                /* NewEnv[0] */
  *np++ = Term;	                /* NewEnv[1] */
  np++;		/* room for SHELL */
#ifdef TIOCSWINSZ
  np += 2;	/* room for TERMCAP and WINDOW */
#else
  np += 4;	/* room for TERMCAP WINDOW LINES COLUMNS */
#endif

  for (op = environ; *op; ++op)
    {
debug1("MakeNewEnv: %s\n", *op);
      if (!IsSymbol(*op, "TERM") && !IsSymbol(*op, "TERMCAP")
	  && !IsSymbol(*op, "STY") && !IsSymbol(*op, "WINDOW")
	  && !IsSymbol(*op, "SCREENCAP") && !IsSymbol(*op, "SHELL")
	  && !IsSymbol(*op, "LINES") && !IsSymbol(*op, "COLUMNS")
	 )
	*np++ = *op;
    }
  *np = 0;
}

void
#ifdef USEVARARGS
/*VARARGS2*/
# if defined(__STDC__)
Msg(int err, char *fmt, ...)
# else /* __STDC__ */
Msg(err, fmt, va_alist)
int err;
char *fmt;
va_dcl
# endif /* __STDC__ */
{
  static va_list ap;
#else /* USEVARARRGS */
/*VARARGS2*/
Msg(err, fmt, p1, p2, p3, p4, p5, p6)
int err;
char *fmt;
unsigned long p1, p2, p3, p4, p5, p6;
{
#endif /* USEVARARRGS */
  char buf[MAXPATHLEN*2];
  char *p = buf;

#ifdef USEVARARGS
# if defined(__STDC__)
  va_start(ap, fmt);
# else /* __STDC__ */
  va_start(ap);
# endif /* __STDC__ */
  (void) vsprintf(p, fmt, ap);
  va_end(ap);
#else /* USEVARARRGS */
  sprintf(p, fmt, p1, p2, p3, p4, p5, p6);
#endif /* USEVARARRGS */
  if (err)
    {
      p += strlen(p);
      if (err > 0 && err < sys_nerr)
	sprintf(p, ": %s", sys_errlist[err]);
      else
	sprintf(p, ": Error %d", err);
    }
  debug2("Msg('%s') (%#x);\n", buf, (unsigned int)display);
  if (display)
    MakeStatus(buf);
  else if (displays)
    {
      for (display = displays; display; display = display->_d_next)
	MakeStatus(buf);
    }
  else
    printf("%s\r\n", buf);
}

void
#ifdef USEVARARGS
/*VARARGS2*/
# if defined(__STDC__)
Panic(int err, char *fmt, ...)
# else /* __STDC__ */
Panic(err, fmt, va_alist)
int err;
char *fmt;
va_dcl
# endif /* __STDC__ */
{
  static va_list ap;
#else /* USEVARARRGS */
/*VARARGS2*/
Panic(err, fmt, p1, p2, p3, p4, p5, p6)
int err;
char *fmt;
unsigned long p1, p2, p3, p4, p5, p6;
{
#endif /* USEVARARRGS */
  char buf[MAXPATHLEN*2];
  char *p = buf;

#ifdef USEVARARGS
# if defined(__STDC__)
  va_start(ap, fmt);
# else /* __STDC__ */
  va_start(ap);
# endif /* __STDC__ */
  (void) vsprintf(p, fmt, ap);
  va_end(ap);
#else /* USEVARARRGS */
  sprintf(p, fmt, p1, p2, p3, p4, p5, p6);
#endif /* USEVARARRGS */
  if (err)
    {
      p += strlen(p);
      if (err > 0 && err < sys_nerr)
	sprintf(p, ": %s", sys_errlist[err]);
      else
	sprintf(p, ": Error %d", err);
    }
  debug1("Panic('%s');\n", buf);
  if (displays == 0)
    printf("%s\r\n", buf);
  else
    for (display = displays; display; display = display->_d_next)
      {
        if (d_status)
	  RemoveStatus();
        FinitTerm();
        Flush();
#ifdef UTMPOK
        RestoreLoginSlot();
#endif
        SetTTY(d_userfd, &d_OldMode);
        fcntl(d_userfd, F_SETFL, 0);
        write(d_userfd, buf, strlen(buf));
        write(d_userfd, "\n", 1);
        freetty();
	if (d_userpid)
	  Kill(d_userpid, SIG_BYE);
      }
#ifdef MULTIUSER
  if (tty_oldmode >= 0)
    {
# ifdef NOREUID
      setuid(eff_uid);
# else
      setreuid(real_uid, eff_uid);
# endif
      debug1("Panic: changing back modes from %s\n", attach_tty);
      chmod(attach_tty, tty_oldmode);
    }
#endif
  eexit(1);
}


/*
 * '^' is allowed as an escape mechanism for control characters. jw.
 */
static char *
MakeWinMsg(s, n)
register char *s;
int n;
{
  static char buf[MAXSTR];
  register char *p = buf;
  register int ctrl;

  ctrl = 0;
  for (; *s && p < buf + MAXSTR - 1; s++, p++)
    if (ctrl)
      {
        ctrl = 0;
        if (*s == '^' || *s < 64)
          *p = *s;
        else 
          *p = *s - 64;
      }
    else
      {
        switch (*s)
          {
          case '%':
	    sprintf(p, "%d", n);
	    p += strlen(p) - 1;
	    break;
          case '~':
	    *p = BELL;
	    break;
	  case '^':
	    ctrl = 1;
	    *p-- = '^';
	    break;
          default:
	    *p = *s;
	    break;
          }
      }
  *p = '\0';
  return buf;
}

void
DisplaySleep(n)
int n;
{
  char buf;
  fd_set r;
  struct timeval t;

  if (!display)
    {
      debug("DisplaySleep has no display sigh\n");
      sleep(n);
      return;
    }
  t.tv_usec = 0;
  t.tv_sec = n;
  FD_ZERO(&r);
  FD_SET(d_userfd, &r);
  if (select(FD_SETSIZE, &r, (fd_set *)0, (fd_set *)0, &t) > 0)
    {
      debug("display activity stopped sleep\n");
      read(d_userfd, &buf, 1);
    }
  debug1("DisplaySleep(%d) ending\n", n);
}
