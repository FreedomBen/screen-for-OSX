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
 * $Id$ FAU
 */

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>

#if defined(BSDI) || defined(__386BSD__) || defined(_CX_UX)
# include <signal.h>
#endif /* BSDI || __386BSD__ || _CX_UX */

#ifdef ISC
# ifdef ENAMETOOLONG
#  undef ENAMETOOLONG
# endif
# ifdef ENOTEMPTY
#  undef ENOTEMPTY
# endif
# include <net/errno.h>
#endif

#ifdef hpux
# ifndef GETUTENT
#  define GETUTENT 1
# endif 
#endif

#ifndef linux /* all done in <errno.h> */
extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];
#endif /* linux */

#ifdef sun
# define getpgrp __getpgrp
# define exit __exit
#endif

#ifdef POSIX
# include <unistd.h>
# if defined(__STDC__)
#  include <stdlib.h>
# endif /* __STDC__ */
#endif /* POSIX */

#ifdef sun
# undef getpgrp
# undef exit
#endif /* sun */

#ifdef POSIX
# include <termios.h>
# ifdef hpux
#  include <bsdtty.h>
# endif /* hpux */
# ifdef NCCS
#  define MAXCC NCCS
# else
#  define MAXCC 256
# endif
#else /* POSIX */
# ifdef TERMIO
#  include <termio.h>
#  ifdef NCC
#   define MAXCC NCC
#  else
#   define MAXCC 256
#  endif
# else /* TERMIO */
#  include <sgtty.h>
# endif /* TERMIO */
#endif /* POSIX */

#ifndef SYSV
# ifdef NEWSOS
#  define strlen ___strlen___
#  include <strings.h>
#  undef strlen
# else /* NEWSOS */
#  include <strings.h>
# endif /* NEWSOS */
#else /* BSD */
# if defined(SVR4) || defined(NEWSOS)
#  define strlen ___strlen___
#  include <string.h>
#  undef strlen
#  ifndef NEWSOS
    extern size_t strlen(const char *);
#  endif /* NEWSOS */
# else /* SVR4 */
#  include <string.h>
# endif /* SVR4 */
#endif /* BSD */

#ifdef USEVARARGS
# if defined(__STDC__)
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
#endif

#if !defined(sun) && !defined(B43) && !defined(ISC) && !defined(pyr) && !defined(_CX_UX)
# include <time.h>
#endif
#include <sys/time.h>

#if (defined(TIOCGWINSZ) || defined(TIOCSWINSZ)) && defined(M_UNIX)
# include <sys/stream.h>
# include <sys/ptem.h>
#endif

#ifdef UTMPOK
# ifdef SVR4
#  include <utmpx.h>
#  define UTMPFILE	UTMPX_FILE
#  define utmp		utmpx
#  define getutent	getutxent
#  define getutid	getutxid
#  define getutline	getutxline
#  define pututline	pututxline
#  define setutent	setutxent
#  define endutent	endutxent
#  define ut_time	ut_xtime
# else /* SVR4 */
#  include <utmp.h>
# endif /* SVR4 */
# if defined(apollo) || defined(linux)
   /* 
    * We don't have GETUTENT, so we dig into utmp ourselves.
    * But we save the permanent filedescriptor and
    * open utmp just when we need to. 
    * This code supports an unsorted utmp. jw.
    */
#  define UTNOKEEP
# endif /* apollo || linux */
#endif

#ifndef UTMPFILE
# ifdef UTMP_FILE
#  define UTMPFILE	UTMP_FILE
# else
#  ifdef _PATH_UTMP
#   define UTMPFILE	_PATH_UTMP
#  else
#   define UTMPFILE	"/etc/utmp"
#  endif /* _PATH_UTMP */
# endif
#endif

#ifndef UTMPOK
#  ifdef USRLIMIT
#	 undef USRLIMIT
#  endif
#endif

#ifdef LOGOUTOK
# ifndef LOGINDEFAULT
#  define LOGINDEFAULT 0
# endif
#else
# undef LOGINDEFAULT
# define LOGINDEFAULT 1
#endif

#ifndef F_OK
#define F_OK 0
#endif
#ifndef X_OK
#define X_OK 1
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif

#ifndef S_IFIFO
#define S_IFIFO  0010000
#endif
#ifndef S_IREAD
#define S_IREAD  0000400
#endif
#ifndef S_IWRITE
#define S_IWRITE 0000200
#endif
#ifndef S_IEXEC
#define S_IEXEC  0000100
#endif

#if defined(S_IFIFO) && defined(S_IFMT) && !defined(S_ISFIFO)
#define S_ISFIFO(mode) ((mode & S_IFMT) == S_IFIFO)
#endif
#if defined(S_IFSOCK) && defined(S_IFMT) && !defined(S_ISSOCK)
#define S_ISSOCK(mode) ((mode & S_IFMT) == S_IFSOCK)
#endif

#ifndef TERMCAP_BUFSIZE
# define TERMCAP_BUFSIZE 1024
#endif

#ifndef MAXPATHLEN
# define MAXPATHLEN 1024
#endif

#ifndef SIG_T_DEFINED
# ifdef SIGVOID
#  if defined(ultrix)
#   define sig_t void
#  else /* nice compilers: */
    typedef void sig_t;
