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
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <sys/types.h>
#include <signal.h>

#include "config.h"
#include "screen.h"
#include "extern.h"

#ifdef SVR4
# include <sys/resource.h>
#endif

extern struct display *display;
extern int eff_uid, real_uid;
extern int eff_gid, real_gid;
extern struct mline mline_old;
extern char *null, *blank;

char *
SaveStr(str)
register const char *str;
{
  register char *cp;

  if ((cp = malloc(strlen(str) + 1)) == NULL)
    Panic(0, strnomem);
  else
    strcpy(cp, str);
  return cp;
}

#ifndef HAVE_STRERROR
char *
strerror(err)
int err;
{
  extern int sys_nerr;
  extern char *sys_errlist[];

  static char er[20];
  if (err > 0 && err < sys_nerr)
    return sys_errlist[err];
  sprintf(er, "Error %d", err);
  return er;
}
#endif

void
centerline(str)
char *str;
{
  int l, n;

  ASSERT(display);
  n = strlen(str);
  if (n > D_width - 1)
    n = D_width - 1;
  l = (D_width - 1 - n) / 2;
  if (l > 0)
    AddStrn("", l);
  AddStrn(str, n);
  AddStr("\r\n");
}

char *
Filename(s)
char *s;
{
  register char *p = s;

  if (p)
    while (*p)
      if (*p++ == '/')
        s = p;
  return s;
}

char *
stripdev(nam)
char *nam;
{
#ifdef apollo
  char *p;
  
  if (nam == NULL)
    return NULL;
  if (p = strstr(nam,"/dev/"))
    return p + 5;
#else /* apollo */
  if (nam == NULL)
    return NULL;
# ifdef SVR4
  /* unixware has /dev/pts012 as synonym for /dev/pts/12 */
  if (!strncmp(nam, "/dev/pts", 8) && nam[8] >= '0' && nam[8] <= '9')
    {
      static char b[13];
      sprintf(b, "pts/%d", atoi(nam + 8));
      return b;
    }
# endif /* SVR4 */
  if (strncmp(nam, "/dev/", 5) == 0)
    return nam + 5;
#endif /* apollo */
  return nam;
}


/*
 *    Signal handling
 */

#ifdef POSIX
sigret_t (*xsignal(sig, func)) __P(SIGPROTOARG)
int sig;
sigret_t (*func) __P(SIGPROTOARG);
{
  struct sigaction osa, sa;
  sa.sa_handler = func;
  (void)sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(sig, &sa, &osa))
    return (sigret_t (*)__P(SIGPROTOARG))-1;
  return osa.sa_handler;
}

#else
# ifdef hpux
/*
 * hpux has berkeley signal semantics if we use sigvector,
 * but not, if we use signal, so we define our own signal() routine.
 */
void (*xsignal(sig, func)) __P(SIGPROTOARG)
int sig;
void (*func) __P(SIGPROTOARG);
{
  struct sigvec osv, sv;

  sv.sv_handler = func;
  sv.sv_mask = sigmask(sig);
  sv.sv_flags = SV_BSDSIG;
  if (sigvector(sig, &sv, &osv) < 0)
    return (void (*)__P(SIGPROTOARG))(BADSIG);
  return osv.sv_handler;
}
# endif	/* hpux */
#endif	/* POSIX */


/*
 *    uid/gid handling
 */

#ifdef HAVE_SETEUID

void
xseteuid(euid)
int euid;
{
  if (seteuid(euid) == 0)
    return;
  seteuid(0);
  if (seteuid(euid))
    Panic(errno, "seteuid");
}

void
xsetegid(egid)
int egid;
{
  if (setegid(egid))
    Panic(errno, "setegid");
}

#else
# ifdef HAVE_SETREUID

void
xseteuid(euid)
int euid;
{
  int oeuid;

  oeuid = geteuid();
  if (oeuid == euid)
    return;
  if (getuid() != euid)
    oeuid = getuid();
  if (setreuid(oeuid, euid))
    Panic(errno, "setreuid");
}

void
xsetegid(egid)
int egid;
{
  int oegid;

  oegid = getegid();
  if (oegid == egid)
    return;
  if (getgid() != egid)
    oegid = getgid();
  if (setregid(oegid, egid))
    Panic(errno, "setregid");
}

# endif
#endif



#ifdef NEED_OWN_BCOPY
void
xbcopy(s1, s2, len)
register char *s1, *s2;
register int len;
{
  if (s1 < s2 && s2 < s1 + len)
    {
      s1 += len;
      s2 += len;
      while (len-- > 0)
	*--s2 = *--s1;
    }
  else
    while (len-- > 0)
      *s2++ = *s1++;
}
#endif	/* NEED_OWN_BCOPY */

