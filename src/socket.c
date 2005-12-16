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

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifndef sgi
# include <sys/file.h>
#endif
#ifndef NAMEDPIPE
#include <sys/socket.h>
#endif
#include <fcntl.h>
#ifndef NAMEDPIPE
#include <sys/un.h>
#endif
#include <signal.h>
#ifndef M_XENIX
#include <sys/time.h>
#endif /* M_XENIX */
#ifdef DIRENT
# include <sys/param.h>
# include <dirent.h>
#else
# include <sys/dir.h>
# define dirent direct
#endif

#include "screen.h"

#ifdef USEVARARGS
# if defined(__STDC__)
#  include <stdarg.h>
# else
#  include <varargs.h>
# endif
#endif

#include "extern.h"

#if defined(_SEQUENT_) && !defined(NAMEDPIPE)
# define connect sconnect	/* _SEQUENT_ has braindamaged connect */
#endif

extern char *RcFileName, *extra_incap, *extra_outcap;
extern ServerSocket, real_uid, real_gid, eff_uid, eff_gid;
extern int dflag, rflag, lsflag, quietflag, wipeflag, xflag;
extern char *attach_tty, *LoginName, HostName[];
extern struct display *display, *displays;
extern struct win *fore, *console_window, *windows;
extern struct NewWindow nwin_undef;
#ifdef NETHACK
extern nethackflag;
#endif
#ifdef MULTIUSER
extern char *multi;
#endif

#ifdef PASSWORD
extern int CheckPassword;
extern char Password[];
#endif
extern char *getenv();

char SockPath[MAXPATH];
char *SockNamePtr, *SockName;

#ifdef MULTI_USER
# define SOCKMODE (S_IWRITE | S_IREAD | (displays ? S_IEXEC : 0) | (multi ? 1 : 0))
#else
# define SOCKMODE (S_IWRITE | S_IREAD | (displays ? S_IEXEC : 0))
#endif

int
RecoverSocket()
{
#ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
#endif
  (void) unlink(SockPath);
#ifndef NOREUID
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
#endif
  close(ServerSocket);
  if ((ServerSocket = MakeServerSocket()) < 0)
    return 0;
  return 1;
}

/*
 * Socket mode 700 means we are Attached. 600 is detached.
 * We return how many sockets we found. If it was exactly one, we come
 * back with a SockPath set to it and open it in a fd pointed to by fdp.
 * If fdp == 0 we simply produce a list if all sockets.
 */
