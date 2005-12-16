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
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include "config.h"
#include "screen.h"
#include "extern.h"

#ifdef SHADOWPW
# include <shadow.h>
#endif /* SHADOWPW */

static sig_t AttacherSigInt __P(SIGPROTOARG);
static void  screen_builtin_lck __P((void));
#ifdef PASSWORD
static void  trysend __P((int, struct msg *, char *));
#endif
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
static sig_t AttacherWinch __P(SIGPROTOARG);
#endif
#if defined(LOCK)
static sig_t DoLock __P(SIGPROTOARG);
static void  LockTerminal __P((void));
static sig_t LockHup __P(SIGPROTOARG);
#endif
#ifdef DEBUG
static sig_t AttacherChld __P(SIGPROTOARG);
#endif

extern int real_uid, real_gid, eff_uid, eff_gid;
extern char *SockName, *SockNamePtr, SockPath[];
extern struct passwd *ppp;
extern char *attach_tty, *attach_term, *LoginName;
extern int xflag, dflag, rflag, quietflag, adaptflag;
extern struct mode attach_Mode;
extern int MasterPid;
extern int nethackflag;

#ifdef MULTIUSER
extern char *multi;
extern int multiattach, multi_uid, own_uid;
extern int tty_mode, tty_oldmode;
# ifdef NOREUID
static int multipipe[2];
# endif
#endif



/*
 *  Send message to a screen backend.
 *  returns 1 if we could attach one, or 0 if none.
 */

