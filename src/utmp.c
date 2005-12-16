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


/*
 * An explanation of some weird things:
 *
 *  linux should have GETUTENT, but their pututline() doesn't have
 *  a return value.
 * 
 *  UTNOKEEP: A (ugly) hack for apollo that does two things:
 *    1) Always close and reopen the utmp file descriptor. (I don't know
 *       for what reason this is done...)
 *    2) Implement an unsorted utmp file much like GETUTENT.
 *  (split into UT_CLOSE and UT_UNSORTED)
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "screen.h"
#include "extern.h"


extern struct display *display;
extern struct win *fore;
extern char *LoginName;
#ifdef NETHACK
extern nethackflag;
#endif


#ifdef UTNOKEEP
# define UT_CLOSE
# define UT_UNSORTED
#endif


#ifdef UTMPOK

static slot_t TtyNameSlot __P((char *));

static int utmpok, utmpfd = -1;
static char UtmpName[] = UTMPFILE;


# if defined(GETUTENT) && !defined(SVR4)
#  if defined(hpux) /* cruel hpux release 8.0 */
#   define pututline _pututline
#  endif /* hpux */
extern struct utmp *getutline(), *pututline();
#  if defined(_SEQUENT_)
extern struct utmp *ut_add_user(), *ut_delete_user();
extern char *ut_find_host();
#   ifndef UTHOST
#    define UTHOST		/* _SEQUENT_ has got ut_find_host() */
#   endif
#  endif /* _SEQUENT_ */
# endif /* GETUTENT && !SVR4 */

# if !defined(GETUTENT) && !defined(UT_UNSORTED)
#  ifdef GETTTYENT
#   include <ttyent.h>
#  else
struct ttyent { char *ty_name; };
static void           setttyent __P((void));
static struct ttyent *getttyent __P((void));
#  endif
# endif /* !GETUTENT && !UT_UNSORTED */

#endif /* UTMPOK */




/*
 * SlotToggle - modify the utmp slot of the fore window.
 *
 * how > 0	do try to set a utmp slot.
 * how = 0	try to withdraw a utmp slot.
 *
 * w_slot = -1  window not logged in.
 * w_slot = 0   window not logged in, but should be logged in. 
 *              (unable to write utmp, or detached).
 */
void
SlotToggle(how)
int how;
{
  debug1("SlotToggle %d\n", how);
#ifdef UTMPOK
  if (how)
    {
      debug(" try to log in\n");
      if ((fore->w_slot == (slot_t) -1) || (fore->w_slot == (slot_t) 0))
	{
# ifdef USRLIMIT
          if (CountUsers() >= USRLIMIT)
            Msg(0, "User limit reached.");
          else
# endif
            {
              if (SetUtmp(fore) == 0)
                Msg(0, "This window is now logged in.");
              else
                Msg(0, "This window should now be logged in.");
            }
	}
      else
	Msg(0, "This window is already logged in.");
    }
  else
    {
      debug(" try to log out\n");
      if (fore->w_slot == (slot_t) -1)
	Msg(0, "This window is already logged out\n");
      else if (fore->w_slot == (slot_t) 0)
	{
	  debug("What a relief! In fact, it was not logged in\n");
	  Msg(0, "This window is not logged in.");
	  fore->w_slot = (slot_t) -1;
	}
      else
	{
	  RemoveUtmp(fore);
	  if (fore->w_slot != (slot_t) -1)
	    Msg(0, "What? Cannot remove Utmp slot?");
	  else
	    Msg(0, "This window is no longer logged in.");
	}
    }
#else	/* !UTMPOK */
# ifdef UTMPFILE
  Msg(0, "Unable to modify %s.\n", UTMPFILE);
# else
  Msg(0, "Unable to modify utmp-database.\n");
# endif
#endif
}




#ifdef UTMPOK



void
InitUtmp()
{
  debug1("InitUtmp testing '%s'...\n", UtmpName);
  if ((utmpfd = open(UtmpName, O_RDWR)) == -1)
    {
      if (errno != EACCES)
	Msg(errno, UtmpName);
      debug("InitUtmp failed.\n");
      utmpok = 0;
      return;
    }
#ifdef GETUTENT
  close(utmpfd);	/* it was just a test */
  utmpfd = -1;
#endif /* GETUTENT */
  utmpok = 1;
}


