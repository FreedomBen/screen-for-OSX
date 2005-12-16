#!/bin/sh
# sh tty.sh tty.c
# This inserts all the needed #ifdefs for IF{} statements
# and generates tty.c

rm -f $1
sed -e '1,12d' -e 's%^IF{\(.*\)}\(.*\)%#if defined(\1)\
  \2\
#endif /* \1 */%' < $0 > $1
chmod -w $1
exit 0

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

/*
 * NOTICE: tty.c is automatically generated from tty.sh
 * Do not change anything here. If you then change tty.sh.
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#ifndef sun
#include <sys/ioctl.h>
#endif

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#include "config.h"
#include "screen.h"
#include "extern.h"

extern char *ShellArgs[];
extern char screenterm[];
extern char HostName[];
extern int iflag;
extern struct display *display, *displays;

struct NewWindow nwin_undef   = 
{
  -1, (char *)0, (char **)0, (char *)0, (char *)0, -1, -1, 
  -1, -1, (struct pseudowin *)0
};
struct NewWindow nwin_default = 
{ 
  0, 0, ShellArgs, 0, screenterm, 0, 1*FLOW_NOW, 
  LOGINDEFAULT, DEFAULTHISTHEIGHT, (struct pseudowin *)0 
};
struct NewWindow nwin_options;

void
nwin_compose(def, new, res)
struct NewWindow *def, *new, *res;
{
  res->StartAt = new->StartAt != nwin_undef.StartAt ? new->StartAt : def->StartAt;
  res->aka = new->aka != nwin_undef.aka ? new->aka : def->aka;
  res->args = new->args != nwin_undef.args ? new->args : def->args;
  res->dir = new->dir != nwin_undef.dir ? new->dir : def->dir;
  res->term = new->term != nwin_undef.term ? new->term : def->term;
  res->aflag = new->aflag != nwin_undef.aflag ? new->aflag : def->aflag;
  res->flowflag = new->flowflag != nwin_undef.flowflag ? new->flowflag : def->flowflag;
  res->lflag = new->lflag != nwin_undef.lflag ? new->lflag : def->lflag;
  res->histheight = new->histheight != nwin_undef.histheight ? new->histheight : def->histheight;
  res->pwin = new->pwin;
}

int
OpenTTY(line)
char *line;
{
  int f;
  sig_t (*sigalrm)__P(SIGPROTOARG);

  sigalrm = signal(SIGALRM, SIG_IGN);
  alarm(2);
  if ((f = open(line, O_RDWR | O_NDELAY)) == -1)
    {
      Msg(errno, "Cannot open line '%s' for R/W", line);
      alarm(0);
      signal(SIGALRM, sigalrm);
      return -1;
    }
#ifdef I_POP
  debug("OpenTTY I_POP\n");
  while (ioctl(f, I_POP) >= 0)
    ;
#endif
  if (display)
    SetTTY(f, &d_NewMode);
  else
    {
      struct mode Mode;

      InitTTY(&Mode);
#ifdef DEBUG
      DebugTTY(&Mode);
#endif
      SetTTY(f, &Mode);
    }
  brktty(f);
  alarm(0);
  signal(SIGALRM, sigalrm);
  debug2("'%s' CONNECT fd=%d.\n", line, f);
  return f;
}

/* 
 * if it is a PLAIN tty, try to
 * generate a break for n * 0.25 seconds.
 */