#  endif
# else
   typedef int sig_t; /* (* sig_t) */
# endif
#else
# if defined (__alpha)
#   define sig_t void	/* From: Dietrich Wiegandt <dietrich@afsw01.cern.ch> */
#  endif
#endif /* SIG_T_DEFINED */

#if defined(SVR4) || (defined(SYSV) && defined(ISC)) || defined(_AIX) || defined(linux) || defined(ultrix) || defined(__386BSD__) || defined(BSDI)
# define SIGPROTOARG   (int)
# define SIGDEFARG     int sigsig
# define SIGARG        0
#else
# define SIGPROTOARG   (void)
# define SIGDEFARG
# define SIGARG
#endif

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif

#ifdef USESIGSET
# define signal sigset
#endif /* USESIGSET */

#ifndef PID_T_DEFINED
typedef int pid_t;
#endif

#ifndef UID_T_DEFINED
typedef int gid_t;
typedef int uid_t;
#endif

#ifdef GETUTENT
  typedef char *slot_t;
#else
  typedef int slot_t;
#endif

#ifdef SYSV
# define index strchr
# define rindex strrchr
# define bzero(poi,len) memset(poi,0,len)
# define bcmp memcmp
# define killpg(pgrp,sig) kill( -(pgrp), sig)
#endif /* SYSV */

#ifndef USEBCOPY
# ifdef USEMEMCPY
#  define bcopy(s,d,len) memcpy(d,s,len)
# else
#  ifdef USEMEMMOVE
#   define bcopy(s,d,len) memmove(d,s,len)
#  else
#   define NEED_OWN_BCOPY
#  endif
# endif
#endif

#if defined(_POSIX_SOURCE) && defined(ISC)
# ifndef O_NDELAY
#  define O_NDELAY O_NONBLOCK
# endif
#endif

#ifdef hpux
# define setreuid(ruid, euid) setresuid(ruid, euid, -1)
# define setregid(rgid, egid) setresgid(rgid, egid, -1)
#endif

#if (!defined(sysV68) && !defined(M_XENIX)) || defined(NeXT)
# include <sys/wait.h>
#endif

#ifndef WTERMSIG
# ifndef BSDWAIT /* if wait is NOT a union: */
#  define WTERMSIG(status) (status & 0177)
# else
#  define WTERMSIG(status) status.w_T.w_Termsig 
# endif
#endif

#ifndef WSTOPSIG
# ifndef BSDWAIT /* if wait is NOT a union: */
#  define WSTOPSIG(status) ((status >> 8) & 0377)
# else
#  define WSTOPSIG(status) status.w_S.w_Stopsig 
# endif
#endif

/* NET-2 uses WCOREDUMP */
#if defined(WCOREDUMP) && !defined(WIFCORESIG)
# define WIFCORESIG(status) WCOREDUMP(status)
#endif

#ifndef WIFCORESIG
# ifndef BSDWAIT /* if wait is NOT a union: */
#  define WIFCORESIG(status) (status & 0200)
# else
#  define WIFCORESIG(status) status.w_T.w_Coredump
# endif
#endif

#ifndef WEXITSTATUS
# ifndef BSDWAIT /* if wait is NOT a union: */
#  define WEXITSTATUS(status) ((status >> 8) & 0377)
# else
#  define WEXITSTATUS(status) status.w_T.w_Retcode
# endif
#endif

#if defined(M_XENIX) || defined(M_UNIX) || defined(_SEQUENT_)
#include <sys/select.h> /* for timeval + FD... */
#endif


/*
 * SunOS 3.5 - Tom Schmidt - Micron Semiconductor, Inc - 27-Jul-93
 * tschmidt@vax.micron.com
 */
#ifndef FD_SET
# ifndef SUNOS3
typedef struct fd_set
{
  int fds_bits[1];
}      fd_set;
# endif
# define FD_ZERO(fd) ((fd)->fds_bits[0] = 0)
# define FD_SET(b, fd) ((fd)->fds_bits[0] |= 1 << (b))
# define FD_ISSET(b, fd) ((fd)->fds_bits[0] & 1 << (b))
# define FD_SETSIZE 32
#endif


#if defined(sgi) 
/* on IRIX, regardless of the stream head's read mode (RNORM/RMSGN/RMSGD)
 * TIOCPKT mode causes data loss if our buffer is too small (IOSIZE)
 * to hold the whole packet at first read().
 * (Marc Boucher)
 */
# undef TIOCPKT
#endif

#if !defined(VDISABLE)
# ifdef _POSIX_VDISABLE
#  define VDISABLE _POSIX_VDISABLE
# else
#  define VDISABLE 0377
# endif /* _POSIX_VDISABLE */
#endif /* !VDISABLE */

#if !defined(FNDELAY) && defined(O_NDELAY)
# define FNDELAY O_NDELAY
#endif

/*typedef long off_t; */	/* Someone might need this */


/* 
 * 4 <= IOSIZE <=1000
 * you may try to vary this value. Use low values if your (VMS) system
 * tends to choke when pasting. Use high values if you want to test
 * how many characters your pty's can buffer.
 */
#define IOSIZE		4096

/* used in screen.c and attacher.c */
#if !defined(NSIG)	/* kbeal needs these w/o SYSV */
# define NSIG 32
#endif /* !NSIG */