/* ARGSUSED */
int
FindSocket(how, fdp)
int how;
int *fdp;
{
  register int s, lasts = 0, found = 0, deadcount = 0, wipecount = 0;
  register int l = 0;
  register DIR *dirp;
  register struct dirent *dp;
  register char *Name;
  struct stat st;
  struct foundsock
    {
      char *name;
      int mode;
    } foundsock[100];	/* 100 is hopefully enough. */
  int foundsockcount = 0;

  /* User may or may not give us a (prefix) SockName. We want to search. */
  debug("FindSocket:\n");
  if (SockName)
    {
      debug1("We want to match '%s'\n", SockName);
      l = strlen(SockName);
#ifdef NAME_MAX
      if (l > NAME_MAX)
	l = NAME_MAX;
#endif
    }

#ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
#endif
  debug1("FindSock searching... '%s'\n", SockPath);
  /*
   * this is a hack: SockName may point to Filename(Sockpath)...
   */
  found = *SockNamePtr;
  *SockNamePtr = '\0';
  if ((dirp = opendir(SockPath)) == NULL)
    {
      Panic(0, "Cannot opendir %s", SockPath);
      /* NOTREACHED */
    }
  *SockNamePtr = found;
  found = 0;
  while ((dp = readdir(dirp)) != NULL)
    {
      Name = dp->d_name;
      /* 
       * there may be a file ".termcap" here. 
       * Ignore it just like "." and "..". 
       */
      if (Name[0] == '.')
	continue;
      if (SockName && l)
	{
	  register char *n = Name;

	  debug2("Attach found: '%s', needed '%s'\n", Name, SockName);
	  /*
	   * The SockNames "hf", "ttyhf", "1", "12345.tty", "12345.ttyhf.medusa"
	   * all match the Name "12345.ttyhf.medusa".
	   */

	  if ((*SockName <= '0' || *SockName > '9') && (*n > '0' && *n <= '9'))
	    {
	      while (*++n)
		if (*n == '.')
		  {
		    n++;
		    break;
		  }
	      if (strncmp("tty", SockName, 3) && !strncmp("tty", n, 3))
		n += 3;
	    }
	  if (strncmp(n, SockName, l))
	    {
	      debug3("strncmp('%s', '%s', %d)\n", n, SockName, l);
	      continue;
	    }
	}
      /*
       * ATTENTION! MakeClientSocket adds SockName to SockPath! 
       * Anyway, we need it earlier.
       */
      strcpy(SockNamePtr, Name);
      if (stat(SockPath, &st))
	continue;
#ifndef SOCK_NOT_IN_FS
# ifdef NAMEDPIPE
      if (!S_ISFIFO(st.st_mode))
	{
	  debug1("'%s' is not a pipe, ignored\n", SockPath);
	  continue;
	}
# else /* NAMEDPIPE */
#  ifdef S_ISSOCK
      if (!S_ISSOCK(st.st_mode))
	{
	  debug1("'%s' is not a socket, ignored\n", SockPath);
	  continue;
	}
#  endif
# endif /* NAMEDPIPE */
#endif
      if (st.st_uid != real_uid)
	continue;
      s = st.st_mode & 0777;
#ifdef MULTIUSER
      if ((s & 0677) == (multi ? 0600 : 0601))
	continue;
#endif
      foundsock[foundsockcount].name = SaveStr(Name);
#ifdef MULTIUSER
      foundsock[foundsockcount].mode = s ^ (multi ? 1 : 0);
#else
      foundsock[foundsockcount].mode = s;
#endif
      debug2("FindSocket: %s has mode %04o...\n", Name, s);
      if ((s & 0677) == 0600)
	{
	  /* 
	   * marc parses the socketname again
	   */
	  int sockmpid = 0;
	  char *nam = Name;

	  while (*nam)
	    {
	      if (*nam > '9' || *nam < '0')
		break;
	      sockmpid = 10 * sockmpid + *nam - '0';
	      nam++;
	    }
	  /*
	   * We try to connect through the socket. If successfull, 
	   * thats o.k. Otherwise we record that mode as -1.
	   * MakeClientSocket() must be careful not to block forever.
	   */
	  if ( ((sockmpid > 2) && kill(sockmpid, 0) == -1 && errno == ESRCH) ||
	      (s = MakeClientSocket(0, Name)) == -1)
	    { 
	      foundsock[foundsockcount].mode = -1;
	      deadcount++;
	    }
	  else
	    close(s);
	}
      if (++foundsockcount >= 100)
	break;
    }
  closedir(dirp);

  if (wipeflag)
    {
      for (s = 0; s < foundsockcount; s++)
	{
	  if (foundsock[s].mode == -1)
	    {
              strcpy(SockNamePtr, foundsock[s].name);
	      debug1("wiping '%s'\n", SockPath);
	      if (unlink(SockPath) == 0)
	        {
		  foundsock[s].mode = -2;
	          wipecount++;
		}
	    }
	}
    }
  for (s = 0; s < foundsockcount; s++)
    if ((foundsock[s].mode) == (dflag ? 0700 : 0600)
        || (xflag && foundsock[s].mode == 0700)) 
      {
	found++;
	lasts = s;
      }
  if (quietflag && (lsflag || (found != 1 && rflag != 2)))
    eexit(10 + found);
  debug2("attach: found=%d, foundsockcount=%d\n", found, foundsockcount);
  if (found == 1 && lsflag == 0)
    {
      if ((lasts = MakeClientSocket(0, SockName = foundsock[lasts].name)) == -1)
        found = 0;
    }
  else if (!quietflag && foundsockcount > 0)
    {
      switch (found)
        {
        case 0:
          if (lsflag)
	    {
#ifdef NETHACK
              if (nethackflag)
	        printf("Your inventory:\n");
	      else
#endif
	      printf((foundsockcount > 1) ?
	             "There are screens on:\n" : "There is a screen on:\n");
	    }
          else
	    {
#ifdef NETHACK
              if (nethackflag)
	        printf("Nothing fitting exists in the game:\n");
	      else
#endif
	      printf((foundsockcount > 1) ?
	             "There are screens on:\n" : "There is a screen on:\n");
	    }
          break;
        case 1:
#ifdef NETHACK
          if (nethackflag)
            printf((foundsockcount > 1) ?
                   "Prove thyself worthy or perish:\n" : 
                   "You see here a good looking screen:\n");
          else
#endif
          printf((foundsockcount > 1) ? 
                 "There are several screens on:\n" :
                 "There is a possible screen on:\n");
          break;
        default:
#ifdef NETHACK
          if (nethackflag)
            printf((foundsockcount > 1) ? 
                   "You may whish for a screen, what do you want?\n" : 
                   "You see here a screen:\n");
          else
#endif
          printf((foundsockcount > 1) ?
                 "There are several screens on:\n" : "There is a screen on:\n");
          break;
        }
      for (s = 0; s < foundsockcount; s++)
	{
	  switch (foundsock[s].mode)
	    {
	    case 0700:
	      printf("\t%s\t(Attached)\n", foundsock[s].name);
	      break;
	    case 0600:
	      printf("\t%s\t(Detached)\n", foundsock[s].name);
	      break;
#ifdef MULTIUSER
	    case 0701:
	      printf("\t%s\t(Multi, attached)\n", foundsock[s].name);
	      break;
	    case 0601:
	      printf("\t%s\t(Multi, detached)\n", foundsock[s].name);
	      break;
#endif
	    case -1:
#if defined(__STDC__) || defined(_AIX)
	      printf("\t%s\t(Dead ??\?)\n", foundsock[s].name);
#else
	      printf("\t%s\t(Dead ???)\n", foundsock[s].name);
#endif
	      break;
	    case -2:
	      printf("\t%s\t(Removed)\n", foundsock[s].name);
	      break;
	    default:
	      printf("\t%s\t(Wrong mode)\n", foundsock[s].name);
	      break;
	    }
	}
    }
  if (deadcount && !quietflag)
    {
      if (wipeflag)
        {
#ifdef NETHACK
          if (nethackflag)
            printf("You hear%s distant explosion%s.\n",
	       (deadcount > 1) ? "" : " a", (deadcount > 1) ? "s" : "");
          else
#endif
          printf("%d Socket%s wiped out.\n", deadcount, (deadcount > 1)?"s":"");
	}
      else
	{
#ifdef NETHACK
          if (nethackflag)
            printf("The dead screen%s touch%s you. Try 'screen -wipe'.\n",
	       (deadcount > 1) ? "s" : "", (deadcount > 1) ? "" : "es");
          else
#endif
          printf("Remove dead Sockets with 'screen -wipe'.\n");
	}
    }

  for (s = 0; s < foundsockcount; s++)
    Free(foundsock[s].name);
  if (found == 1 && fdp)
    *fdp = lasts;
#ifndef NOREUID
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
#endif
  if (fdp)
    return found;
  return foundsockcount - wipecount;
}