void SendBreak(wp, n, closeopen)
struct win *wp;
int n, closeopen;
{
  if (wp->w_t.flags & TTY_FLAG_PLAIN == 0)
    return;

  debug3("break(%d, %d) fd %d\n", n, closeopen, wp->w_ptyfd);
#ifdef POSIX
  (void) tcflush(wp->w_ptyfd, TCIOFLUSH);
#else
# ifdef TIOCFLUSH
  (void) ioctl(wp->w_ptyfd, TIOCFLUSH, (char *)0);
# endif /* TIOCFLUSH */
#endif /* POSIX */
  if (closeopen)
    {
      close(wp->w_ptyfd);
      sleep((n + 3) / 4);
      if ((wp->w_ptyfd = OpenTTY(wp->w_tty)) < 1)
	{
	  Msg(0, "Ouch, cannot reopen line %s, please try harder", wp->w_tty);
	  return;
	}
    }
  else
    {
#ifdef POSIX 
      debug("tcsendbreak\n");
      if (tcsendbreak(wp->w_ptyfd, n) < 0)
	{
	  Msg(errno, "cannot send BREAK");
	  return;
	}
#else
      if (!n)
	n++;
# ifdef TCSBRK
      debug("TCSBRK\n");
	{
	  int i;
	  for (i = 0; i < n; i++)
	    if (ioctl(wp->w_ptyfd, TCSBRK, (char *)0) < 0)
	      {
		Msg(errno, "Cannot send BREAK");
		return;
	      }
	}
# else /* TCSBRK */
#  if defined(TIOCSBRK) && defined(TIOCCBRK)
      debug("TIOCSBRK TIOCCBRK\n");
      if (ioctl(wp->w_ptyfd, TIOCSBRK, (char *)0) < 0)
	{
	  Msg(errno, "Can't send BREAK");
	  return;
	}
      sleep((n + 3) / 4);
      if (ioctl(wp->w_ptyfd, TIOCCBRK, (char *)0) < 0)
	{
	  Msg(errno, "BREAK stuck!!! -- HELP!");
	  return;
	}
#  else /* TIOCSBRK && TIOCCBRK */
      Msg(0, "Break not simulated yet"); 
      return;
#  endif /* TIOCSBRK && TIOCCBRK */
# endif /* TCSBRK */
#endif /* POSIX */
      debug("            broken\n");
    }
}


void
InitTTY(m)
struct mode *m;
{
bzero((char *)m, sizeof(*m));
#ifdef POSIX
  /* struct termios tio 
   * defaults, as seen on SunOS 4.1.1
   */
  debug("InitTTY: POSIX: termios defaults a la SunOS 4.1.1\n");
IF{BRKINT}	m->tio.c_iflag = BRKINT;
IF{IGNPAR}	m->tio.c_iflag |= IGNPAR;
IF{ISTRIP}	m->tio.c_iflag |= ISTRIP;
IF{ICRNL}	m->tio.c_iflag |= ICRNL;
IF{IXON}	m->tio.c_iflag |= IXON;
IF{IMAXBEL}	m->tio.c_iflag |= IMAXBEL; 
IF{ONLCR}	m->tio.c_iflag |= ONLCR; 
IF{TAB3}	m->tio.c_iflag |= TAB3; 
IF{B9600}	m->tio.c_iflag |= B9600; 

IF{OPOST}	m->tio.c_oflag = OPOST;

IF{CS8} 	m->tio.c_cflag = CS8;
IF{CREAD}	m->tio.c_cflag |= CREAD;
IF{PARENB}	m->tio.c_cflag |= PARENB;
IF{ISHIFT) && defined(B9600}	m->tio.c_cflag |= B9600 << ISHIFT;
IF{ECHOCTL}	m->tio.c_lflag |= ECHOCTL;
IF{ECHOKE}	m->tio.c_lflag |= ECHOKE;

IF{ISIG}	m->tio.c_lflag = ISIG;
IF{ICANON}	m->tio.c_lflag |= ICANON;
IF{ECHO}	m->tio.c_lflag |= ECHO;
IF{ECHOE}	m->tio.c_lflag |= ECHOE;
IF{ECHOK}	m->tio.c_lflag |=  ECHOK;
IF{IEXTEN}	m->tio.c_lflag |= IEXTEN;


# if !defined(_SEQUENT_) && !defined(hpux) && !defined(BSDI) && !defined(__386BSD__)
  m->tio.c_line = 0;
# endif /* !_SEQUENT_ && !hpux && !BSDI !__386BSD__ */

IF{VINTR}	m->tio.c_cc[VINTR] = Ctrl('C');
IF{VQUIT}	m->tio.c_cc[VQUIT] = Ctrl('\\');
IF{VERASE}	m->tio.c_cc[VERASE] = 0x7f; /* DEL */
IF{VKILL}	m->tio.c_cc[VKILL] = Ctrl('H');
IF{VEOF}	m->tio.c_cc[VEOF] = Ctrl('D');
IF{VEOL}	m->tio.c_cc[VEOL] = 0;
IF{VEOL2}	m->tio.c_cc[VEOL2] = 0;
IF{VSWTCH}	m->tio.c_cc[VSWTCH] = 0;
IF{VSTART}	m->tio.c_cc[VSTART] = Ctrl('Q');
IF{VSTOP}	m->tio.c_cc[VSTOP] = Ctrl('S');
IF{VSUSP}	m->tio.c_cc[VSUSP] = Ctrl('Z');
IF{VDSUSP}	m->tio.c_cc[VDSUSP] = Ctrl('Y');
IF{VREPRINT}	m->tio.c_cc[VREPRINT] = Ctrl('R');
IF{VDISCARD}	m->tio.c_cc[VDISCARD] = Ctrl('O');
IF{VWERASE}	m->tio.c_cc[VWERASE] = Ctrl('W');
IF{VLNEXT}	m->tio.c_cc[VLNEXT] = Ctrl('V');
IF{VSTATUS}	m->tio.c_cc[VSTATUS] = Ctrl('T');

# ifdef hpux
  /* struct ltchars m_ltchars */
# endif /* hpux */

#else /* POSIX */

# ifdef TERMIO
  debug("InitTTY: nonPOSIX, struct termio a la Motorola SYSV68\n");
  /* struct termio tio 
   * defaults, as seen on Mototola SYSV68:
   * input: 7bit, CR->NL, ^S/^Q flow control 
   * output: POSTprocessing: NL->NL-CR, Tabs to spaces
   * control: 9600baud, 8bit CSIZE, enable input
   * local: enable signals, erase/kill processing, echo on.
   */
IF{ISTRIP}	m->tio.c_iflag = ISTRIP;
IF{ICRNL}	m->tio.c_iflag |= ICRNL;
IF{IXON}	m->tio.c_iflag |= IXON;

IF{OPOST}	m->tio.c_oflag = OPOST;
IF{ONLCR}	m->tio.c_oflag |= ONLCR;
IF{TAB3}	m->tio.c_oflag |= TAB3;

IF{B9600}	m->tio.c_cflag = B9600;
IF{CS8} 	m->tio.c_cflag |= CS8;
IF{CREAD}	m->tio.c_cflag |= CREAD;

IF{ISIG}	m->tio.c_lflag = ISIG;
IF{ICANON}	m->tio.c_lflag |= ICANON;
IF{ECHO}	m->tio.c_lflag |= ECHO;
IF{ECHOE}	m->tio.c_lflag |= ECHOE;
IF{ECHOK}	m->tio.c_lflag |= ECHOK;

IF{VINTR}	m->tio.c_cc[VINTR] = Ctrl('C');
IF{VQUIT}	m->tio.c_cc[VQUIT] = Ctrl('\\');
IF{VERASE}	m->tio.c_cc[VERASE] = 0177; /* DEL */
IF{VKILL}	m->tio.c_cc[VKILL] = Ctrl('H');
IF{VEOF}	m->tio.c_cc[VEOF] = Ctrl('D');
IF{VEOL}	m->tio.c_cc[VEOL] = 0377;
IF{VEOL2}	m->tio.c_cc[VEOL2] = 0377;
IF{VSWTCH}	m->tio.c_cc[VSWTCH] = 000;
# else /* TERMIO */
  debug("InitTTY: nonPOSIX, sgttyb and co. not initialised\n");
  /* struct sgttyb m_ttyb */
  /* struct tchars m_tchars */
  /* struct ltchars m_ltchars */
  /* int m_ldisc */
  /* int m_lmode */
# endif /* TERMIO */
#endif /* POSIX */
}

