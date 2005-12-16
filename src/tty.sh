#! /bin/sh
# sh tty.sh tty.c
# This inserts all the needed #ifdefs for IF{} statements
# and generates tty.c

rm -f $1
sed -e '1,17d' \
-e 's%^IF{\(.*\)}\(.*\)%#if defined(\1)\
  \2\
#endif /* \1 */%' \
-e 's%^XIF{\(.*\)}\(.*\)%#if defined(\1) \&\& \1 < MAXCC\
  \2\
#endif /* \1 */%' \
 < $0 > $1
chmod -w $1
exit 0

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

/*
 * NOTICE: tty.c is automatically generated from tty.sh
 * Do not change anything here. If you then change tty.sh.
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#ifndef sgi
# include <sys/file.h>
#endif
#if !defined(sun) || defined(SUNOS3)
# include <sys/ioctl.h> /* collosions with termios.h */
#else
# ifndef TIOCEXCL
#  include <sys/ttold.h>	/* needed for TIOCEXCL */
# endif
#endif

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#include "config.h"
#include "screen.h"
#include "extern.h"


extern struct display *display, *displays;
extern int iflag;

/* Frank Schulz (fschulz@pyramid.com):
 * I have no idea why VSTART is not defined and my fix is probably not
 * the cleanest, but it works.
 */
#if !defined(VSTART) && defined(_VSTART)
#define VSTART _VSTART
#endif
#if !defined(VSTOP) && defined(_VSTOP)
#define VSTOP _VSTOP
#endif

static sigret_t
SigAlrmDummy SIGDEFARG
{
  debug("SigAlrmDummy()\n");
  SIGRETURN;
}

/*
 *  Carefully open a charcter device. Not used to open ttys.
 */

