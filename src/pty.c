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

#include "config.h"
#include "screen.h"

#ifndef sun
#include <sys/ioctl.h>
#endif

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#include "extern.h"

/*
 * if no PTYRANGE[01] is in the config file, we pick a default
 */
#ifndef PTYRANGE0
# define PTYRANGE0 "qpr"
#endif
#ifndef PTYRANGE1
# define PTYRANGE1 "0123456789abcdef"
#endif

extern int eff_uid;

/* used for opening a new pty-pair: */
static char PtyName[32], TtyName[32];

#if !(defined(sequent) || defined(_SEQUENT_) || defined(SVR4))
# ifdef hpux
static char PtyProto[] = "/dev/ptym/ptyXY";
static char TtyProto[] = "/dev/pty/ttyXY";
# else
static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";
# endif /* hpux */
#endif

#if defined(sequent) || defined(_SEQUENT_) || defined(SVR4)

int
OpenPTY(ttyn)
char **ttyn;
{
  char *m, *s;
  register int f;
# ifdef SVR4
  char *ptsname();
  int unlockpt __P((int)), grantpt __P((int));
  sig_t (*sigcld)__P(SIGPROTOARG);

  if ((f = open("/dev/ptmx", O_RDWR)) == -1)
    return(-1);

  /*
   * SIGCHLD set to SIG_DFL for grantpt() because it fork()s and
   * exec()s pt_chmod
   */
  sigcld = signal(SIGCHLD, SIG_DFL);
       
  if ((m = ptsname(f)) == NULL || unlockpt(f) || grantpt(f))
    {
      signal(SIGCHLD, sigcld);
      close(f);
      return(-1);
    } 
  signal(SIGCHLD, sigcld);
  strncpy(TtyName, m, sizeof(TtyName));
# else /* SVR4 */
  if ((f = getpseudotty(&s, &m)) < 0)
    return(-1);
#ifdef _SEQUENT_
  fvhangup(s);
#endif
  strncpy(PtyName, m, sizeof(PtyName));
  strncpy(TtyName, s, sizeof(TtyName));
# endif /* SVR4 */
# ifdef POSIX
  tcflush(f, TCIOFLUSH);
# else
#  ifdef TIOCFLUSH
  (void) ioctl(f, TIOCFLUSH, (char *) 0);
#  endif
# endif
# ifdef LOCKPTY
  (void) ioctl(f, TIOCEXCL, (char *) 0);
# endif
  *ttyn = TtyName;
  return (f);
}

#else /* defined(sequent) || defined(_SEQUENT_) || defined(SVR4) */

# ifdef MIPS

int
OpenPTY(ttyn)
char **ttyn;
{
  register char *p, *l, *d;
  register f, tf;
  register my_minor;
  struct stat buf;
   
  strcpy(PtyName, PtyProto);
  for (p = PtyName; *p != 'X'; ++p)
    ;
  for (l = PTYRANGE0; *p = *l; ++l)
    {
      for (d = PTYRANGE1; p[1] = *d; ++d)
	{
	  if ((f = open(PtyName, O_RDWR)) != -1)
	    {
	      fstat(f, &buf);
	      my_minor = minor(buf.st_rdev);
	      sprintf(TtyName, "/dev/ttyq%d", my_minor);
	      if (eff_uid && access(TtyName, R_OK | W_OK))
		{
		  close(f);
		  continue;
		}
#ifdef LOCKPTY
	      (void) ioctl(f, TIOCEXCL, (char *)0);
#endif
	      *ttyn = TtyName;
	      return f;
	    }
	}
    }
  return -1;
}

# else  /* MIPS */

#  ifdef sgi
#   ifdef __sgi /* __sgi -> IRIX 4.0 */

int
OpenPTY(ttyn)
char **ttyn;
{
  int fd;
  char *name; 
  sig_t (*sigcld)__P(SIGPROTOARG);

  /*
   * SIGCHLD set to SIG_DFL for _getpty() because it may fork() and
   * exec() /usr/adm/mkpts
   */
  sigcld = signal(SIGCHLD, SIG_DFL);
  name = _getpty(&fd, O_RDWR | O_NDELAY, 0600, 0);
  signal(SIGCHLD, sigcld);

  if (name == NULL)
    return -1;
  *ttyn = name;
  return fd;
}

#   else /* __sgi */

int
OpenPTY(ttyn)
char **ttyn;
{
  register f;
  register my_minor;
  struct stat buf;
   
  strcpy(PtyName, "/dev/ptc");
  f = open(PtyName, O_RDWR | O_NDELAY);
  if (f >= 0)
    {
      if (fstat(f, &buf) < 0)
	{
	  close(f);
	  return -1;
	}
      my_minor = minor(buf.st_rdev);
      sprintf(TtyName, "/dev/ttyq%d", my_minor);
    }
  *ttyn = TtyName;
  return f;
}

#   endif /* __sgi */
#  else /* sgi */

#   ifdef _AIX /* RS6000 */

int
OpenPTY(ttyn)
char **ttyn;
{
  register int i, f, tf;

  /* a dumb looking loop replaced by mycrofts code: */
  strcpy (PtyName, "/dev/ptc");
  if ((f = open (PtyName, O_RDWR)) != -1)
    {
      strcpy (TtyName, ttyname (f));
      strcpy (PtyName, TtyName);
      PtyName [7] = 'c'; 
      if (eff_uid && access(TtyName, R_OK | W_OK))
	{
	  close(f);
	  continue;
	}
#ifdef LOCKPTY
      if (ioctl (f, TIOCEXCL, (char *) 0) == -1)
	return -1;
#endif /* LOCKPTY */
      *ttyn = TtyName;
      return f;
    }
  return -1;
}

#   else /* _AIX, RS6000 */

int
OpenPTY(ttyn)
char **ttyn;
{
  register char *p, *q, *l, *d;
  register int f;

#    if !defined(hpux) && !defined(sun) && !defined(m68k) && !defined(m88k)
  debug("Hello, You are none of: sequent, _SEQUENT_, SVR4, MIPS, sgi, AIX\n");
  debug("       This OpenPTY() is for hpux, m68k and sun ...\n");
  debug("       ... and for you?\n");
#    endif
  strcpy(PtyName, PtyProto);
  strcpy(TtyName, TtyProto);
  for (p = PtyName; *p != 'X'; ++p)
    ;
  for (q = TtyName; *q != 'X'; ++q)
    ;
  for (l = PTYRANGE0; (*p = *l) != '\0'; ++l)
    {
      for (d = PTYRANGE1; (p[1] = *d) != '\0'; ++d)
	{
	  debug1("OpenPTY tries '%s'\n", PtyName);
	  if ((f = open(PtyName, O_RDWR)) == -1)
	    continue;
	  q[0] = *l;
	  q[1] = *d;
	  if (eff_uid && access(TtyName, R_OK | W_OK))
	    {
	      close(f);
	      continue;
	    }
#ifdef sun
	  /* Hack to ensure that the slave side of the pty is
	   * unused. May not work in anything d_other than SunOS4.1
	   */
	    {
	      int pgrp;

	      /* tcgetpgrp does not work (uses TIOCGETPGRP)! */
	      if (ioctl(f, TIOCGPGRP, &pgrp) != -1 || errno != EIO)
		{
		  close(f);
		  continue;
		}
	    }
#endif
#ifdef LOCKPTY
	  (void) ioctl(f, TIOCEXCL, (char *) 0);
#endif
	  *ttyn = TtyName;
	  return f;
	}
    }
  return -1;
}

#   endif /* _AIX, RS6000 */
#  endif /* sgi */
# endif /* MIPS */
#endif