/* imported section from screen.c */

#if defined(TERMIO) || defined(POSIX)
int intrc, origintrc = VDISABLE;        /* display? */
#else
int intrc, origintrc = -1;        /* display? */
#endif
static startc, stopc;                   /* display? */

void 
SetTTY(fd, mp)
int fd;
struct mode *mp;
{
  errno = 0;
#ifdef POSIX
  tcsetattr(fd, TCSADRAIN, &mp->tio);
# ifdef hpux
  ioctl(fd, TIOCSLTC, &mp->m_ltchars);
# endif
#else
# ifdef TERMIO
  ioctl(fd, TCSETAW, &mp->tio);
# else
  /* ioctl(fd, TIOCSETP, &mp->m_ttyb); */
  ioctl(fd, TIOCSETC, &mp->m_tchars);
  ioctl(fd, TIOCLSET, &mp->m_lmode);
  ioctl(fd, TIOCSETD, &mp->m_ldisc);
  ioctl(fd, TIOCSETP, &mp->m_ttyb);
  ioctl(fd, TIOCSLTC, &mp->m_ltchars); /* moved here for apollo. jw */
# endif
#endif
  if (errno)
    Msg(errno, "SetTTY (fd %d): ioctl failed", fd);
}

void
GetTTY(fd, mp)
int fd;
struct mode *mp;
{
  errno = 0;
#ifdef POSIX
  tcgetattr(fd, &mp->tio);
# ifdef hpux
  ioctl(fd, TIOCGLTC, &mp->m_ltchars);
# endif
#else
# ifdef TERMIO
  ioctl(fd, TCGETA, &mp->tio);
# else
  ioctl(fd, TIOCGETP, &mp->m_ttyb);
  ioctl(fd, TIOCGETC, &mp->m_tchars);
  ioctl(fd, TIOCGLTC, &mp->m_ltchars);
  ioctl(fd, TIOCLGET, &mp->m_lmode);
  ioctl(fd, TIOCGETD, &mp->m_ldisc);
# endif
#endif
  if (errno)
    Msg(errno, "GetTTY (fd %d): ioctl failed", fd);
}