#ifdef USRLIMIT
int
CountUsers()
{
# ifdef GETUTENT
  struct utmp *ut, *getutent();
# else /* GETUTENT */
  struct utmp utmpbuf;
# endif /* GETUTENT */
  int UserCount;

# ifdef UT_CLOSE
  InitUtmp();
# endif /* UT_CLOSE */
  debug1("CountUsers() - utmpok=%d\n", utmpok);
  if (!utmpok)
    return 0;
  UserCount = 0;
# ifdef GETUTENT
  setutent();
  while (ut = getutent())
    if (ut->ut_type == USER_PROCESS)
      UserCount++;
# else /* GETUTENT */
  (void) lseek(utmpfd, (off_t) 0, 0);
  while (read(utmpfd, &utmpbuf, sizeof(utmpbuf)) == sizeof(utmpbuf))
    if (utmpbuf.ut_name[0] != '\0')
      UserCount++;
# endif /* GETUTENT */
# ifdef UT_CLOSE
  close(utmpfd);
# endif /* UT_CLOSE */
  return UserCount;
}
#endif /* USRLIMIT */



/*
 * the utmp entry for tty is located and removed.
 * it is stored in D_utmp_logintty.
 */
void
RemoveLoginSlot()
{
#ifdef GETUTENT
  struct utmp *uu;
#endif /* GETUTENT */
  struct utmp u;	/* 'empty' slot that we write back */
#ifdef _SEQUENT_
  char *p;
#endif /* _SEQUENT_ */

  ASSERT(display);
  debug("RemoveLoginSlot: removing your logintty\n");
  D_loginslot = TtyNameSlot(D_usertty);
  if (D_loginslot == (slot_t)0 || D_loginslot == (slot_t)-1)
    return;
#ifdef UT_CLOSE
  InitUtmp();
#endif /* UT_CLOSE */
  if (!utmpok)
    {
      debug("RemoveLoginSlot: utmpok == 0\n");
      return;
    }

#ifdef _SEQUENT_
  if (p = ut_find_host(D_loginslot))
    strncpy(D_loginhost, p, sizeof(D_loginhost) - 1);
  D_loginhost[sizeof(D_loginhost) - 1] = 0;
#endif /* _SEQUENT_ */

  bzero((char *) &u, sizeof(u));

#ifdef GETUTENT
  setutent();
  strncpy(u.ut_line, D_loginslot, sizeof(u.ut_line));
  if ((uu = getutline(&u)) == 0)
    {
      Msg(0, "Utmp slot not found -> not removed");
      return;
    }
  D_utmp_logintty = *uu;
# ifdef _SEQUENT_
  if (ut_delete_user(D_loginslot, uu->ut_pid, 0, 0) == 0)
# else /* _SEQUENT_ */
  u = *uu;
  u.ut_type = DEAD_PROCESS;
  u.ut_exit.e_termination = 0;
  u.ut_exit.e_exit= 0;
  if (pututline(&u) == 0)
# endif /* _SEQUENT_ */

#else /* GETUTENT */

  bzero((char *)&D_utmp_logintty, sizeof(u));
  (void) lseek(utmpfd, (off_t) (D_loginslot * sizeof(u)), 0);
  if (read(utmpfd, (char *) &D_utmp_logintty, sizeof(u)) != sizeof(u))
    {
      Msg(errno, "cannot read %s ??", UtmpName);
      sleep(1);
    }
# ifdef UT_UNSORTED
  /* copy tty line */
  bcopy((char *)&D_utmp_logintty, (char *)&u, sizeof(u));
  bzero(u.ut_name, sizeof(u.ut_name));
  bzero(u.ut_host, sizeof(u.ut_host));
#  ifdef linux
  u.ut_type = DEAD_PROCESS;
#  endif
# endif /* UT_UNSORTED */
  (void) lseek(utmpfd, (off_t) (D_loginslot * sizeof(u)), 0);
  if (write(utmpfd, (char *) &u, sizeof(u)) != sizeof(u))

#endif /* GETUTENT */

    {
#ifdef NETHACK
      if (nethackflag)
	Msg(errno, "%s is too hard to dig in", UtmpName); 
      else
#endif /* NETHACK */
      Msg(errno, "Could not write %s", UtmpName);
    }
#ifdef UT_CLOSE
  close(utmpfd);
#endif /* UT_CLOSE */
  debug1(" slot %d zapped\n", (int)D_loginslot);
}

