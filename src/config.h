/* config.h.  Generated automatically by configure.  */
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
 * $Id$ FAU
 */





/**********************************************************************
 *
 *	User Configuration Section
 */



/*
 * By default screen will create a directory named ".screen" in the
 * user's HOME directory to contain the named sockets.  If this causes
 * you problems (e.g. some user's HOME directories are NFS-mounted and
 * don't support named sockets) you can have screen create the socket
 * directories in a common subdirectory, such as /tmp or /usr/tmp.  It
 * makes things a little more secure if you choose a directory where
 * the "sticky" bit is on, but this isn't required.  Screen will name
 * the subdirectories "S-$USER" (e.g /tmp/S-davison).
 * Do not define TMPTEST unless it's for debugging purpose.
 * If you want to have your socket directory in "/tmp/screens" then
 * define LOCALSOCKDIR and change the definition of SOCKDIR below.
 */
#undef HOMESOCKDIR

#ifndef HOMESOCKDIR
# ifndef TMPTEST
#  define SOCKDIR "/usr/local/screens"
# else
#  define SOCKDIR "/tmp/testscreens"
# endif
#endif

/*
 * Screen sources two startup files. First a global file with a path
 * specified here, second your local $HOME/.screenrc
 * Don't define this, if you don't want it.
 */
#define ETCSCREENRC "/usr/local/etc/screenrc"

/* 
 * define PTYMODE if you do not like the default of 0622, which allows 
 * public write to your pty.
 * define PTYGROUP if you do not want the tty to be in "your" group.
 * Note, screen is unable to change mode or group of the pty if it
 * is not installed with sufficient privilege. (e.g. set-uid-root)
 */
#undef PTYMODE
#undef PTYGROUP

/*
 * If screen is NOT installed set-uid root, screen can provide tty
 * security by exclusively locking the ptys.  While this keeps other
 * users from opening your ptys, it also keeps your own subprocesses
 * from being able to open /dev/tty.  Define LOCKPTY to add this
 * exclusive locking.
 */
#undef LOCKPTY

/*
 * If you'd rather see the status line on the first line of your
 * terminal rather than the last, define TOPSTAT.
 */
#undef TOPSTAT

/*
 * here come the erlangen extensions to screen:
 * define LOCK if you want to use a lock program for a screenlock.
 * define PASSWORD for secure reattach of your screen.
 * define COPY_PASTE to use the famous hacker's treasure zoo.
 * define POW_DETACH to have a detach_and_logout key.
 * define REMOTE_DETACH (-d option) to move screen between terminals.
 * define AUTO_NUKE to enable Tim MacKenzies clear screen nuking
 * define MULTI to allow multiple attaches.
 * define MULTIUSER to allow other users attach to your session
 *                  (if they are in the acl, of course)
 * (jw)
 */
#undef SIMPLESCREEN
#ifndef SIMPLESCREEN
# define LOCK
# define PASSWORD
# define COPY_PASTE
# define REMOTE_DETACH
# define POW_DETACH
# define AUTO_NUKE
# define MULTI
# define MULTIUSER
#endif /* SIMPLESCREEN */

/*
 * As error messages are mostly meaningless to the user, we
 * try to throw out phrases that are somewhat more familiar
 * to ...well, at least familiar to us NetHack players.
 */
#ifndef NONETHACK
# define NETHACK
#endif /* NONETHACK */

/*
 * If screen is installed with permissions to update /etc/utmp (such
 * as if it is installed set-uid root), define UTMPOK.
 */
#define UTMPOK

/* Set LOGINDEFAULT to one (1)
 * if you want entries added to /etc/utmp by default, else set it to
 * zero (0).
 */
#define LOGINDEFAULT	1


/*
 * If UTMPOK is defined and your system (incorrectly) counts logins by
 * counting non-null entries in /etc/utmp (instead of counting non-null
 * entries with no hostname that are not on a pseudo tty), define USRLIMIT
 * to have screen put an upper-limit on the number of entries to write
 * into /etc/utmp.  This helps to keep you from exceeding a limited-user
 * license.
 */
#undef USRLIMIT



/**********************************************************************
 *
 *	End of User Configuration Section
 *
 *      Rest of this file is modified by 'configure'
 *      Change at your own risk!
 *
 */

/*
 * Define POSIX if your system supports IEEE Std 1003.1-1988 (POSIX).
 */
#define POSIX 1

/*
 * Define BSDJOBS if you have BSD-style job control (both process
 * groups and a tty that deals correctly with them).
 */
#define BSDJOBS 1

/*
 * Define TERMIO if you have struct termio instead of struct sgttyb.
 * This is usually the case for SVID systems, where BSD uses sgttyb.
 * POSIX systems should define this anyway, even though they use
 * struct termios.
 */
#define TERMIO 1

/*
 * Define TERMINFO if your machine emulates the termcap routines
 * with the terminfo database.
 * Thus the .screenrc file is parsed for
 * the command 'terminfo' and not 'termcap'.
 */