#ifdef NAMEDPIPE

int
MakeServerSocket()
{
  register int s;
  struct stat st;

  strcpy(SockNamePtr, SockName);
# ifdef NAME_MAX
  if (strlen(SockNamePtr) > NAME_MAX)
    {
      debug2("MakeClientSocket: '%s' truncated to %d chars\n",
	     SockNamePtr, NAME_MAX);
      SockNamePtr[NAME_MAX] = '\0';
    }
# endif /* NAME_MAX */

# ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
# endif /* NOREUID */
  if ((s = open(SockPath, O_WRONLY | O_NDELAY)) >= 0)
    {
      debug("huii, my fifo already exists??\n");
      if (quietflag)
	{
	  Kill(d_userpid, SIG_BYE);
	  eexit(11);
	}
      printf("There is already a screen running on %s.\n",
	     Filename(SockPath));
      if (stat(SockPath, &st) == -1)
	Panic(errno, "stat");
      if (st.st_uid != real_uid)
	Panic(0, "Unfortunatelly you are not its owner.");
      if ((st.st_mode & 0700) == 0600)
	Panic(0, "To resume it, use \"screen -r\"");
      else
	Panic(0, "It is not detached.");
      /* NOTREACHED */
    }
# ifndef NOREUID
  (void) unlink(SockPath);
  if (mknod(SockPath, S_IFIFO | SOCKMODE, 0))
    Panic(0, "mknod fifo %s failed", SockPath);
#  ifdef BROKEN_PIPE
  if ((s = open(SockPath, O_RDWR | O_NDELAY, 0)) < 0)
#  else
  if ((s = open(SockPath, O_RDONLY | O_NDELAY, 0)) < 0)
#  endif
    Panic(errno, "open fifo %s", SockPath);
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
  return s;
# else /* NOREUID */
  if (UserContext() > 0)
    {
      (void) unlink(SockPath);
      if (mknod(SockPath, S_IFIFO | SOCKMODE, 0))
	UserReturn(0);
      UserReturn(1);
    }
  if (UserStatus() <= 0)
    Panic(0, "mknod fifo %s failed", SockPath);
#  ifdef BROKEN_PIPE
  if ((s = secopen(SockPath, O_RDWR | O_NDELAY, 0)) < 0)
#  else
  if ((s = secopen(SockPath, O_RDONLY | O_NDELAY, 0)) < 0)
#  endif
    Panic(errno, "open fifo %s", SockPath);
  return s;
# endif /* NOREUID */
}