/*
 * D_utmp_logintty is reinserted into utmp
 */
void
RestoreLoginSlot()
{
  debug("RestoreLoginSlot()\n");
  ASSERT(display);
#ifdef UT_CLOSE
  InitUtmp();
#endif /* UT_CLOSE */
  if (utmpok && D_loginslot != (slot_t)0 && D_loginslot != (slot_t)-1)
    {
#ifdef GETUTENT
# ifdef _SEQUENT_
      int fail;
      debug1(" logging you in again (slot %s)\n", D_loginslot);
/*
 * We have problems if we add the console and use ut_add_user()
 * because the id will be 'scon' instead of 'co'. So we
 * restore it with pututline(). The reason why we don't use
 * pututline all the time is that we want to set the host field.
 * Unfortunatelly this can only be done with ut_add_user().
 */
      if (*D_loginhost)
        {
          fail = (ut_add_user(D_utmp_logintty.ut_name, D_loginslot, D_utmp_logintty.ut_pid,
                              *D_loginhost ? D_loginhost : (char *)0) == 0);
        }
      else
        {
          setutent();
          fail = (pututline(&D_utmp_logintty) == 0);
        }
      if (fail)
# else	/* _SEQUENT_ */
      debug1(" logging you in again (slot %s)\n", D_loginslot);
      setutent();
      if (pututline(&D_utmp_logintty) == 0)
# endif	/* _SEQUENT */
#else	/* GETUTENT */
      debug1(" logging you in again (slot %d)\n", D_loginslot);
# ifdef sequent
      /* 
       * call sequent undocumented routine to count logins 
       * and add utmp entry if possible 
       */
      if (add_utmp(D_loginslot, &D_utmp_logintty) == -1)
# else /* sequent */
      (void) lseek(utmpfd, (off_t) (D_loginslot * sizeof(struct utmp)), 0);
      if (write(utmpfd, (char *) &D_utmp_logintty, sizeof(struct utmp))
	  != sizeof(struct utmp))
# endif /* sequent */
#endif	/* GETUTENT */
        {
#ifdef NETHACK
          if (nethackflag)
            Msg(errno, "%s is too hard to dig in", UtmpName);
	  else
#endif /* NETHACK */
          Msg(errno,"Could not write %s", UtmpName);
        }
    }
#ifdef UT_CLOSE
  close(utmpfd);
#endif /* UT_CLOSE */
  D_loginslot = (slot_t) 0;
}



/*
 * Construct a utmp entry for window wi.
 * the hostname field reflects what we know about the user (i.e. display)
 * location. If d_loginhost is not set, then he is local and we write
 * down the name of his terminal line; else he is remote and we keep
 * the hostname here. The letter S and the window id will be appended.
 * A saved utmp entry in wi->w_savut serves as a template, usually.
 */ 

int
SetUtmp(wi)
struct win *wi;
{
  register char *p;
  register slot_t slot;
#ifndef _SEQUENT_
  char *line;
#endif
  struct utmp u;
  int saved_ut;
#ifdef UTHOST
# ifdef _SEQUENT_
  char host[100+5];
# else /* _SEQUENT_ */
  char host[sizeof(D_utmp_logintty.ut_host)+5];
# endif /* _SEQUENT_ */
#endif /* UTHOST */

  wi->w_slot = (slot_t)0;
  if (!utmpok)
    return -1;
  if ((slot = TtyNameSlot(wi->w_tty)) == (slot_t)0)
    {
      debug1("SetUtmp failed (tty %s).\n",wi->w_tty);
      return -1;
    }
  debug2("SetUtmp %d will get slot %d...\n", wi->w_number, (int)slot);
#ifdef UT_CLOSE
  InitUtmp();
#endif /* UT_CLOSE */