void
bclear(p, n)
char *p;
int n;
{
  bcopy(blank, p, n);
}


void
Kill(pid, sig)
int pid, sig;
{
  if (pid < 2)
    return;
  (void) kill(pid, sig);
}

void
closeallfiles(except)
int except;
{
  int f;
#ifdef SVR4
  struct rlimit rl;
  
  if ((getrlimit(RLIMIT_NOFILE, &rl) == 0) && rl.rlim_max != RLIM_INFINITY)
    f = rl.rlim_max;
  else
#endif /* SVR4 */
#if defined(SYSV) && !defined(ISC)
  f = NOFILE;
#else /* SYSV && !ISC */
  f = getdtablesize();
#endif /* SYSV && !ISC */
  while (--f > 2)
    if (f != except)
      close(f);
}



/*
 *  Security - switch to real uid
 */

#ifndef USE_SETEUID
static int UserPID;
static sigret_t (*Usersigcld)__P(SIGPROTOARG);
#endif
static int UserSTAT;

int
UserContext()
{
#ifndef USE_SETEUID
  if (eff_uid == real_uid && eff_gid == real_gid)
    return 1;
  Usersigcld = signal(SIGCHLD, SIG_DFL);
  debug("UserContext: forking.\n");
  switch (UserPID = fork())
    {
    case -1:
      Msg(errno, "fork");
      return -1;
    case 0:
      signal(SIGHUP, SIG_DFL);
      signal(SIGINT, SIG_IGN);
      signal(SIGQUIT, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
# ifdef BSDJOBS
      signal(SIGTTIN, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);
# endif
      setuid(real_uid);
      setgid(real_gid);
      return 1;
    default:
      return 0;
    }
#else
  xseteuid(real_uid);
  xsetegid(real_gid);
  return 1;
#endif
}

void
UserReturn(val)
int val;
{
#ifndef USE_SETEUID
  if (eff_uid == real_uid && eff_gid == real_gid)
    UserSTAT = val;
  else
    _exit(val);
#else
  xseteuid(eff_uid);
  xsetegid(eff_gid);
  UserSTAT = val;
#endif
}

int
UserStatus()
{
#ifndef USE_SETEUID
  int i;
# ifdef BSDWAIT
  union wait wstat;
# else
  int wstat;
# endif

  if (eff_uid == real_uid && eff_gid == real_gid)
    return UserSTAT;
  if (UserPID < 0)
    return -1;
  while ((errno = 0, i = wait(&wstat)) != UserPID)
    if (i < 0 && errno != EINTR)
      break;
  (void) signal(SIGCHLD, Usersigcld);
  if (i == -1)
    return -1;
  return WEXITSTATUS(wstat);
#else
  return UserSTAT;
#endif
}

#ifdef NEED_RENAME
int
rename (old, new)
char *old;
char *new;
{
  if (link(old, new) < 0)
    return -1;
  return unlink(old);
}
#endif


int
AddXChar(buf, ch)
char *buf;
int ch;
{
  char *p = buf;

  if (ch < ' ' || ch == 0x7f)
    {
      *p++ = '^';
      *p++ = ch ^ 0x40;
    }
  else if (ch >= 0x80)
    {
      *p++ = '\\';
      *p++ = (ch >> 6 & 7) + '0';
      *p++ = (ch >> 3 & 7) + '0';
      *p++ = (ch >> 0 & 7) + '0';
    }
  else
    *p++ = ch;
  return p - buf;
}

int
AddXChars(buf, len, str)
char *buf, *str;
int len;
{
  char *p;

  len -= 4;     /* longest sequence produced by AddXChar() */
  for (p = buf; p < buf + len && *str; str++)
    {
      if (*str == ' ')
        *p++ = *str;
      else
        p += AddXChar(p, *str);
    }
  *p = 0;
  return p - buf;
}


#ifdef TERMINFO
/*
 * This is a replacement for the buggy _delay function from the termcap
 * emulation of libcurses, which ignores ospeed.
 */
int
_delay(delay, outc)
register int delay;
int (*outc)();
{
  int pad;
  extern short ospeed;
  static short osp2pad[] = {
    0,2000,1333,909,743,666,500,333,166,83,55,41,20,10,5,2,1,1
  };

  if (ospeed <= 0 || ospeed >= sizeof(osp2pad)/sizeof(*osp2pad))
    return 0;
  pad =osp2pad[ospeed];
  delay = (delay + pad / 2) / pad;
  while (delay-- > 0)
    (*outc)(0);
  return 0;
}
#endif