int
MakeClientSocket(err, name)
int err;
char *name;
{
  register int s = 0;

  strcpy(SockNamePtr, name);
# ifdef NAME_MAX
  if (strlen(SockNamePtr) > NAME_MAX)
    {
      debug2("MakeClientSocket: '%s' truncated to %d chars\n",
	     SockNamePtr, NAME_MAX);
      SockNamePtr[NAME_MAX] = '\0';
    }
# endif /* NAME_MAX */
  
  if ((s = secopen(SockPath, O_WRONLY | O_NDELAY, 0)) >= 0)
    {
      (void) fcntl(s, F_SETFL, 0);
      return s;
    }
  if (err)
    Msg(errno, "open: %s (but continuing...)", SockPath);
  debug1("MakeClientSocket() open %s failed\n", SockPath);
  return -1;
}

#else	/* NAMEDPIPE */

int
MakeServerSocket()
{
  register int s;
  struct sockaddr_un a;
  struct stat st;

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    Panic(errno, "socket");
  a.sun_family = AF_UNIX;
  strcpy(SockNamePtr, SockName);
# ifdef NAME_MAX
  if (strlen(SockNamePtr) > NAME_MAX)
    {
      debug2("MakeServerSocket: '%s' truncated to %d chars\n",
	     SockNamePtr, NAME_MAX);
      SockNamePtr[NAME_MAX] = '\0';
    }
# endif /* NAME_MAX */

  strcpy(a.sun_path, SockPath);
# ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
# endif /* NOREUID */
  if (connect(s, (struct sockaddr *) & a, strlen(SockPath) + 2) != -1)
    {
      debug("oooooh! socket already is alive!\n");
      if (quietflag)
	{ 
	  Kill(d_userpid, SIG_BYE);
	  /* 
	   * oh, well. nobody receives that return code. papa 
	   * dies by signal.
	   */
	  eexit(11);
	}
      printf("There is already a screen running on %s.\n",
	     Filename(SockPath));
      if (stat(SockPath, &st) == -1)
	Panic(errno, "stat");
      if (st.st_uid != real_uid)
	Panic(0, "Unfortunatelly you are not its owner.");
      if ((st.st_mode & 0700) == 0600)
	Panic(0, "To resume it, use \"screen -r\"");
      else
	Panic(0, "It is not detached.");
      /* NOTREACHED */
    }
#if defined(m88k) || defined(m68k)
  close(s);	/* we get bind: Invalid argument if this is not done */
  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    Panic(errno, "reopen socket");
#endif
  (void) unlink(SockPath);
  if (bind(s, (struct sockaddr *) & a, strlen(SockPath) + 2) == -1)
    Panic(errno, "bind (%s)", SockPath);
#ifdef SOCK_NOT_IN_FS
    {
      int f;
      if (f = secopen(SockPath, O_RDWR | O_CREAT, SOCKMODE) < 0)
        Panic(errno, "shadow socket open");
      close(f);
    }
#else
  chmod(SockPath, SOCKMODE);
# ifdef NOREUID
  chown(SockPath, real_uid, real_gid);
# endif /* NOREUID */
#endif /* SOCK_NOT_IN_FS */
  if (listen(s, 5) == -1)
    Panic(errno, "listen");
# ifdef F_SETOWN
  fcntl(s, F_SETOWN, getpid());
  debug1("Serversocket owned by %d\n", fcntl(s, F_GETOWN, 0));
# endif /* F_SETOWN */
# ifndef NOREUID
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
# endif /* NOREUID */
  return s;
}

