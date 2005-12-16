/* Copyright (c) 1987-1990 Oliver Laumann and Wayne Davison.
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

/*
 *		Beginning of User Configuration Section
 *
 * First off, you should decide if you intend to install screen set-uid to
 * root.  This isn't necessary to use screen, but it allows the pseudo-ttys
 * to be set to their proper owner (for security purposes), /etc/utmp to be
 * updated, and /dev/kmem to be accessed to read the load average values.
 *
 * An alternative to installing screen set-uid root is to install it set-gid
 * utmp (with the file /etc/utmp installed to be group-utmp writable) or
 * set-gid kmem (with /dev/kmem set to be group-kmem readable) or some other
 * custom group to give you both.  The final alternative is to omit /etc/utmp
 * updating and the /dev/kmem reading (see the following defines) and simply
 * run screen as a regular program -- its major functions will be unaffected.
 *
 * The following defines are set up for an "average" BSD system without any
 * special privileges (not suid root, no load-average, no /etc/utmp updating).
 */

/*
 * If screen is going to be installed set-uid root, you MUST define SUIDROOT.
 */
/*#define SUIDROOT			/**/

/*
 * If screen is installed with permissions to update /etc/utmp (such as if
 * it is installed set-uid root), define UTMPOK.  Set LOGINDEFAULT to one (1)
 * if you want entries added to /etc/utmp by default, else set it to zero (0).
 */
/*#define UTMPOK 			/**/
/*#define LOGINDEFAULT	1		/**/

/*
 * If UTMPOK is defined and your system (incorrectly) counts logins by
 * counting non-null entries in /etc/utmp (instead of counting non-null
 * entries with no hostname that are not on a pseudo tty), define USRLIMIT
 * to have screen put an upper-limit on the number of entries to write
 * into /etc/utmp.  This helps to keep you from exceeding a limited-user
 * license.
 */
/*#define USRLIMIT 32			/**/

/*
 * If screen is NOT installed set-uid root, screen can provide tty security
 * by exclusively locking the ptys.  While this keeps other users from
 * opening your ptys, it also keeps your own subprocesses from being able
 * to open /dev/tty.  Define LOCKPTY to add this exclusive locking.
 */
/*#define LOCKPTY 			/**/

/*
 * If your system does not have the calls setreuid() and setregid(), define
 * NOREUID to force screen to use a forked process to safely create output
 * files without retaining any special privileges.  (Output logging will be
 * disabled, however.)
 */
/*#define NOREUID 			/**/

/*
 * If you want the "time" command to display the current load average
 * you must install screen with the needed privileges to read /dev/kmem
 * and have a load average format that screen understands.  We handle the
 * following formats:  3 doubles (BSD), 3 longs (sun), and 4 longs (alliant).
 */
/*#define LOADAV_3DOUBLES 		/**/
/*#define LOADAV_3LONGS			/**/
/*#define LOADAV_4LONGS			/**/

/*
 * If your system has the new format /etc/ttys (like 4.3 BSD) and the
 * getttyent(3) library functions, define GETTTYENT.
 */
/*#define GETTTYENT			/**/

/*
 * If your version of NFS supports named sockets and you install screen
 * suid root, you may need to define NFS_HACK for screen to be able to
 * open the sockets.
 */
/*#define NFS_HACK			/**/

/*
 * By default screen will create a directory named ".screen" in the user's
 * HOME directory to contain the named sockets.  If this causes you problems
 * (e.g. some user's HOME directories are NFS-mounted and don't support
 * named sockets) you can have screen create the socket directories in a
 * common subdirectory, such as /tmp or /usr/tmp.  It makes things a little
 * more secure if you choose a directory where the "sticky" bit is on, but
 * this isn't required.  Screen will name the subdirectories "S-$USER"
 * (e.g /tmp/S-davison).
 */
/*#define SOCKDIR  "/tmp" 		/**/

/*
 * Define USEBCOPY if the bcopy() from your system's C library supports the
 * overlapping of source and destination blocks.  When undefined, screen
 * uses its own (probably slower) version of bcopy().
 */
#define USEBCOPY			/**/

/*
 * If you'd rather see the status line on the first line of your terminal
 * rather than the last, define TOPSTAT.
 */
/*#define TOPSTAT 			/**/

/*
 * If your system has vsprintf() and requires the use of the macros in
 * "varargs.h" to use functions with variable arguments, define USEVARARGS.
 */
/*#define USEVARARGS			/**/

/*
 *	End of User Configuration Section
 */