  bzero((char *) &u, sizeof(u));
  if ((saved_ut = bcmp((char *) &wi->w_savut, (char *) &u, sizeof(u))))
    /* restore original, of which we will adopt all fields but ut_host */
    bcopy((char *) &wi->w_savut, (char *) &u, sizeof(u));

#ifdef UTHOST
  host[sizeof(host)-5] = '\0';
  if (display)
    {
# ifdef _SEQUENT_
      strncpy(host, D_loginhost, sizeof(host) - 5);
# else /* _SEQUENT */
      strncpy(host, D_utmp_logintty.ut_host, sizeof(host) - 5);
# endif /* _SEQUENT */
      if (D_loginslot != (slot_t)0 && D_loginslot != (slot_t)-1 && host[0] != '\0')
	{
	  /*
	   * we want to set our ut_host field to something like
	   * ":ttyhf:s.0" or
	   * "faui45:s.0" or
	   * "132.199.81.4:s.0" (even this may hurt..), but not
	   * "faui45.informati"......:s.0
	   */
	  for (p = host; *p; p++)
	    if ((*p < '0' || *p > '9') && (*p != '.'))
	      break;
	  if (*p)
	    {
	      for (p = host; *p; p++)
		if (*p == '.')
		  {
		    *p = '\0';
		    break;
		  }
	    }
	}
      else
	{
	  strncpy(host + 1, stripdev(D_usertty), sizeof(host) - 6);
	  host[0] = ':';
	}
    }
  else
    strncpy(host, "local", sizeof(host) - 5);
  sprintf(host + strlen(host), ":S.%c", '0' + wi->w_number);
  debug1("rlogin hostname: '%s'\n", host);
# if !defined(_SEQUENT_) && !defined(sequent)
  strncpy(u.ut_host, host, sizeof(u.ut_host));
# endif
#endif /* UTHOST */

#ifdef _SEQUENT_
  if (ut_add_user(saved_ut ? u.ut_user : LoginName, slot, saved_ut ? u.ut_pid : wi->w_pid, host) == 0)
#else /* _SEQUENT_ */
  if (!saved_ut)
    { /* make new utmp from scratch */
      line = stripdev(wi->w_tty);
# ifdef GETUTENT
      strncpy(u.ut_user, LoginName, sizeof(u.ut_user));
      /* Now the tricky part... guess ut_id */
#  ifdef sgi
      strncpy(u.ut_id, line + 3, sizeof(u.ut_id));
#  else /* sgi */
#   ifdef _IBMR2
      strncpy(u.ut_id, line, sizeof(u.ut_id));
#   else
      strncpy(u.ut_id, line + strlen(line) - 2, sizeof(u.ut_id));
#   endif
#  endif /* sgi */
      strncpy(u.ut_line, line, sizeof(u.ut_line));
      u.ut_pid = wi->w_pid;
      u.ut_type = USER_PROCESS;
      (void) time((time_t *)&u.ut_time);
    } /* !saved_ut {-: */
  setutent();
  if (pututline(&u) == 0)
# else	/* GETUTENT */
      strncpy(u.ut_line, line, sizeof(u.ut_line));
      strncpy(u.ut_name, LoginName, sizeof(u.ut_name));
#  if defined(linux)	/* should have GETUTENT */
      u.ut_type = USER_PROCESS;
      u.ut_pid = wi->w_pid;
      strncpy(u.ut_id, line + 3, sizeof(u.ut_id));
#  endif /* linux */
      (void) time((time_t *)&u.ut_time); /* cast needed for ultrix */
    } /* !saved_ut */
#  ifdef sequent
  /*
   * call sequent undocumented routine to count logins and 
   * add utmp entry if possible 
   */
  if (add_utmp(slot, &u) == -1)
#  else /* sequent */
  (void) lseek(utmpfd, (off_t) (slot * sizeof(u)), 0);
  if (write(utmpfd, (char *) &u, sizeof(u)) != sizeof(u))