int
MakeClientSocket(err, name)
int err;
char *name;
{
  register int s;
  struct sockaddr_un a;

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    Panic(errno, "socket");
  a.sun_family = AF_UNIX;
  strcpy(SockNamePtr, name);
# ifdef NAME_MAX
  if (strlen(SockNamePtr) > NAME_MAX)
    {
      debug2("MakeClientSocket: '%s' truncated to %d chars\n",
	     SockNamePtr, NAME_MAX);
      SockNamePtr[NAME_MAX] = '\0';
    }
# endif /* NAME_MAX */

  strcpy(a.sun_path, SockPath);
# ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
# else /* NOREUID */
  if (access(SockPath, W_OK))
    {
      if (err)
	Msg(errno, "%s", SockPath);
      debug2("MakeClientSocket: access(%s): %d.\n", SockPath, errno);
      close(s);
      return -1;
    }
# endif /* NOREUID */
  if (connect(s, (struct sockaddr *) & a, strlen(SockPath) + 2) == -1)
    {
      if (err)
	Msg(errno, "%s: connect", SockPath);
      debug("MakeClientSocket: connect failed.\n");
      close(s);
      s = -1;
    }
# ifndef NOREUID
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
# endif /* NOREUID */
  return s;
}
#endif /* NAMEDPIPE */


void
SendCreateMsg(s, nwin)
int s;
struct NewWindow *nwin;
{
  struct msg m;
  register char *p;
  register int len, n;
  char **av = nwin->args;

  debug1("SendCreateMsg() to '%s'\n", SockPath);
  bzero((char *)&m, sizeof(m));
  m.type = MSG_CREATE;
  strcpy(m.m_tty, attach_tty);
  p = m.m.create.line;
  n = 0;
  if (nwin->args != nwin_undef.args)
    for (av = nwin->args; *av && n < MAXARGS - 1; ++av, ++n)
      {
        len = strlen(*av) + 1;
        if (p + len >= m.m.create.line + MAXPATH - 1)
	  break;
        strcpy(p, *av);
        p += len;
      }
  if (nwin->aka != nwin_undef.aka && p + strlen(nwin->aka) + 1 < m.m.create.line + MAXPATH)
    strcpy(p, nwin->aka);
  else
    *p = '\0';
  m.m.create.nargs = n;
  m.m.create.aflag = nwin->aflag;
  m.m.create.flowflag = nwin->flowflag;
  m.m.create.lflag = nwin->lflag;
  m.m.create.hheight = nwin->histheight;
#ifdef SYSV
  if (getcwd(m.m.create.dir, sizeof(m.m.create.dir)) == 0)
#else
  if (getwd(m.m.create.dir) == 0)
#endif
    {
      Msg(errno, "%s", m.m.create.dir);
      return;
    }
  if (nwin->term != nwin_undef.term)
    strncpy(m.m.create.screenterm, nwin->term, 19);
  m.m.create.screenterm[19] = '\0';
  debug1("SendCreateMsg writing '%s'\n", m.m.create.line);
  if (write(s, (char *) &m, sizeof m) != sizeof m)
    Msg(errno, "write");
}

