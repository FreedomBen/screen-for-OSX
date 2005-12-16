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
 *	Larry W. Virden (lwv27%cas.BITNET@CUNYVM.CUNY.Edu)
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
#include <sys/param.h>
#include <signal.h>
#include "config.h"
#include "screen.h"
#include "extern.h"

extern char *blank;
extern struct display *display;
extern int eff_uid, real_uid;
extern int eff_gid, real_gid;

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

void
centerline(str)
char *str;
{
  int l, n;

  n = strlen(str);
  if (n > d_width - 1)
    n = d_width - 1;
  l = (d_width - 1 - n) / 2;
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
  if (strncmp(nam, "/dev/", 5) == 0)
    return nam + 5;
#endif /* apollo */
  return nam;
}

#ifdef hpux
/*
 * hpux has berkeley signal semantics if we use sigvector,
 * but not, if we use signal, so we define our own signal() routine.
 * (jw)
 */
void (*signal(sig, func)) ()
int sig;
void (*func) ();
{
  struct sigvec osv, sv;

  sv.sv_handler = func;
  sv.sv_mask = sigmask(sig);
  sv.sv_flags = SV_BSDSIG;
  if (sigvector(sig, &sv, &osv) < 0)
    return (BADSIG);
  return (osv.sv_handler);
}
#endif	/* hpux */

#ifndef USEBCOPY
void
#ifdef linux
bcopy(ss1, ss2, len)
register const void *ss1;
register void *ss2;
#else
bcopy(s1, s2, len)
register char *s1, *s2;
#endif

register int len;
{
#ifdef linux
  register char *s1 = (char *)ss1;
  register char *s2 = (char *)ss2;
#endif
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
#endif	/* USEBCOPY */

void
bclear(p, n)
char *p;
int n;
{
  bcopy(blank, p, n);
}


void
closeallfiles(except)
int except;
{
  int f;
#ifdef SVR4
  int getrlimit __P((int, struct rlimit *));
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


#ifdef NOREUID
static int UserPID;
static sig_t (*Usersigcld)__P(SIGPROTOARG);
#endif
static int UserSTAT;

int
UserContext()
{
#ifdef NOREUID
  if (eff_uid == real_uid)
    return(1);
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
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
  return 1;
#endif
}

void
UserReturn(val)
int val;
{
#if defined(NOREUID)
  if (eff_uid == real_uid)
    UserSTAT = val;
  else
    exit(val);
#else
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
  UserSTAT = val;
#endif
}

int
UserStatus()
{
#ifdef NOREUID
  int i;
# ifdef BSDWAIT
  union wait wstat;
# else
  int wstat;
# endif

  if (eff_uid == real_uid)
    return UserSTAT;
  if (UserPID < 0)
    return -1;
  while ((errno = 0, i = wait(&wstat)) != UserPID)
    if (i < 0 && errno != EINTR)
      break;
  (void) signal(SIGCHLD, Usersigcld);
  if (i == -1)
    return -1;
  return (WEXITSTATUS(wstat));
#else
  return UserSTAT;
#endif
}