#  endif /* sequent */
# endif /* GETUTENT */
#endif /* _SEQUENT_ */

    {
#ifdef NETHACK
      if (nethackflag)
        Msg(errno, "%s is too hard to dig in", UtmpName);
      else
#endif /* NETHACK */
      Msg(errno,"Could not write %s", UtmpName);
#ifdef UT_CLOSE
      close(utmpfd);
#endif /* UT_CLOSE */
      return -1;
    }
  debug("SetUtmp successful\n");
  wi->w_slot = slot;
#ifdef UT_CLOSE
  close(utmpfd);
#endif /* UT_CLOSE */
  bcopy((char *) &u, (char *) &wi->w_savut, sizeof(u));
  return 0;
}



/*
 * if slot could be removed or was 0,  wi->w_slot = -1;
 * else not changed.
 */

int
RemoveUtmp(wi)
struct win *wi;
{
#ifdef GETUTENT
  struct utmp *uu;
#endif /* GETUTENT */
  struct utmp u;
  slot_t slot;

  slot = wi->w_slot;
#ifdef GETUTENT
  debug1("RemoveUtmp(%s)\n", (slot == (slot_t) 0) ?
         "no slot (0)":((slot == (slot_t) -1) ? "no slot (-1)" : slot));
#else /* GETUTENT */
  debug1("RemoveUtmp(wi.slot: %d)\n", slot);
#endif /* GETUTENT */
#ifdef UT_CLOSE
  InitUtmp();
#endif /* UT_CLOSE */
  if (!utmpok)
    return -1;
  if (slot == (slot_t) 0 || slot == (slot_t) -1)
    {
      debug1("There is no utmp-slot to be removed(%d)\n", (int)slot);
      wi->w_slot = (slot_t) -1;
      return 0;
    }
  bzero((char *) &u, sizeof(u));
#ifdef GETUTENT
  setutent();
# ifdef sgi
  bcopy((char *) &wi->w_savut, (char *) &u, sizeof(u));
  uu  = &u;
# else
  strncpy(u.ut_line, slot, sizeof(u.ut_line));
  if ((uu = getutline(&u)) == 0)
    {
      Msg(0, "Utmp slot not found -> not removed");
      return -1;
    }
  bcopy((char *)uu, (char *)&wi->w_savut, sizeof(wi->w_savut));
# endif
# ifdef _SEQUENT_
  if (ut_delete_user(slot, uu->ut_pid, 0, 0) == 0)
# else /* _SEQUENT_ */
  u = *uu;
  u.ut_type = DEAD_PROCESS;
  u.ut_exit.e_termination = 0;
  u.ut_exit.e_exit= 0;
  if (pututline(&u) == 0)
# endif /* _SEQUENT_ */
#else /* GETUTENT */
  (void) lseek(utmpfd, (off_t) (slot * sizeof(u)), 0);
  if (read(utmpfd, (char *) &wi->w_savut, sizeof(u)) != sizeof(u))
    {
      bzero((char *)&wi->w_savut, sizeof(wi->w_savut));
      Msg(errno, "cannot read %s?", UtmpName);
      sleep(1);
    }
# ifdef UT_UNSORTED
  bcopy((char *)&wi->w_savut, (char *)&u, sizeof(u));
  bzero(u.ut_name, sizeof(u.ut_name));
  bzero(u.ut_host, sizeof(u.ut_host));
#  ifdef linux
  u.ut_type = DEAD_PROCESS;
#  endif
# endif /* UT_UNSORTED */
  (void) lseek(utmpfd, (off_t) (slot * sizeof(u)), 0);
  if (write(utmpfd, (char *) &u, sizeof(u)) != sizeof(u))
#endif /* GETUTENT */
    {
#ifdef NETHACK
      if (nethackflag)
        Msg(errno, "%s is too hard to dig in", UtmpName);
      else
#endif /* NETHACK */
      Msg(errno,"Could not write %s", UtmpName);
#ifdef UT_CLOSE
      close(utmpfd);
#endif /* UT_CLOSE */
      return -1;
    }
  debug("RemoveUtmp successfull\n");
  wi->w_slot = (slot_t) -1;
#ifdef UT_CLOSE
  close(utmpfd);
#endif /* UT_CLOSE */
  return 0;
}