void
#ifdef USEVARARGS
/*VARARGS1*/
# if defined(__STDC__)
SendErrorMsg(char *fmt, ...)
# else /* __STDC__ */
SendErrorMsg(fmt, va_alist)
char *fmt;
va_dcl
# endif /* __STDC__ */
{ /* } */
  static va_list ap;
#else /* USEVARARGS */
/*VARARGS1*/
SendErrorMsg(fmt, p1, p2, p3, p4, p5, p6)
char *fmt;
unsigned long p1, p2, p3, p4, p5, p6;
{
#endif /* USEVARARGS */
  register int s;
  struct msg m;

#ifdef USEVARARGS
# if defined(__STDC__)
  va_start(ap, fmt);
# else /* __STDC__ */
  va_start(ap);
# endif /* __STDC__ */
  (void) vsprintf(m.m.message, fmt, ap);
  va_end(ap);
#else /* USEVARARGS */
  sprintf(m.m.message, fmt, p1, p2, p3, p4, p5, p6);
#endif /* USEVARARGS */
  debug1("SendErrorMsg: '%s'\n", m.m.message);
  if (display == 0)
    return;
  s = MakeClientSocket(1, SockName);
  m.type = MSG_ERROR;
  strcpy(m.m_tty, d_usertty);
  debug1("SendErrorMsg(): writing to '%s'\n", SockPath);
  (void) write(s, (char *) &m, sizeof m);
  close(s);
  sleep(2);
}

#ifdef PASSWORD
static int
CheckPasswd(pwd, pid, utty)
int pid;
char *pwd, *utty;
{
  if (CheckPassword && 
      strcmp(crypt(pwd, (strlen(Password) > 1) ? Password : "JW"),
	     Password))
    {
      if (*pwd)
	{
# ifdef NETHACK
          if (nethackflag)
	    Msg(0, "'%s' tries to explode in the sky, but fails. (%s)", utty, pwd);
          else
# endif /* NETHACK */
	  Msg(0, "Illegal reattach attempt from terminal %s, \"%s\"", utty, pwd);
	}
      debug1("CheckPass() wrong password kill(%d, SIG_PW_FAIL)\n", pid);
      Kill(pid, SIG_PW_FAIL);
      return 0;
    }
  debug1("CheckPass() from %d happy\n", pid);
  Kill(pid, SIG_PW_OK);
  return 1;
}
#endif	/* PASSWORD */

static void
ExecCreate(mp)
struct msg *mp;
{
  struct NewWindow nwin;
  char *args[MAXARGS];
  register int n;
  register char **pp = args, *p = mp->m.create.line;

  nwin = nwin_undef;
  for (n = mp->m.create.nargs; n > 0; --n)
    {
      *pp++ = p;
      p += strlen(p) + 1;
    }
  *pp = 0;
  if (*p)
    nwin.aka = p;
  if (*args)
    nwin.args = args;
  nwin.aflag = mp->m.create.aflag;
  nwin.flowflag = mp->m.create.flowflag;
  if (*mp->m.create.dir)
    nwin.dir = mp->m.create.dir;
  nwin.lflag = mp->m.create.lflag;
  nwin.histheight = mp->m.create.hheight;
  if (*mp->m.create.screenterm)
    nwin.term =  mp->m.create.screenterm;
  MakeWindow(&nwin);
}

static int
CheckPid(pid)
int pid;
{
  debug1("Checking pid %d\n", pid);
  if (pid < 2)
    return(-1);
  if (eff_uid == real_uid)
    return kill(pid, 0);
  if (UserContext() == 1)
    {
      UserReturn(kill(pid, 0));
    }
  return UserStatus();
}

void
ReceiveMsg()
{
  int left, len, i;
  static struct msg m;
  char *p;
  int ns = ServerSocket;
  struct display *next;
  struct mode Mode;
#ifdef UTMPOK
  struct win *wi;
#endif

#ifdef NAMEDPIPE
  debug("Ha, there was someone knocking on my fifo??\n");
  if (fcntl(ServerSocket, F_SETFL, 0) == -1)
    Panic(errno, "DELAY fcntl");
#else
  struct sockaddr_un a;

  len = sizeof(a);
  debug("Ha, there was someone knocking on my socket??\n");
  if ((ns = accept(ns, (struct sockaddr *) &a, &len)) < 0)
    {
      Msg(errno, "accept");
      return;
    }
#endif				/* NAMEDPIPE */

  p = (char *) &m;
  left = sizeof(m);
  while (left > 0)
    {
      len = read(ns, p, left);
      if (len < 0 && errno == EINTR)
	continue;
      if (len <= 0)
	break;
      p += len;
      left -= len;
    }

#ifdef NAMEDPIPE
# ifndef BROKEN_PIPE
  /* Reopen pipe to prevent EOFs at the select() call */
  close(ServerSocket);
  if ((ServerSocket = secopen(SockPath, O_RDONLY | O_NDELAY, 0)) < 0)
    Panic(errno, "reopen fifo %s", SockPath);
# endif
#else
  close(ns);
#endif

  if (len < 0)
    {
      Msg(errno, "read");
      return;
    }
  if (left > 0)
    {
      if (left != sizeof(m))
        Msg(0, "Message %d of %d bytes too small", left, sizeof(m));
      else
	debug("No data on socket.\n");
      return;
    }
  debug2("*** RecMsg: type %d tty %s\n", m.type, m.m_tty);
  for (display = displays; display; display = display->_d_next)
    if (strcmp(d_usertty, m.m_tty) == 0)
      break;
  debug2("display: %s %sfound\n", m.m_tty, display ? "" : "not ");
  if (!display)
    {
      struct win *w;

      for (w = windows; w; w = w->w_next)
        if (!strcmp(m.m_tty, w->w_tty))
	  {
            display = w->w_display;
	    debug2("but window %s %sfound.\n", m.m_tty, display ? "" : "deatached (ignoring) ");
	    break;
          }
    }

  /* Remove the d_status to prevent garbage on the screen */
  if (display && d_status)
    RemoveStatus();

  switch (m.type)
    {
    case MSG_WINCH:
      if (display)
        CheckScreenSize(1); /* Change fore */
      break;
    case MSG_CREATE:
      if (display)
	ExecCreate(&m);
      break;
    case MSG_CONT:
	if (display && d_userpid != 0 && kill(d_userpid, 0) == 0)
	  break;		/* Intruder Alert */
      debug2("RecMsg: apid=%d,was %d\n", m.m.attach.apid, display ? d_userpid : 0);
      /* FALLTHROUGH */
    case MSG_ATTACH:
      if (CheckPid(m.m.attach.apid))
	{
	  debug1("Attach attempt with bad pid(%d)\n", m.m.attach.apid);
	  Msg(0, "Attach attempt with bad pid(%d) !", m.m.attach.apid);
          break;
	}
      if ((i = secopen(m.m_tty, O_RDWR | O_NDELAY, 0)) < 0)
	{
	  debug1("ALERT: Cannot open %s!\n", m.m_tty);
#ifdef NETHACK
          if (nethackflag)
	    Msg(errno, 
	        "You can't open (%s). Perhaps there's a Monster behind it",
	        m.m_tty);
          else
#endif
	  Msg(errno, "Attach: Could not open %s", m.m_tty);
	  Kill(m.m.attach.apid, SIG_BYE);
	  break;
	}
#ifdef PASSWORD
      if (!CheckPasswd(m.m.attach.password, m.m.attach.apid, m.m_tty))
	{
	  debug3("RcvMsg:Checkpass(%s,%d,%s) failed\n",
		 m.m.attach.password, m.m.attach.apid, m.m_tty);
	  close(i);
	  break;
	}
#else
# ifdef MULTIUSER
      Kill(m.m.attach.apid, SIGCONT);
# endif
#endif				/* PASSWORD */
      if (display)
	{
	  debug("RecMsg: hey, why you disturb, we are not detached. hangup!\n");
	  close(i);
	  Kill(m.m.attach.apid, SIG_BYE);
	  Msg(0, "Attach msg ignored: We are not detached.");
	  break;
	}

#ifdef MULTIUSER
      if (strcmp(m.m.attach.auser, LoginName))
        if (findacl(m.m.attach.auser) == 0)
	  {
              write(i, "Access to session denied.\n", 26);
	      close(i);
	      Kill(m.m.attach.apid, SIG_BYE);
	      Msg(0, "Attach: access denied for d_user %s", m.m.attach.auser);
	      break;
	  }
#endif

      errno = 0;
      debug2("RecMsg: apid %d is o.k. and we just opened '%s'\n", m.m.attach.apid, m.m_tty);

      /* create new display */
      GetTTY(i, &Mode);
      if (MakeDisplay(m.m.attach.auser, m.m_tty, m.m.attach.envterm, i, m.m.attach.apid, &Mode) == 0)
        {
	  write(i, "Could not make display.\n", 24);
	  close(i);
	  Msg(errno, "Attach: could not make display for d_user %s", m.m.attach.auser);
	  Kill(m.m.attach.apid, SIG_BYE);
	  break;
        }
#if defined(pyr) || defined(xelos) || defined(sequent)
      /*
       * Kludge for systems with braindamaged termcap routines,
       * which evaluate $TERMCAP, regardless weather it describes
       * the correct terminal type or not.
       */
      debug("unsetenv(TERMCAP) in case of a different terminal");
      unsetenv("TERMCAP");
#endif
    
      /*
       * We reboot our Terminal Emulator. Forget all we knew about
       * the old terminal, reread the termcap entries in .screenrc
       * (and nothing more from .screenrc is read. Mainly because
       * I did not check, weather a full reinit is save. jw) 
       * and /etc/screenrc, and initialise anew.
       */
      if (extra_outcap)
	free(extra_outcap);
      if (extra_incap)
	free(extra_incap);
      extra_incap = extra_outcap = 0;
      debug2("Message says size (%dx%d)\n", m.m.attach.columns, m.m.attach.lines);
#ifdef ETCSCREENRC
      if ((p = getenv("SYSSCREENRC")) == NULL)
	StartRc(ETCSCREENRC);
      else
	StartRc(p);
#endif
      StartRc(RcFileName);
      if (InitTermcap(m.m.attach.columns, m.m.attach.lines))
	{
	  FreeDisplay();
	  Kill(m.m.attach.apid, SIG_BYE);
	  break;
	}
      InitTerm(m.m.attach.adaptflag);
      if (displays->_d_next == 0)
        (void) chsock();
      signal(SIGHUP, SigHup);
#ifdef UTMPOK
      /*
       * we set the Utmp slots again, if we were detached normally
       * and if we were detached by ^Z.
       */
      RemoveLoginSlot();
      if (displays->_d_next == 0)
        for (wi = windows; wi; wi = wi->w_next)
	  if (wi->w_slot != (slot_t) -1)
	    SetUtmp(wi);
#endif
      SetMode(&d_OldMode, &d_NewMode);
      SetTTY(d_userfd, &d_NewMode);
      if (fore && fore->w_display == 0)
        SetForeWindow(fore);
      else
        d_fore = 0;
      Activate(0);
      if (displays->_d_next == 0 && console_window)
	{
	  if (TtyGrabConsole(console_window->w_ptyfd, 1, "reattach") == 0)
	    Msg(0, "console %s is on window %d", HostName, console_window->w_number);
	}
      debug("activated...\n");
      break;
    case MSG_ERROR:
      Msg(0, "%s", m.m.message);
      break;
    case MSG_HANGUP:
      SigHup(SIGARG);
      break;
#ifdef REMOTE_DETACH
    case MSG_DETACH:
# ifdef POW_DETACH
    case MSG_POW_DETACH:
# endif				/* POW_DETACH */
      for (display = displays; display; display = next)
	{
	  next = display->_d_next;
# ifdef POW_DETACH
	  if (m.type == MSG_POW_DETACH)
	    Detach(D_REMOTE_POWER);
	  else
# endif				/* POW_DETACH */
	  if (m.type == MSG_DETACH)
	    Detach(D_REMOTE);
	}
      break;
#endif
    default:
      Msg(0, "Invalid message (type %d).", m.type);
    }
}

#if defined(_SEQUENT_) && !defined(NAMEDPIPE)
#undef connect

/*
 *  sequent_ptx socket emulation must have mode 000 on the socket!
 */
int
sconnect(s, sapp, len)
int s, len;
struct sockaddr *sapp;
{
  register struct sockaddr_un *sap;
  struct stat st;
  int x;

  sap = (struct sockaddr_un *)sapp;
  if (stat(sap->sun_path, &st))
    return -1;
  chmod(sap->sun_path, 0);
  x = connect(s, (struct sockaddr *) sap, len);
  chmod(sap->sun_path, st.st_mode);
  return x;
}
#endif

int
chsock()
{
  int r, euid = geteuid();
  if (euid != real_uid)
    {
      if (UserContext() <= 0)
        return UserStatus();
    }
  r = chmod(SockPath, SOCKMODE);
  if (euid != real_uid)
    UserReturn(r);
  return r;
}