void
SetMode(op, np)
struct mode *op, *np;
{
  *np = *op;

#if defined(TERMIO) || defined(POSIX)
  np->tio.c_iflag &= ~ICRNL;
# ifdef ONLCR
  np->tio.c_oflag &= ~ONLCR;
# endif
  np->tio.c_lflag &= ~(ICANON | ECHO);

  /*
   * Unfortunately, the master process never will get SIGINT if the real
   * terminal is different from the one on which it was originaly started
   * (process group membership has not been restored or the new tty could not
   * be made controlling again). In my solution, it is the attacher who
   * receives SIGINT (because it is always correctly associated with the real
   * tty) and forwards it to the master [kill(MasterPid, SIGINT)]. 
   * Marc Boucher (marc@CAM.ORG)
   */
  if (iflag)
    np->tio.c_lflag |= ISIG;
  else
    np->tio.c_lflag &= ~ISIG;
  /* 
   * careful, careful catche monkey..
   * never set VMIN and VTIME to zero, if you want blocking io.
   */
  np->tio.c_cc[VMIN] = 1;
  np->tio.c_cc[VTIME] = 0;
IF{VSTART}	startc = op->tio.c_cc[VSTART];
IF{VSTOP}	stopc = op->tio.c_cc[VSTOP];
  if (iflag)
    intrc = op->tio.c_cc[VINTR];
  else
    {
      origintrc = op->tio.c_cc[VINTR];
      intrc = np->tio.c_cc[VINTR] = VDISABLE;
    }
  np->tio.c_cc[VQUIT] = VDISABLE;
  if (d_flow == 0)
    {
      np->tio.c_cc[VINTR] = VDISABLE;
IF{VSTART}	np->tio.c_cc[VSTART] = VDISABLE;
IF{VSTOP}	np->tio.c_cc[VSTOP] = VDISABLE;
      np->tio.c_iflag &= ~IXON;
    }
IF{VDISCARD}	np->tio.c_cc[VDISCARD] = VDISABLE;
IF{VSUSP}	np->tio.c_cc[VSUSP] = VDISABLE;
# ifdef hpux
  np->m_ltchars.t_suspc = VDISABLE;
  np->m_ltchars.t_dsuspc = VDISABLE;
  np->m_ltchars.t_flushc = VDISABLE;
  np->m_ltchars.t_lnextc = VDISABLE;
# else /* hpux */
IF{VDSUSP}	np->tio.c_cc[VDSUSP] = VDISABLE;
# endif /* hpux */
#else /* TERMIO || POSIX */
  startc = op->m_tchars.t_startc;
  stopc = op->m_tchars.t_stopc;
  if (iflag)
    {
      origintrc = op->m_tchars.t_intrc;
      intrc = op->m_tchars.t_intrc;
    }
  else
    {
      intrc = np->m_tchars.t_intrc = -1;
    }
  np->m_ttyb.sg_flags &= ~(CRMOD | ECHO);
  np->m_ttyb.sg_flags |= CBREAK;
  np->m_tchars.t_quitc = -1;
  if (d_flow == 0)
    {
      np->m_tchars.t_intrc = -1;
      np->m_tchars.t_startc = -1;
      np->m_tchars.t_stopc = -1;
    }
  np->m_ltchars.t_suspc = -1;
  np->m_ltchars.t_dsuspc = -1;
  np->m_ltchars.t_flushc = -1;
  np->m_ltchars.t_lnextc = -1;
#endif /* defined(TERMIO) || defined(POSIX) */
}