/*
 * TtyNameSlot:
 * return an index, where the named tty is found in utmp.
 */

static slot_t 
TtyNameSlot(nam)
char *nam;
{
  char *name;
  register slot_t slot;
#ifdef UT_UNSORTED
  struct utmp u;
#else
# ifndef GETUTENT
  register struct ttyent *tp;
# endif /* GETUTENT */
#endif /* UT_UNSORTED */

  debug1("TtyNameSlot(%s)\n", nam);
#ifdef UT_CLOSE
  InitUtmp();
#endif /* UT_CLOSE */
  if (!utmpok || nam == 0)
    return (slot_t)0;
  name = stripdev(nam);
#ifdef GETUTENT
  slot = name;
#else /* GETUTENT */
# ifdef UT_UNSORTED
  slot = 0;
  (void) lseek(utmpfd, (off_t) 0, 0);
  while ((read(utmpfd, (char *)&u, sizeof(u)) == sizeof(u))
         && (strcmp(u.ut_line, name)))
    slot++;
# else /* UT_UNSORTED*/
  slot = 1;
  setttyent();
  while ((tp = getttyent()) != 0 && strcmp(name, tp->ty_name) != 0)
    slot++;
# endif /* UTNOKEEP */
#endif /* GETUTENT */

#ifdef UT_CLOSE
  close(utmpfd);
#endif
  return slot;
}



#if !defined(GETTTYENT) && !defined(GETUTENT) && !defined(UT_UNSORTED)

/*
 *  Cheap plastic imitation of ttyent routines.
 */

static char *tt, *ttnext;
static char ttys[] = "/etc/ttys";

static void
setttyent()
{
  if (ttnext == 0)
    {
      struct stat s;
      register int f;
      register char *p, *ep;

      if ((f = open(ttys, O_RDONLY)) == -1 || fstat(f, &s) == -1)
	Panic(errno, ttys);
      if ((tt = malloc((unsigned) s.st_size + 1)) == 0)
	Panic(0, strnomem);
      if (read(f, tt, s.st_size) != s.st_size)
	Panic(errno, ttys);
      close(f);
      for (p = tt, ep = p + s.st_size; p < ep; p++)
	if (*p == '\n')
	  *p = '\0';
      *p = '\0';
    }
  ttnext = tt;
}

static struct ttyent *
getttyent()
{
  static struct ttyent t;

  if (*ttnext == '\0')
    return NULL;
  t.ty_name = ttnext + 2;
  ttnext += strlen(ttnext) + 1;
  return &t;
}

#endif	/* !GETTTYENT && !GETUTENT && !UT_UNSORTED*/



#endif /* UTMPOK */




/*********************************************************************
 *
 *  getlogin() replacement (for SVR4 machines)
 */

# if defined(BUGGYGETLOGIN) && defined(UTMP_FILE)
char *
getlogin()
{
  char *tty;
#ifdef utmp
# undef utmp
#endif
  struct utmp u;
  static char retbuf[sizeof(u.ut_user)+1];
  int fd;

  for (fd = 0; fd <= 2 && (tty = ttyname(fd)) == NULL; fd++)
    ;
  if ((tty == NULL) || ((fd = open(UTMP_FILE, O_RDONLY)) < 0))
    return NULL;
  tty = stripdev(tty);
  retbuf[0] = '\0';
  while (read(fd, (char *)&u, sizeof(struct utmp)) == sizeof(struct utmp))
    {
      if (!strncmp(tty, u.ut_line, sizeof(u.ut_line)))
	{
	  strncpy(retbuf, u.ut_user, sizeof(u.ut_user));
	  retbuf[sizeof(u.ut_user)] = '\0';
	  if (u.ut_type == USER_PROCESS)
	    break;
	}
    }
  close(fd);

  return *retbuf ? retbuf : NULL;
}
# endif /* BUGGYGETLOGIN */