int
Attach(how)
int how;
{
  int lasts;
  struct msg m;
  struct stat st;
  char *s;

  debug2("Attach: how=%d, tty=%s\n", how, attach_tty);
#ifdef MULTIUSER
  while ((how == MSG_ATTACH || how == MSG_CONT) && multiattach)
    {
# ifdef NOREUID
      int ret;

      if (pipe(multipipe))
	Panic(errno, "pipe");
# endif
      if (chmod(attach_tty, 0666))
	Panic(errno, "chmod %s", attach_tty);
      tty_oldmode = tty_mode;
# ifdef NOREUID
      eff_uid = -1;	/* make UserContext fork */
      real_uid = multi_uid;
      if ((ret = UserContext()) <= 0)
	{
	  char dummy;
          eff_uid = 0;
	  real_uid = own_uid;
	  if (ret < 0)
	    Panic(errno, "UserContext");
	  close(multipipe[1]);
	  read(multipipe[0], &dummy, 1);
	  if (tty_oldmode >= 0)
	    {
	      chmod(attach_tty, tty_oldmode);
	      tty_oldmode = -1;
	    }
	  ret = UserStatus();
#ifdef LOCK
	  if (ret == SIG_LOCK)
	    LockTerminal();
	  else
#endif
#ifdef SIGTSTP
	  if (ret == SIG_STOP)
	    kill(getpid(), SIGTSTP);
	  else
#endif
	  if (ret == SIG_POWER_BYE)
	    {
	      int ppid;
	      setuid(real_uid);
	      setgid(real_gid);
	      if ((ppid = getppid()) > 1)
		Kill(ppid, SIGHUP);
	      exit(0);
	    }
	  else
            exit(ret);
	  how = MSG_ATTACH;
	  continue;
	}
      close(multipipe[0]);
      eff_uid  = real_uid;
# else /* NOREUID */
      real_uid = multi_uid;
      eff_uid  = own_uid;
      setreuid(real_uid, eff_uid);
      if (chmod(attach_tty, 0666))
	Panic(errno, "chmod %s", attach_tty);
# endif /* NOREUID */
      break;
    }
#endif /* MULTIUSER */

  bzero((char *) &m, sizeof(m));
  m.type = how;
  strcpy(m.m_tty, attach_tty);

  if (how == MSG_WINCH)
    {
      if ((lasts = MakeClientSocket(0, SockName)) >= 0)
	{
          write(lasts, (char *)&m, sizeof(m));
          close(lasts);
	}
      return 0;
    }

  if (how == MSG_CONT)
    {
      if ((lasts = MakeClientSocket(0, SockName)) < 0)
        {
          Panic(0, "Sorry, cannot contact session \"%s\" again\r\n",
                 SockName ? SockName : "<NULL>");
        }
    }
  else
    {
      switch (FindSocket(how, &lasts))
	{
	case 0:
	  if (rflag == 2)
	    return 0;
	  if (quietflag)
	    eexit(10);
	  Panic(0, SockName && *SockName ? "There is no screen to be %sed matching %s." : "There is no screen to be %sed.",
		xflag ? "attach" :
		dflag ? "detach" :
                        "resum", SockName);
	  /* NOTREACHED */
	case 1:
	  break;
	default:
	  Panic(0, "Type \"screen [-d] -r [pid.]tty.host\" to resume one of them.");
	  /* NOTREACHED */
	}
    }
  /*
   * Go in UserContext. Advantage is, you can kill your attacher
   * when things go wrong. Any disadvantages? jw.
   * Do this before the attach to prevent races!
   */
#ifdef MULTIUSER
  if (!multiattach)
#endif
    setuid(real_uid);
  setgid(real_gid);

  SockName = SockNamePtr;
  MasterPid = 0;
  while (*SockName)
    {
      if (*SockName > '9' || *SockName < '0')
	break;
      MasterPid = 10 * MasterPid + *SockName - '0';
      SockName++;
    }
  SockName = SockNamePtr;
  debug1("Attach decided, it is '%s'\n", SockPath);
  debug1("Attach found MasterPid == %d\n", MasterPid);
  if (stat(SockPath, &st) == -1)
    Panic(errno, "stat %s", SockPath);
  if ((st.st_mode & 0600) != 0600)
    Panic(0, "Socket is in wrong mode (%03o)", st.st_mode);
  if (!xflag && (st.st_mode & 0700) != (dflag ? 0700 : 0600))
    Panic(0, "That screen is %sdetached.", dflag ? "already " : "not ");
#ifdef REMOTE_DETACH
  if (dflag && !xflag &&
      (how == MSG_ATTACH || how == MSG_DETACH || how == MSG_POW_DETACH))
    {
      m.m.detach.dpid = getpid();
      strncpy(m.m.detach.duser, LoginName, sizeof(m.m.detach.duser) - 1); 
      m.m.detach.duser[sizeof(m.m.detach.duser) - 1] = 0;
# ifdef POW_DETACH
      if (dflag == 2)
	m.type = MSG_POW_DETACH;
      else
# endif
	m.type = MSG_DETACH;
      if (write(lasts, (char *) &m, sizeof(m)) != sizeof(m))
	Panic(errno, "write");
      close(lasts);
      if (how != MSG_ATTACH)
	return 0;	/* we detached it. jw. */
      sleep(1);	/* we dont want to overrun our poor backend. jw. */
      if ((lasts = MakeClientSocket(0, SockName)) == -1)
	Panic(0, "Cannot contact screen again. Shit.");
      m.type = how;
    }
#endif
  strcpy(m.m.attach.envterm, attach_term);
  debug1("attach: sending %d bytes... ", sizeof m);

  strncpy(m.m.attach.auser, LoginName, sizeof(m.m.attach.auser) - 1); 
  m.m.attach.auser[sizeof(m.m.attach.auser) - 1] = 0;
  m.m.attach.apid = getpid();
  m.m.attach.adaptflag = adaptflag;
  m.m.attach.lines = m.m.attach.columns = 0;
  if ((s = getenv("LINES")))
    m.m.attach.lines = atoi(s);
  if ((s = getenv("COLUMNS")))
    m.m.attach.columns = atoi(s);

#ifdef PASSWORD
  if (how == MSG_ATTACH || how == MSG_CONT)
    trysend(lasts, &m, m.m.attach.password);
  else
#endif
    {
      if (write(lasts, (char *) &m, sizeof(m)) != sizeof(m))
	Panic(errno, "write");
      close(lasts);
    }
  debug1("Attach(%d): sent\n", m.type);
#ifdef MULTIUSER
  if (multi && (how == MSG_ATTACH || how == MSG_CONT))
    {
# ifndef PASSWORD
      pause();
# endif
# ifdef NOREUID
      close(multipipe[1]);
# else
      setreuid(real_uid, eff_uid);
      if (tty_oldmode >= 0)
        if (chmod(attach_tty, tty_oldmode))
          Panic(errno, "chmod %s", attach_tty);
      tty_oldmode = -1;
      setreuid(eff_uid, real_uid);
# endif
    }
#endif
  rflag = 0;
  return 1;
}


#ifdef PASSWORD

static trysendstatok, trysendstatfail;

static sig_t
trysendok(SIGDEFARG)
{
  trysendstatok = 1;
}

static sig_t
trysendfail(SIGDEFARG)
{
# ifdef SYSVSIGS
  signal(SIG_PW_FAIL, trysendfail);
# endif /* SYSVSIGS */
  trysendstatfail = 1;
}

static char screenpw[9];