#define TERMINFO 1

/*
 * If your library does not define ospeed, define this.
 */
#undef NEED_OSPEED

/*
 * Define SYSV if your machine is SYSV complient (Sys V, HPUX, A/UX)
 */
#define SYSV 1

/*
 * Define SIGVOID if your signal handlers return void.  On older
 * systems, signal returns int, but on newer ones, it returns void.
 */
#define SIGVOID 1 

/*
 * Define SYSVSIGS if signal handlers must be reinstalled after
 * they have been called.
 */
#undef SYSVSIGS

/*
 * Define BSDWAIT if your system defines a 'union wait' in <sys/wait.h>
 */
#undef BSDWAIT

/*
 * Define DIRENT if your system has <dirent.h> instead of <sys/dir.h>
 */
#define DIRENT 1

/*
 * If your system has getutent(), pututline(), etc. to write to the
 * utmp file, define GETUTENT.
 */
#define GETUTENT 1

/*
 * Define UTHOST if the utmp file has a host field.
 */
#undef UTHOST

/*
 * If ttyslot() breaks getlogin() by returning indexes to utmp entries
 * of type DEAD_PROCESS, then our getlogin() replacement should be
 * selected by defining BUGGYGETLOGIN.
 */
#undef BUGGYGETLOGIN

/*
 * If your system does not have the calls setreuid() and setregid(),
 * define NOREUID to force screen to use a forked process to safely
 * create output files without retaining any special privileges.
 * (Output logging will be disabled, however.)
 */
#define NOREUID 1

/*
 * If you want the "time" command to display the current load average
 * define LOADAV. Maybe you must install screen with the needed
 * privileges to read /dev/kmem.
 */
#define LOADAV 1

#define LOADAV_NUM 3
#define LOADAV_TYPE long
#define LOADAV_SCALE 1000
#undef LOADAV_GETLOADAVG
#define LOADAV_UNIX "/unix"
#define LOADAV_AVENRUN "avenrun"

#define NLIST_STRUCT 1
#undef NLIST_NAME_UNION

/*
 * If your system has the new format /etc/ttys (like 4.3 BSD) and the
 * getttyent(3) library functions, define GETTTYENT.
 */
#undef GETTTYENT

/*
 * Define USEBCOPY if the bcopy/memcpy from your system's C library
 * supports the overlapping of source and destination blocks.  When
 * undefined, screen uses its own (probably slower) version of bcopy().
 */
#undef USEBCOPY

/*
 * If your system has vsprintf() and requires the use of the macros in
 * "varargs.h" to use functions with variable arguments,
 * define USEVARARGS.
 */
#define USEVARARGS 1

/*
 * Define this if your system supports named pipes.
 */
#undef NAMEDPIPE

/*
 * Define this if your system exits select() immediatly if a pipe is
 * opened read-only and no writer has opened it.
 */
#define BROKEN_PIPE 1

/*
 * Define this if the unix-domain socket implementation doesn't
 * create a socket in the filesystem.
 */
#undef SOCK_NOT_IN_FS

/*
 * If your system has setenv() and unsetenv() define USESETENV
 */
#undef USESETENV

/*
 * If your system does not come with a setenv()/putenv()/getenv()
 * functions, you may bring in our own code by defining NEEDPUTENV.
 */
#define NEEDPUTENV 1

/*
 * If the passwords are stored in a shadow file and you want the
 * builtin lock to work properly, define SHADOWPW.
 */
#define SHADOWPW 1

/*
 * If you are on a SYS V machine that restricts filename length to 14 
 * characters, you may need to enforce that by setting NAME_MAX to 14
 */
#ifdef NAME_MAX
# undef NAME_MAX	/* override system value */
#endif
#undef NAME_MAX

/* 
 * define PTYRANGE0 and or PTYRANGE1 if you want to adapt screen
 * to unusual environments. E.g. For SunOs the defaults are "qpr" and 
 * "0123456789abcdef". For SunOs 4.1.2 
 * #define PTYRANGE0 "pqrstuvwxyzPQRST" 
 * is recommended by Dan Jacobson.
 */
#undef PTYRANGE0
#undef PTYRANGE1

/*
 * some defines to prevent redeclarations/retypedefs
 */
#undef CRYPT_DECLARED
#undef GETHOSTNAME_DECLARED
#define KILLSTUFF_DECLARED 1
#define MEMFUNCS_DECLARED 1
#undef MKNOD_DECLARED
#undef NLIST_DECLARED
#define PUTENV_DECLARED 1
#undef REUID_DECLARED
#define SETPGID_DECLARED 1
#undef VPRNT_DECLARED
#define WAITSTUFF_DECLARED 1
#undef GETDTABLESIZE_DECLARED
#undef SELECT_DECLARED
#undef INDEX_DECLARED

#undef SIG_T_DEFINED
#define PID_T_DEFINED 1
#define UID_T_DEFINED 1