int
OpenTTY(line)
char *line;
{
  int f;
  sigret_t (*sigalrm)__P(SIGPROTOARG);

  sigalrm = signal(SIGALRM, SigAlrmDummy);
  alarm(2);
  /* this open only succeeds, if real uid is allowed */
  if ((f = secopen(line, O_RDWR | O_NONBLOCK, 0)) == -1)
    {
      Msg(errno, "Cannot open line '%s' for R/W", line);
      alarm(0);
      signal(SIGALRM, sigalrm);
      return -1;
    }
  if (!isatty(f))
    {
      Msg(0, "'%s' is not a tty", line);
      alarm(0);
      signal(SIGALRM, sigalrm);
      return -1;
    }
#ifdef I_POP
  debug("OpenTTY I_POP\n");
  while (ioctl(f, I_POP, (char *)0) >= 0)
    ;
#endif
  /*
   * We come here exclusively. This is to stop all kermit and cu type things
   * accessing the same tty line.
   * Perhaps we should better create a lock in some /usr/spool/locks directory?
   */
#ifdef TIOCEXCL
 errno = 0;
 if (ioctl(f, TIOCEXCL, (char *) 0) < 0)
   Msg(errno, "%s: ioctl TIOCEXCL failed", line);
 debug3("%d %d %d\n", (int)getuid(), (int)geteuid(), (int)getpid());
 debug2("%s TIOCEXCL errno %d\n", line, errno);
#endif  /* TIOCEXCL */
  /*
   * We create a sane tty mode. We do not copy things from the display tty
   */
#if WE_REALLY_WANT_TO_COPY_THE_TTY_MODE
  if (display)
    {
      debug1("OpenTTY: using mode of display for %s\n", line);
      SetTTY(f, &D_NewMode);
#ifdef DEBUG
      DebugTTY(&D_NewMode);
#endif
    }
  else
#endif 
    {
      struct mode Mode;

      InitTTY(&Mode, TTY_TYPE_PLAIN);
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
 *  Tty mode handling
 */

#if defined(TERMIO) || defined(POSIX)
int intrc, origintrc = VDISABLE;        /* display? */
#else
int intrc, origintrc = -1;		/* display? */
#endif
static int startc, stopc;		/* display? */


void
InitTTY(m, ttyflag)
struct mode *m;
int ttyflag;
{
  bzero((char *)m, sizeof(*m));
#ifdef POSIX
  /* struct termios tio 
   * defaults, as seen on SunOS 4.1.3
   */
  debug1("InitTTY: POSIX: termios defaults a la SunOS 4.1.3 (%d)\n", ttyflag);
IF{BRKINT}	m->tio.c_iflag |= BRKINT;
IF{IGNPAR}	m->tio.c_iflag |= IGNPAR;
IF{ISTRIP}	m->tio.c_iflag |= ISTRIP;
IF{IXON}	m->tio.c_iflag |= IXON;
/* IF{IMAXBEL}	m->tio.c_iflag |= IMAXBEL; */

  if (!ttyflag)	/* may not even be good for ptys.. */
    {
IF{ICRNL}	m->tio.c_iflag |= ICRNL;
IF{ONLCR}	m->tio.c_oflag |= ONLCR; 
IF{TAB3}	m->tio.c_oflag |= TAB3; 
IF{PARENB}	m->tio.c_cflag |= PARENB;
    }

IF{OPOST}	m->tio.c_oflag |= OPOST;

IF{B9600}	m->tio.c_cflag |= B9600;
IF{CS8} 	m->tio.c_cflag |= CS8;
IF{CREAD}	m->tio.c_cflag |= CREAD;
IF{IBSHIFT) && defined(B9600}	m->tio.c_cflag |= B9600 << IBSHIFT;
/* IF{CLOCAL}	m->tio.c_cflag |= CLOCAL; */

IF{ECHOCTL}	m->tio.c_lflag |= ECHOCTL;
IF{ECHOKE}	m->tio.c_lflag |= ECHOKE;

  if (!ttyflag)
    {
IF{ISIG}	m->tio.c_lflag |= ISIG;
IF{ICANON}	m->tio.c_lflag |= ICANON;
IF{ECHO}	m->tio.c_lflag |= ECHO;
    }
IF{ECHOE}	m->tio.c_lflag |= ECHOE;
IF{ECHOK}	m->tio.c_lflag |= ECHOK;
IF{IEXTEN}	m->tio.c_lflag |= IEXTEN;

XIF{VINTR}	m->tio.c_cc[VINTR]    = Ctrl('C');
XIF{VQUIT}	m->tio.c_cc[VQUIT]    = Ctrl('\\');
XIF{VERASE}	m->tio.c_cc[VERASE]   = 0x7f; /* DEL */
XIF{VKILL}	m->tio.c_cc[VKILL]    = Ctrl('H');
XIF{VEOF}	m->tio.c_cc[VEOF]     = Ctrl('D');
XIF{VEOL}	m->tio.c_cc[VEOL]     = 0000;
XIF{VEOL2}	m->tio.c_cc[VEOL2]    = 0000;
XIF{VSWTCH}	m->tio.c_cc[VSWTCH]   = 0000;
XIF{VSTART}	m->tio.c_cc[VSTART]   = Ctrl('Q');
XIF{VSTOP}	m->tio.c_cc[VSTOP]    = Ctrl('S');
XIF{VSUSP}	m->tio.c_cc[VSUSP]    = Ctrl('Z');
XIF{VDSUSP}	m->tio.c_cc[VDSUSP]   = Ctrl('Y');
XIF{VREPRINT}	m->tio.c_cc[VREPRINT] = Ctrl('R');
XIF{VDISCARD}	m->tio.c_cc[VDISCARD] = Ctrl('O');
XIF{VWERASE}	m->tio.c_cc[VWERASE]  = Ctrl('W');
XIF{VLNEXT}	m->tio.c_cc[VLNEXT]   = Ctrl('V');
XIF{VSTATUS}	m->tio.c_cc[VSTATUS]  = Ctrl('T');

  if (ttyflag)
    {
      m->tio.c_cc[VMIN] = 1;
      m->tio.c_cc[VTIME] = 0;
    }
# ifdef HPUX_LTCHARS_HACK
  m->m_ltchars.t_suspc =  Ctrl('Z');
  m->m_ltchars.t_dsuspc = Ctrl('Y');
  m->m_ltchars.t_rprntc = Ctrl('R');
  m->m_ltchars.t_flushc = Ctrl('O');
  m->m_ltchars.t_werasc = Ctrl('W');
  m->m_ltchars.t_lnextc = Ctrl('V');
# endif /* HPUX_LTCHARS_HACK */

#else /* POSIX */

# ifdef TERMIO
  debug1("InitTTY: nonPOSIX, struct termio a la Motorola SYSV68 (%d)\n", ttyflag);
  /* struct termio tio 
   * defaults, as seen on Mototola SYSV68:
   * input: 7bit, CR->NL, ^S/^Q flow control 
   * output: POSTprocessing: NL->NL-CR, Tabs to spaces
   * control: 9600baud, 8bit CSIZE, enable input
   * local: enable signals, erase/kill processing, echo on.
   */
IF{ISTRIP}	m->tio.c_iflag |= ISTRIP;
IF{IXON}	m->tio.c_iflag |= IXON;

IF{OPOST}	m->tio.c_oflag |= OPOST;

  if (!ttyflag)	/* may not even be good for ptys.. */
    {
IF{ICRNL}	m->tio.c_iflag |= ICRNL;
IF{ONLCR}	m->tio.c_oflag |= ONLCR;
IF{TAB3}	m->tio.c_oflag |= TAB3;
    }

IF{B9600}	m->tio.c_cflag  = B9600;
IF{CS8} 	m->tio.c_cflag |= CS8;
IF{CREAD}	m->tio.c_cflag |= CREAD;

  if (!ttyflag)
    {
IF{ISIG}	m->tio.c_lflag |= ISIG;
IF{ICANON}	m->tio.c_lflag |= ICANON;
IF{ECHO}	m->tio.c_lflag |= ECHO;
    }
IF{ECHOE}	m->tio.c_lflag |= ECHOE;
IF{ECHOK}	m->tio.c_lflag |= ECHOK;

XIF{VINTR}	m->tio.c_cc[VINTR]  = Ctrl('C');
XIF{VQUIT}	m->tio.c_cc[VQUIT]  = Ctrl('\\');
XIF{VERASE}	m->tio.c_cc[VERASE] = 0177; /* DEL */
XIF{VKILL}	m->tio.c_cc[VKILL]  = Ctrl('H');
XIF{VEOF}	m->tio.c_cc[VEOF]   = Ctrl('D');
XIF{VEOL}	m->tio.c_cc[VEOL]   = 0377;
XIF{VEOL2}	m->tio.c_cc[VEOL2]  = 0377;
XIF{VSWTCH}	m->tio.c_cc[VSWTCH] = 0000;
  if (ttyflag)
    {
      m->tio.c_cc[VMIN] = 1;
      m->tio.c_cc[VTIME] = 0;
    }
# else /* TERMIO */
  debug1("InitTTY: BSD: defaults a la SunOS 4.1.3 (%d)\n", ttyflag);
  m->m_ttyb.sg_ispeed = B9600;
  m->m_ttyb.sg_ospeed = B9600;
  m->m_ttyb.sg_erase  = 0177; /*DEL */
  m->m_ttyb.sg_kill   = Ctrl('H');
  if (!ttyflag)
    m->m_ttyb.sg_flags = CRMOD | ECHO
IF{ANYP}	| ANYP
    ;
  else
    m->m_ttyb.sg_flags = CBREAK
IF{ANYP}	| ANYP
    ;

  m->m_tchars.t_intrc   = Ctrl('C');
  m->m_tchars.t_quitc   = Ctrl('\\');
  m->m_tchars.t_startc  = Ctrl('Q');
  m->m_tchars.t_stopc   = Ctrl('S');
  m->m_tchars.t_eofc    = Ctrl('D');
  m->m_tchars.t_brkc    = -1;

  m->m_ltchars.t_suspc  = Ctrl('Z');
  m->m_ltchars.t_dsuspc = Ctrl('Y');
  m->m_ltchars.t_rprntc = Ctrl('R');
  m->m_ltchars.t_flushc = Ctrl('O');
  m->m_ltchars.t_werasc = Ctrl('W');
  m->m_ltchars.t_lnextc = Ctrl('V');

IF{NTTYDISC}	m->m_ldisc = NTTYDISC;

  m->m_lmode = 0
IF{LDECCTQ}	| LDECCTQ
IF{LCTLECH}	| LCTLECH
IF{LPASS8}	| LPASS8
IF{LCRTKIL}	| LCRTKIL
IF{LCRTERA}	| LCRTERA
IF{LCRTBS}	| LCRTBS
;
# endif /* TERMIO */
#endif /* POSIX */

#if defined(KANJI) && defined(TIOCKSET) && defined(KM_ASCII) && defined(KM_SYSSJIS)
  m->m_jtchars.t_ascii = 'J';
  m->m_jtchars.t_kanji = 'B';
  m->m_knjmode = KM_ASCII | KM_SYSSJIS;
#endif
}

void 
SetTTY(fd, mp)
int fd;
struct mode *mp;
{
  errno = 0;
#ifdef POSIX
  tcsetattr(fd, TCSADRAIN, &mp->tio);
# ifdef HPUX_LTCHARS_HACK
  ioctl(fd, TIOCSLTC, (char *)&mp->m_ltchars);
# endif
#else
# ifdef TERMIO
  ioctl(fd, TCSETAW, (char *)&mp->tio);
#  ifdef CYTERMIO
  if (mp->tio.c_line == 3)
    {
      ioctl(fd, LDSETMAPKEY, (char *)&mp->m_mapkey);
      ioctl(fd, LDSETMAPSCREEN, (char *)&mp->m_mapscreen);
      ioctl(fd, LDSETBACKSPACE, (char *)&mp->m_backspace);
    }
#  endif
# else
  /* ioctl(fd, TIOCSETP, (char *)&mp->m_ttyb); */
  ioctl(fd, TIOCSETC, (char *)&mp->m_tchars);
  ioctl(fd, TIOCLSET, (char *)&mp->m_lmode);
  ioctl(fd, TIOCSETD, (char *)&mp->m_ldisc);
  ioctl(fd, TIOCSETP, (char *)&mp->m_ttyb);
  ioctl(fd, TIOCSLTC, (char *)&mp->m_ltchars); /* moved here for apollo. jw */
# endif
#endif
#if defined(KANJI) && defined(TIOCKSET) && defined(KM_ASCII) && defined(KM_SYSSJIS)
  ioctl(fd, TIOCKSETC, &mp->m_jtchars);
  ioctl(fd, TIOCKSET, &mp->m_knjmode);
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
# ifdef HPUX_LTCHARS_HACK
  ioctl(fd, TIOCGLTC, (char *)&mp->m_ltchars);
# endif
#else
# ifdef TERMIO
  ioctl(fd, TCGETA, (char *)&mp->tio);
#  ifdef CYTERMIO
  if (mp->tio.c_line == 3)
    {
      ioctl(fd, LDGETMAPKEY, (char *)&mp->m_mapkey);
      ioctl(fd, LDGETMAPSCREEN, (char *)&mp->m_mapscreen);
      ioctl(fd, LDGETBACKSPACE, (char *)&mp->m_backspace);
    }
  else
    {
      mp->m_mapkey = NOMAPKEY;
      mp->m_mapscreen = NOMAPSCREEN;
      mp->m_backspace = '\b';
    }
#  endif
# else
  ioctl(fd, TIOCGETP, (char *)&mp->m_ttyb);
  ioctl(fd, TIOCGETC, (char *)&mp->m_tchars);
  ioctl(fd, TIOCGLTC, (char *)&mp->m_ltchars);
  ioctl(fd, TIOCLGET, (char *)&mp->m_lmode);
  ioctl(fd, TIOCGETD, (char *)&mp->m_ldisc);
# endif
#endif
#if defined(KANJI) && defined(TIOCKSET) && defined(KM_ASCII) && defined(KM_SYSSJIS)
  ioctl(fd, TIOCKGETC, &mp->m_jtchars);
  ioctl(fd, TIOCKGET, &mp->m_knjmode);
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
# ifdef CYTERMIO
  np->m_mapkey = NOMAPKEY;
  np->m_mapscreen = NOMAPSCREEN;
  np->tio.c_line = 0;
# endif
IF{ICRNL}  np->tio.c_iflag &= ~ICRNL;
IF{ISTRIP}  np->tio.c_iflag &= ~ISTRIP;
IF{ONLCR}  np->tio.c_oflag &= ~ONLCR;
  np->tio.c_lflag &= ~(ICANON | ECHO);
  /*
   * From Andrew Myers (andru@tonic.lcs.mit.edu)
   */
IF{IEXTEN}  np->tio.c_lflag &= ~IEXTEN;

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
XIF{VSTART}	startc = op->tio.c_cc[VSTART];
XIF{VSTOP}	stopc = op->tio.c_cc[VSTOP];
  if (iflag)
    origintrc = intrc = op->tio.c_cc[VINTR];
  else
    {
      origintrc = op->tio.c_cc[VINTR];
      intrc = np->tio.c_cc[VINTR] = VDISABLE;
    }
  np->tio.c_cc[VQUIT] = VDISABLE;
  if (D_flow == 0)
    {
      np->tio.c_cc[VINTR] = VDISABLE;
XIF{VSTART}	np->tio.c_cc[VSTART] = VDISABLE;
XIF{VSTOP}	np->tio.c_cc[VSTOP] = VDISABLE;
      np->tio.c_iflag &= ~IXON;
    }
XIF{VDISCARD}	np->tio.c_cc[VDISCARD] = VDISABLE;
XIF{VLNEXT}	np->tio.c_cc[VLNEXT] = VDISABLE;
XIF{VSTATUS}	np->tio.c_cc[VSTATUS] = VDISABLE;
XIF{VSUSP}	np->tio.c_cc[VSUSP] = VDISABLE;
XIF{VERASE}	np->tio.c_cc[VERASE] = VDISABLE;
XIF{VKILL}	np->tio.c_cc[VKILL] = VDISABLE;
# ifdef HPUX_LTCHARS_HACK
  np->m_ltchars.t_suspc  = VDISABLE;
  np->m_ltchars.t_dsuspc = VDISABLE;
  np->m_ltchars.t_rprntc = VDISABLE;
  np->m_ltchars.t_flushc = VDISABLE;
  np->m_ltchars.t_werasc = VDISABLE;
  np->m_ltchars.t_lnextc = VDISABLE;
# else /* HPUX_LTCHARS_HACK */
XIF{VDSUSP}	np->tio.c_cc[VDSUSP] = VDISABLE;
XIF{VREPRINT}	np->tio.c_cc[VREPRINT] = VDISABLE;
XIF{VWERASE}	np->tio.c_cc[VWERASE] = VDISABLE;
# endif /* HPUX_LTCHARS_HACK */
#else /* TERMIO || POSIX */
  startc = op->m_tchars.t_startc;
  stopc = op->m_tchars.t_stopc;
  if (iflag)
    origintrc = intrc = op->m_tchars.t_intrc;
  else
    {
      origintrc = op->m_tchars.t_intrc;
      intrc = np->m_tchars.t_intrc = -1;
    }
  np->m_ttyb.sg_flags &= ~(CRMOD | ECHO);
  np->m_ttyb.sg_flags |= CBREAK;
# if defined(CYRILL) && defined(CSTYLE) && defined(CS_8BITS)
  np->m_ttyb.sg_flags &= ~CSTYLE;
  np->m_ttyb.sg_flags |= CS_8BITS;
# endif
  np->m_tchars.t_quitc = -1;
  if (D_flow == 0)
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
  if (D_flow == on)
    return;
#if defined(TERMIO) || defined(POSIX)
  if (on)
    {
      D_NewMode.tio.c_cc[VINTR] = intrc;
XIF{VSTART}	D_NewMode.tio.c_cc[VSTART] = startc;
XIF{VSTOP}	D_NewMode.tio.c_cc[VSTOP] = stopc;
      D_NewMode.tio.c_iflag |= IXON;
    }
  else
    {
      D_NewMode.tio.c_cc[VINTR] = VDISABLE;
XIF{VSTART}	D_NewMode.tio.c_cc[VSTART] = VDISABLE;
XIF{VSTOP}	D_NewMode.tio.c_cc[VSTOP] = VDISABLE;
      D_NewMode.tio.c_iflag &= ~IXON;
    }
# ifdef POSIX
  if (tcsetattr(D_userfd, TCSANOW, &D_NewMode.tio))
# else
  if (ioctl(D_userfd, TCSETAW, (char *)&D_NewMode.tio) != 0)
# endif
    debug1("SetFlow: ioctl errno %d\n", errno);
#else /* POSIX || TERMIO */
  if (on)
    {
      D_NewMode.m_tchars.t_intrc = intrc;
      D_NewMode.m_tchars.t_startc = startc;
      D_NewMode.m_tchars.t_stopc = stopc;
    }
  else
    {
      D_NewMode.m_tchars.t_intrc = -1;
      D_NewMode.m_tchars.t_startc = -1;
      D_NewMode.m_tchars.t_stopc = -1;
    }
  if (ioctl(D_userfd, TIOCSETC, (char *)&D_NewMode.m_tchars) != 0)
    debug1("SetFlow: ioctl errno %d\n", errno);
#endif /* POSIX || TERMIO */
  D_flow = on;
}


/*
 *  Job control handling
 *
 *  Somehow the ultrix session handling is broken, so use
 *  the bsdish variant.
 */

/*ARGSUSED*/
void
brktty(fd)
int fd;
{
#if defined(POSIX) && !defined(ultrix)
  setsid();		/* will break terminal affiliation */
# if defined(BSD) && defined(TIOCSCTTY)
  ioctl(fd, TIOCSCTTY, (char *)0);
# endif /* BSD && TIOCSCTTY */
#else /* POSIX */
# ifdef SYSV
  setpgrp();		/* will break terminal affiliation */
# else /* SYSV */
#  ifdef BSDJOBS
  int devtty;

  if ((devtty = open("/dev/tty", O_RDWR | O_NONBLOCK)) >= 0)
    {
      if (ioctl(devtty, TIOCNOTTY, (char *)0))
        debug2("brktty: ioctl(devtty=%d, TIOCNOTTY, 0) = %d\n", devtty, errno);
      close(devtty);
    }
#  endif /* BSDJOBS */
# endif /* SYSV */
#endif /* POSIX */
}

int
fgtty(fd)
int fd;
{
#ifdef BSDJOBS
  int mypid;

  mypid = getpid();

  /* The next lines should be obsolete. Can anybody check if they
   * are really needed on the BSD platforms?
   */
# if defined(__osf__) || (BSD >= 199103) || defined(ISC)
  setsid();	/* should be already done */
#  ifdef TIOCSCTTY
  ioctl(fd, TIOCSCTTY, (char *)0);
#  endif
# endif

# ifdef POSIX
  if (tcsetpgrp(fd, mypid))
    {
      debug1("fgtty: tcsetpgrp: %d\n", errno);
      return -1;
    }
# else /* POSIX */
  if (ioctl(fd, TIOCSPGRP, (char *)&mypid) != 0)
    debug1("fgtty: TIOSETPGRP: %d\n", errno);
#  ifndef SYSV	/* Already done in brktty():setpgrp() */
  if (setpgrp(fd, mypid))
    debug1("fgtty: setpgrp: %d\n", errno);
#  endif
# endif /* POSIX */
#endif /* BSDJOBS */
  return 0;
}


/* 
 * Send a break for n * 0.25 seconds. Tty must be PLAIN.
 */

void SendBreak(wp, n, closeopen)
struct win *wp;
int n, closeopen;
{
  if (wp->w_type != TTY_TYPE_PLAIN)
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
      (void) fcntl(wp->w_ptyfd, F_SETFL, FNBLOCK);
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


/*
 *  Console grabbing
 */

/*ARGSUSED*/
int
TtyGrabConsole(fd, on, rc_name)
int fd, on;
char *rc_name;
{
#ifdef TIOCCONS
  struct display *d;
  int ret = 0;
  int sfd = -1;

  if (on)
    {
      if (displays == 0)
	{
	  Msg(0, "I need a display");
	  return -1;
	}
      for (d = displays; d; d = d->d_next)
	if (strcmp(d->d_usertty, "/dev/console") == 0)
	  break;
      if (d)
	{
	  Msg(0, "too dangerous - screen is running on /dev/console");
	  return -1;
	}
    }

  if (!on)
    {
      char *slave;
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
  if (UserContext() == 1)
    UserReturn(ioctl(fd, TIOCCONS, (char *)&on));
  ret = UserStatus();
  if (ret)
    Msg(errno, "%s: ioctl TIOCCONS failed", rc_name);
  if (!on)
    {
      close(sfd);
      close(fd);
    }
  return ret;

#else
  Msg(0, "%s: don't know how to grab the console", rc_name);
  return -1;
#endif
}


/*
 *  Write out the mode struct in a readable form
 */

#ifdef DEBUG
void
DebugTTY(m)
struct mode *m;
{
  int i;

#ifdef POSIX
  debug("struct termios tio:\n");
  debug1("c_iflag = %#x\n", (unsigned int)m->tio.c_iflag);
  debug1("c_oflag = %#x\n", (unsigned int)m->tio.c_oflag);
  debug1("c_cflag = %#x\n", (unsigned int)m->tio.c_cflag);
  debug1("c_lflag = %#x\n", (unsigned int)m->tio.c_lflag);
  for (i = 0; i < sizeof(m->tio.c_cc)/sizeof(*m->tio.c_cc); i++)
    {
      debug2("c_cc[%d] = %#x\n", i, m->tio.c_cc[i]);
    }
# ifdef HPUX_LTCHARS_HACK
  debug1("suspc     = %#02x\n", m->m_ltchars.t_suspc);
  debug1("dsuspc    = %#02x\n", m->m_ltchars.t_dsuspc);
  debug1("rprntc    = %#02x\n", m->m_ltchars.t_rprntc);
  debug1("flushc    = %#02x\n", m->m_ltchars.t_flushc);
  debug1("werasc    = %#02x\n", m->m_ltchars.t_werasc);
  debug1("lnextc    = %#02x\n", m->m_ltchars.t_lnextc);
# endif /* HPUX_LTCHARS_HACK */
#else /* POSIX */
# ifdef TERMIO
  debug("struct termio tio:\n");
  debug1("c_iflag = %04o\n", m->tio.c_iflag);
  debug1("c_oflag = %04o\n", m->tio.c_oflag);
  debug1("c_cflag = %04o\n", m->tio.c_cflag);
  debug1("c_lflag = %04o\n", m->tio.c_lflag);
  for (i = 0; i < sizeof(m->tio.c_cc)/sizeof(*m->tio.c_cc); i++)
    {
      debug2("c_cc[%d] = %04o\n", i, m->tio.c_cc[i]);
    }
# else /* TERMIO */
  debug1("sg_ispeed = %d\n",    m->m_ttyb.sg_ispeed);
  debug1("sg_ospeed = %d\n",    m->m_ttyb.sg_ospeed);
  debug1("sg_erase  = %#02x\n", m->m_ttyb.sg_erase);
  debug1("sg_kill   = %#02x\n", m->m_ttyb.sg_kill);
  debug1("sg_flags  = %#04x\n", (unsigned short)m->m_ttyb.sg_flags);
  debug1("intrc     = %#02x\n", m->m_tchars.t_intrc);
  debug1("quitc     = %#02x\n", m->m_tchars.t_quitc);
  debug1("startc    = %#02x\n", m->m_tchars.t_startc);
  debug1("stopc     = %#02x\n", m->m_tchars.t_stopc);
  debug1("eofc      = %#02x\n", m->m_tchars.t_eofc);
  debug1("brkc      = %#02x\n", m->m_tchars.t_brkc);
  debug1("suspc     = %#02x\n", m->m_ltchars.t_suspc);
  debug1("dsuspc    = %#02x\n", m->m_ltchars.t_dsuspc);
  debug1("rprntc    = %#02x\n", m->m_ltchars.t_rprntc);
  debug1("flushc    = %#02x\n", m->m_ltchars.t_flushc);
  debug1("werasc    = %#02x\n", m->m_ltchars.t_werasc);
  debug1("lnextc    = %#02x\n", m->m_ltchars.t_lnextc);
  debug1("ldisc     = %d\n",    m->m_ldisc);
  debug1("lmode     = %#x\n",   m->m_lmode);
# endif /* TERMIO */
#endif /* POSIX */
}
#endif /* DEBUG */