static void
trysend(fd, m, pwto)
int fd;
struct msg *m;
char *pwto;
{
  char *npw = NULL;
  sig_t (*sighup)__P(SIGPROTOARG);
  sig_t (*sigusr1)__P(SIGPROTOARG);
  int tries;

  sigusr1 = signal(SIG_PW_OK, trysendok);
  sighup = signal(SIG_PW_FAIL, trysendfail);
  for (tries = 0; ; )
    {
      strcpy(pwto, screenpw);
      trysendstatok = trysendstatfail = 0;
      if (write(fd, (char *) m, sizeof(*m)) != sizeof(*m))
	Panic(errno, "write");
      close(fd);
      while (trysendstatok == 0 && trysendstatfail == 0)
	pause();
      if (trysendstatok)
	{
	  signal(SIG_PW_OK, sigusr1);
	  signal(SIG_PW_FAIL, sighup);
	  if (trysendstatfail)
	    sighup(SIGARG);
	  return;
	}
      if (++tries > 1 || (npw = getpass("Screen Password:")) == 0 || *npw == 0)
	{
#ifdef NETHACK
	  if (nethackflag)
	    Panic(0, "The guard slams the door in your face.");
	  else
#endif
	  Panic(0, "Password incorrect.");
	}
      strncpy(screenpw, npw, 8);
      if ((fd = MakeClientSocket(0, SockName)) == -1)
	Panic(0, "Cannot contact screen again. Shit.");
    }
}
#endif /* PASSWORD */


#ifdef DEBUG
static int AttacherPanic;

static sig_t
AttacherChld(SIGDEFARG)
{
  AttacherPanic=1;
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}
#endif

/*
 * the frontend's Interrupt handler
 * we forward SIGINT to the poor backend
 */
static sig_t 
AttacherSigInt(SIGDEFARG)
{
  signal(SIGINT, AttacherSigInt);
  Kill(MasterPid, SIGINT);
# ifndef SIGVOID
  return (sig_t) 0;
# endif
}

/*
 * Unfortunatelly this is also the SIGHUP handler, so we have to
 * check, if the backend is already detached.
 */