void
SetFlow(on)
int on;
{
  ASSERT(display);
  if (d_flow == on)
    return;
#if defined(TERMIO) || defined(POSIX)
  if (on)
    {
      d_NewMode.tio.c_cc[VINTR] = intrc;
IF{VSTART}	d_NewMode.tio.c_cc[VSTART] = startc;
IF{VSTOP}	d_NewMode.tio.c_cc[VSTOP] = stopc;
      d_NewMode.tio.c_iflag |= IXON;
    }
  else
    {
      d_NewMode.tio.c_cc[VINTR] = VDISABLE;
IF{VSTART}	d_NewMode.tio.c_cc[VSTART] = VDISABLE;
IF{VSTOP}	d_NewMode.tio.c_cc[VSTOP] = VDISABLE;
      d_NewMode.tio.c_iflag &= ~IXON;
    }
# ifdef POSIX
  if (tcsetattr(d_userfd, TCSANOW, &d_NewMode.tio))
# else
  if (ioctl(d_userfd, TCSETAW, &d_NewMode.tio) != 0)
# endif
    debug1("SetFlow: ioctl errno %d\n", errno);
#else /* POSIX || TERMIO */
  if (on)
    {
      d_NewMode.m_tchars.t_intrc = intrc;
      d_NewMode.m_tchars.t_startc = startc;
      d_NewMode.m_tchars.t_stopc = stopc;
    }
  else
    {
      d_NewMode.m_tchars.t_intrc = -1;
      d_NewMode.m_tchars.t_startc = -1;
      d_NewMode.m_tchars.t_stopc = -1;
    }
  if (ioctl(d_userfd, TIOCSETC, &d_NewMode.m_tchars) != 0)
    debug1("SetFlow: ioctl errno %d\n", errno);
#endif /* POSIX || TERMIO */
  d_flow = on;
}

/* end imported section from screen.c */

int
TtyGrabConsole(fd, on, rc_name)
int fd, on;
char *rc_name;
{
#ifdef TIOCCONS
  char *slave;
  int sfd = -1, ret = 0;
  struct display *d;

  if (!on)
    {
      if ((fd = OpenPTY(&slave)) < 0)
	{
	  Msg(errno, "%s: could not open detach pty master", rc_name);
	  return -1;
	}
      if ((sfd = open(slave, O_RDWR)) < 0)
	{
	  Msg(errno, "%s: could not open detach pty slave", rc_name);
	  close(fd);
	  return -1;
	}
    }
  else
    {
      if (displays == 0)
	{
	  Msg(0, "I need a display");
	  return -1;
	}
      for (d = displays; d; d = d->_d_next)
	if (strcmp(d->_d_usertty, "/dev/console") == 0)
	  break;
      if (d)
	{
	  Msg(0, "too dangerous - screen is running on /dev/console");
	  return -1;
	}
    }
  if (UserContext() == 1)
    UserReturn(ioctl(fd, TIOCCONS, &on));
  ret = UserStatus();
  if (ret)
    Msg(errno, "%s: ioctl TIOCCONS failed", rc_name);
  if (!on)
    {
      close(sfd);
      close(fd);
    }
  return ret;
#else /* TIOCCONS */
  Msg(0, "%s: no TIOCCONS on this machine", rc_name);
  return -1;
#endif /* TIOCCONS */
}

#ifdef DEBUG
void
DebugTTY(m)
struct mode *m;
{
  int i;

#ifdef POSIX
  debug("struct termios tio:\n");
  debug1("c_iflag = %#x\n", m->tio.c_iflag);
  debug1("c_oflag = %#x\n", m->tio.c_oflag);
  debug1("c_cflag = %#x\n", m->tio.c_cflag);
  debug1("c_lflag = %#x\n", m->tio.c_lflag);
# if !defined(_SEQUENT_) && !defined(hpux)
  debug1("c_line = %#x\n", m->tio.c_line);
# endif /* !_SEQUENT_ && !hpux */
  for (i = 0; i < NCCS; i++)
    {
      debug2("c_cc[%d] = %#x\n", i, m->tio.c_cc[i]);
    }
# ifdef hpux
  /* struct ltchars m_ltchars */
# endif /* hpux */
#else /* POSIX */
# ifdef TERMIO
  debug("struct termio tio:\n");
  debug1("c_iflag = %04o\n", m->tio.c_iflag);
  debug1("c_oflag = %04o\n", m->tio.c_oflag);
  debug1("c_cflag = %04o\n", m->tio.c_cflag);
  debug1("c_lflag = %04o\n", m->tio.c_lflag);
  for (i = 0; i < NCC; i++)
    {
      debug2("c_cc[%d] = %04o\n", i, m->tio.c_cc[i]);
    }
# else /* TERMIO */
  debug("DebugTty: sgttyb? please implement this\n");
  /* struct sgttyb m_ttyb */
  /* struct tchars m_tchars */
  /* struct ltchars m_ltchars */
  /* int m_ldisc */
  /* int m_lmode */
# endif /* TERMIO */
#endif /* POSIX */
}
#endif /* DEBUG */