static sig_t
AttacherFinit(SIGDEFARG)
{
  struct stat statb;
  struct msg m;
  int s;

  debug("AttacherFinit();\n");
  signal(SIGHUP, SIG_IGN);
  /* Check if signal comes from backend */
  if (SockName)
    {
      strcpy(SockNamePtr, SockName);
      if (stat(SockPath, &statb) == 0 && (statb.st_mode & 0777) != 0600)
	{
	  debug("Detaching backend!\n");
	  bzero((char *) &m, sizeof(m));
	  strcpy(m.m_tty, attach_tty);
          debug1("attach_tty is %s\n", attach_tty);
	  m.m.detach.dpid = getpid();
	  m.type = MSG_HANGUP;
	  if ((s = MakeClientSocket(0, SockName)) >= 0)
	    {
	      write(s, (char *)&m, sizeof(m));
	      close(s);
	    }
	}
    }
#ifdef MULTIUSER
  if (tty_oldmode >= 0)
    {
      setuid(own_uid);
      chmod(attach_tty, tty_oldmode);
    }
#endif
  exit(0);
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

#ifdef POW_DETACH
static sig_t
AttacherFinitBye(SIGDEFARG)
{
  int ppid;
  debug("AttacherFintBye()\n");
#if defined(MULTIUSER) && defined(NOREUID)
  if (multiattach)
    exit(SIG_POWER_BYE);
#endif
#ifdef MULTIUSER
  setuid(own_uid);
#else
  setuid(real_uid);
#endif
  setgid(real_gid);
  /* we don't want to disturb init (even if we were root), eh? jw */
  if ((ppid = getppid()) > 1)
    Kill(ppid, SIGHUP);		/* carefully say good bye. jw. */
  exit(0);
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}
#endif

static int SuspendPlease;

static sig_t
SigStop(SIGDEFARG)
{
  debug("SigStop()\n");
  SuspendPlease = 1;
#ifndef SIGVOID
  return((sig_t) 0);
#endif
}

#ifdef LOCK
static int LockPlease;

static sig_t
DoLock(SIGDEFARG)
{
# ifdef SYSVSIGS
  signal(SIG_LOCK, DoLock);
# endif
  debug("DoLock()\n");
  LockPlease = 1;
# ifndef SIGVOID
  return((sig_t) 0);
# endif
}
#endif

#if defined(SIGWINCH) && defined(TIOCGWINSZ)
static int SigWinchPlease;

static sig_t
AttacherWinch(SIGDEFARG)
{
  debug("AttacherWinch()\n");
  SigWinchPlease = 1;
# ifndef SIGVOID
  return((sig_t) 0);
# endif
}
#endif


/*
 *  Attacher loop - no return
 */

void
Attacher()
{
  signal(SIGHUP, AttacherFinit);
  signal(SIG_BYE, AttacherFinit);
#ifdef POW_DETACH
  signal(SIG_POWER_BYE, AttacherFinitBye);
#endif
#ifdef LOCK
  signal(SIG_LOCK, DoLock);
#endif
  signal(SIGINT, AttacherSigInt);
#ifdef BSDJOBS
  signal(SIG_STOP, SigStop);
#endif
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
  signal(SIGWINCH, AttacherWinch);
#endif
#ifdef DEBUG
  signal(SIGCHLD, AttacherChld);
#endif
  debug("attacher: going for a nap.\n");
  dflag = 0;
  for (;;)
    {
      pause();
      debug("attacher: huh! a signal!\n");
#ifdef DEBUG
      if (AttacherPanic)
        {
	  fcntl(0, F_SETFL, 0);
	  SetTTY(0, &attach_Mode);
	  printf("\nSuddenly the Dungeon collapses!! - You die...\n");
	  eexit(1);
        }
#endif
#ifdef BSDJOBS
      if (SuspendPlease)
	{
	  SuspendPlease = 0;
#if defined(MULTIUSER) && defined(NOREUID)
	  if (multiattach)
	    exit(SIG_STOP);
#endif
	  signal(SIGTSTP, SIG_DFL);
	  debug("attacher: killing myself SIGTSTP\n");
	  kill(getpid(), SIGTSTP);
	  debug("attacher: continuing from stop\n");
	  signal(SIG_STOP, SigStop);
	  (void) Attach(MSG_CONT);
	}
#endif
#ifdef LOCK
      if (LockPlease)
	{
	  LockPlease = 0;
#if defined(MULTIUSER) && defined(NOREUID)
	  if (multiattach)
	    exit(SIG_LOCK);
#endif
	  LockTerminal();
# ifdef SYSVSIGS
	  signal(SIG_LOCK, DoLock);
# endif
	  (void) Attach(MSG_CONT);
	}
#endif	/* LOCK */
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
      if (SigWinchPlease)
	{
	  SigWinchPlease = 0;
# ifdef SYSVSIGS
	  signal(SIGWINCH, AttacherWinch);
# endif
	  (void) Attach(MSG_WINCH);
	}
#endif	/* SIGWINCH */
    }
}

#ifdef LOCK

/* ADDED by Rainer Pruy 10/15/87 */
/* POLISHED by mls. 03/10/91 */

static char LockEnd[] = "Welcome back to screen !!\n";

static sig_t
LockHup(SIGDEFARG)
{
  int ppid = getppid();
  setuid(real_uid);
  setgid(real_gid);
  if (ppid > 1)
    Kill(ppid, SIGHUP);
  exit(0);
}

static void
LockTerminal()
{
  char *prg;
  int sig, pid;
  sig_t (*sigs[NSIG])__P(SIGPROTOARG);

  for (sig = 1; sig < NSIG; sig++)
    {
      sigs[sig] = signal(sig, SIG_IGN);
    }
  signal(SIGHUP, LockHup);
  printf("\n");

  prg = getenv("LOCKPRG");
  if (prg && strcmp(prg, "builtin") && !access(prg, X_OK))
    {
      signal(SIGCHLD, SIG_DFL);
      debug1("lockterminal: '%s' seems executable, execl it!\n", prg);
      if ((pid = fork()) == 0)
        {
          /* Child */
          setuid(real_uid);	/* this should be done already */
          setgid(real_gid);
          closeallfiles(0);	/* important: /etc/shadow may be open */
          execl(prg, "SCREEN-LOCK", NULL);
          exit(errno);
        }
      if (pid == -1)
        {
#ifdef NETHACK
          if (nethackflag)
            Msg(errno, "Cannot fork terminal - lock failed");
          else
#endif
          Msg(errno, "Cannot lock terminal - fork failed");
        }
      else
        {
#ifdef BSDWAIT
          union wait wstat;
#else
          int wstat;
#endif
          int wret;

#ifdef hpux
          signal(SIGCHLD, SIG_DFL);
#endif
          errno = 0;
          while (((wret = wait(&wstat)) != pid) ||
	         ((wret == -1) && (errno == EINTR))
	         )
	    errno = 0;
    
          if (errno)
	    {
	      Msg(errno, "Lock");
	      sleep(2);
	    }
	  else if (WTERMSIG(wstat) != 0)
	    {
	      fprintf(stderr, "Lock: %s: Killed by signal: %d%s\n", prg,
		      WTERMSIG(wstat), WIFCORESIG(wstat) ? " (Core dumped)" : "");
	      sleep(2);
	    }
	  else if (WEXITSTATUS(wstat))
	    {
	      debug2("Lock: %s: return code %d\n", prg, WEXITSTATUS(wstat));
	    }
          else
	    printf(LockEnd);
        }
    }
  else
    {
      if (prg)
	{
          debug1("lockterminal: '%s' seems NOT executable, we use our builtin\n", prg);
	}
      else
	{
	  debug("lockterminal: using buitin.\n");
	}
      screen_builtin_lck();
    }
  /* reset signals */
  for (sig = 1; sig < NSIG; sig++)
    {
      if (sigs[sig] != (sig_t(*)__P(SIGPROTOARG)) -1)
	signal(sig, sigs[sig]);
    }
}				/* LockTerminal */

/* -- original copyright by Luigi Cannelloni 1985 (luigi@faui70.UUCP) -- */
static void
screen_builtin_lck()
{
  char fullname[100], *cp1, message[BUFSIZ];
  char c, *pass, mypass[9];
#ifdef SHADOWPW
  struct spwd *sss = NULL;
#endif
  int t;

#ifdef undef
  /* get password entry */
  if ((ppp = getpwuid(real_uid)) == NULL)
    {
      fprintf(stderr, "screen_builtin_lck: No passwd entry.\007\n");
      sleep(2);
      return;
    }
  if (!isatty(0))
    {
      fprintf(stderr, "screen_builtin_lck: Not a tty.\007\n");
      sleep(2);
      return;
    }
#endif
  pass = ppp->pw_passwd;
#ifdef SHADOWPW
realpw:
#endif /* SHADOWPW */
  for (t = 0; t < 13; t++)
    {
      c = pass[t];
      if (!(c == '.' || c == '/' ||
            (c >= '0' && c <= '9') || 
            (c >= 'a' && c <= 'z') || 
            (c >= 'A' && c <= 'Z'))) 
        break;
    }
  if (t < 13)
    {
      debug("builtin_lock: ppp->pw_passwd bad, has it a shadow?\n");
#ifdef SHADOWPW
      setspent(); /* rewind shadow file */
      if ((sss == NULL) && (sss = getspnam(ppp->pw_name)))
        {
          pass = sss->sp_pwdp;
          goto realpw;
        }
#endif /* SHADOWPW */
      if ((pass = getpass("Key:   ")))
        {
          strncpy(mypass, pass, 8);
          mypass[8] = 0;
          if (*mypass == 0)
            return;
          if ((pass = getpass("Again: ")))
            {
              if (strcmp(mypass, pass))
                {
                  fprintf(stderr, "Passwords don't match.\007\n");
                  sleep(2);
                  return;
                }
            }
        }
      if (pass == 0)
        {
          fprintf(stderr, "Getpass error.\007\n");
          sleep(2);
          return;
        }
      pass = 0;
    }

  debug("screen_builtin_lck looking in gcos field\n");
  strcpy(fullname, ppp->pw_gecos);
  if ((cp1 = index(fullname, ',')) != NULL)
    *cp1 = '\0';
  if ((cp1 = index(fullname, '&')) != NULL)
    {
      sprintf(cp1, "%s", ppp->pw_name);
      if (*cp1 >= 'a' && *cp1 <= 'z')
	*cp1 -= 'a' - 'A';
    }

  sprintf(message, "Screen used by %s <%s>.\nPassword:\007",
          fullname, ppp->pw_name);

  /* loop here to wait for correct password */
  for (;;)
    {
      debug("screen_builtin_lck awaiting password\n");
      errno = 0;
      if ((cp1 = getpass(message)) == NULL)
        {
          AttacherFinit(SIGARG);
          /* NOTREACHED */
        }
      debug3("getpass(%d): %x == %s\n", errno, (unsigned int)cp1, cp1);
      if (pass)
        {
          if (!strcmp(crypt(cp1, pass), pass))
            break;
        }
      else
        {
          if (!strcmp(cp1, mypass))
            break;
        }
      debug("screen_builtin_lck: NO!!!!!\n");
    }
  debug("password ok.\n");
}

#endif	/* LOCK */
