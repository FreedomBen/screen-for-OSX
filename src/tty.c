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

#ifndef lint
  static char rcs_id[] = "$Id$ FAU";
#endif


#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#ifndef sun
#include <sys/ioctl.h>
#endif
#ifdef BSDI
# include <string.h>
#endif /* BSDI */

#ifdef ISC
# include <sys/tty.h>
# include <sys/sioctl.h>
# include <sys/pty.h>
#endif

#ifdef MIPS
extern int errno;
#endif

#include "config.h"
#include "screen.h"
#include "extern.h"

void DebugTTY();

void
InitTTY(m)
struct mode *m;
{
  int i;

#ifdef POSIX
  /* struct termios tio 
   * defaults, as seen on SunOS 4.1.1
   */
  m->tio.c_iflag = BRKINT | IGNPAR | ISTRIP | ICRNL | IXON;
  m->tio.c_oflag = OPOST;
  m->tio.c_cflag = CS8 | CREAD | PARENB ;
  m->tio.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN;
# ifndef _POSIX_SOURCE
  m->tio.c_iflag |= IMAXBEL; 
  m->tio.c_oflag |= ONLCR | TAB3;
#  ifdef B9600
  m->tio.c_cflag |= B9600 | (B9600 << IBSHIFT);
#  endif /* B9600 */
  m->tio.c_lflag |= ECHOCTL | ECHOKE;
# endif /* _POSIX_SOURCE */
  m->tio.c_line = 0;
  m->tio.c_cc[VINTR] = Ctrl('C');
  m->tio.c_cc[VQUIT] = Ctrl('\\');
  m->tio.c_cc[VERASE] = 0x7f; /* DEL */
  m->tio.c_cc[VKILL] = Ctrl('H');
  m->tio.c_cc[VEOF] = Ctrl('D');
  m->tio.c_cc[VEOL] = 0;
# ifndef _POSIX_SOURCE
  m->tio.c_cc[VEOL2] = 0;
  m->tio.c_cc[VSWTCH] = 0;
# endif /* _POSIX_SOURCE */
  m->tio.c_cc[VSTART] = Ctrl('Q');
  m->tio.c_cc[VSTOP] = Ctrl('S');
  m->tio.c_cc[VSUSP] = Ctrl('Z');
# ifndef _POSIX_SOURCE
  m->tio.c_cc[VDSUSP] = Ctrl('Y');
  m->tio.c_cc[VREPRINT] = Ctrl('R');
  m->tio.c_cc[VDISCARD] = Ctrl('O');
  m->tio.c_cc[VWERASE] = Ctrl('W');
  m->tio.c_cc[VLNEXT] = Ctrl('V');
  m->tio.c_cc[VSTATUS] = Ctrl('T');
# endif /* _POSIX_SOURCE */
# ifdef hpux
  /* struct ltchars m_ltchars */
# endif /* hpux */
#else /* POSIX */
# ifdef TERMIO
  /* struct termio tio 
   * defaults, as seen on Mototola SYSV68:
   * input: 7bit, CR->NL, ^S/^Q flow control 
   * output: POSTprocessing: NL->NL-CR, Tabs to spaces
   * control: 9600baud, 8bit CSIZE, enable input
   * local: enable signals, erase/kill processing, echo on.
   */
  m->tio.c_iflag = ISTRIP | ICRNL | IXON;
  m->tio.c_oflag = OPOST | ONLCR | TAB3;
  m->tio.c_cflag = B9600 | CS8 | CREAD;
  m->tio.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK;
  m->tio.c_cc[VINTR] = Ctrl('C');
  m->tio.c_cc[VQUIT] = Ctrl('\\');
  m->tio.c_cc[VERASE] = 0177; /* DEL */
  m->tio.c_cc[VKILL] = Ctrl('H');
  m->tio.c_cc[VEOF] = Ctrl('D');
  m->tio.c_cc[VEOL] = 0377;
  m->tio.c_cc[VEOL2] = 0377;
#ifdef VSWTCH
  m->tio.c_cc[VSWTCH] = 000;
#endif
# else /* TERMIO */
  /* struct sgttyb m_ttyb */
  /* struct tchars m_tchars */
  /* struct ltchars m_ltchars */
  /* int m_ldisc */
  /* int m_lmode */
# endif /* TERMIO */
#endif /* POSIX */
}

#define MOFF(a) (unsigned short)(&((struct mode *)0)->a)
#define T_C 0x01	/* char */
#define T_S 0x02	/* short */
#define T_L 0x03	/* long */
#define T_O 0x10	/* orwith */
#define T_T 0x20	/* setTo */
#define T_U 0x30	/* user defined */
#define T_LO (T_L | T_O)
#define T_CU (T_C | T_U)

static struct ttyflagtab
{
  char *name;		/* name of the flag */
  unsigned short off;	/* where this flag is in the structure */
  unsigned short type;	/* T_CHAR, T_SHORT, T_LONG, T_ORWITH, T_SETTO, T_USER */
  unsigned long flag;	/* value to set to */
  unsigned long clear;	/* bits to clear first, =flag, if 0 */
} tflags[] =
{
#if defined(POSIX) || defined(TERMIO)
  /* struct termios tio */
  "ignbrk",	MOFF(tio.c_iflag),	T_LO,	IGNBRK,	0,
  "brkint", 	MOFF(tio.c_iflag),	T_LO,	BRKINT,	0,
  "ignpar", 	MOFF(tio.c_iflag),	T_LO,	IGNPAR,	0,
  "parmrk", 	MOFF(tio.c_iflag),	T_LO,	PARMRK,	0,
  "inpck", 	MOFF(tio.c_iflag),	T_LO,	INPCK,	0,
  "istrip", 	MOFF(tio.c_iflag),	T_LO,	ISTRIP,	0,
  "inlcr", 	MOFF(tio.c_iflag),	T_LO,	INLCR,	0,
  "igncr",	MOFF(tio.c_iflag),	T_LO,	IGNCR,	0,
  "icrnl",	MOFF(tio.c_iflag),	T_LO,	ICRNL,	0,
  "ixon", 	MOFF(tio.c_iflag),	T_LO,	IXON,	0,
  "ixoff", 	MOFF(tio.c_iflag),	T_LO,	IXOFF,	0,

  "opost",	MOFF(tio.c_oflag),	T_LO,	OPOST,	0,
 
  "0",		MOFF(tio.c_cflag),	T_LO,	B0,	CBAUD,
  "50",		MOFF(tio.c_cflag),      T_LO,	B50,	CBAUD,
  "75",		MOFF(tio.c_cflag),      T_LO,	B75,	CBAUD,
  "110",	MOFF(tio.c_cflag),      T_LO,	B110,	CBAUD,
  "134",	MOFF(tio.c_cflag),      T_LO,	B134,	CBAUD,
  "150",	MOFF(tio.c_cflag),      T_LO,	B150,	CBAUD,
  "200",	MOFF(tio.c_cflag),      T_LO,	B200,	CBAUD,
  "300",	MOFF(tio.c_cflag),      T_LO,	B300,	CBAUD,
  "600",	MOFF(tio.c_cflag),      T_LO,	B600,	CBAUD,
  "1200",	MOFF(tio.c_cflag),      T_LO,	B1200,	CBAUD,
  "2400",	MOFF(tio.c_cflag),      T_LO,	B2400,	CBAUD,
  "4800",	MOFF(tio.c_cflag),      T_LO,	B4800,	CBAUD,
  "9600",	MOFF(tio.c_cflag),      T_LO,	B9600,	CBAUD,
  "19200",	MOFF(tio.c_cflag),      T_LO,	B19200,	CBAUD,
  "34800",	MOFF(tio.c_cflag),      T_LO,	B38400,	CBAUD,

  "cs5",	MOFF(tio.c_cflag),	T_LO,	CS5,	CSIZE,
  "cs6",	MOFF(tio.c_cflag),	T_LO,	CS6,	CSIZE,
  "cs7",	MOFF(tio.c_cflag),	T_LO,	CS7,	CSIZE,
  "cs8",	MOFF(tio.c_cflag),	T_LO,	CS8,	CSIZE,
  "cstopb",	MOFF(tio.c_cflag),	T_LO,	CSTOPB,	0,
  "cread",	MOFF(tio.c_cflag),	T_LO,	CREAD,	0,
  "parenb",	MOFF(tio.c_cflag),	T_LO,	PARENB,	0,
  "parodd",	MOFF(tio.c_cflag),	T_LO,	PARODD,	0,
  "hupcl",	MOFF(tio.c_cflag),	T_LO,	HUPCL,	0,
  "clocal",	MOFF(tio.c_cflag),	T_LO,	CLOCAL,	0,

  "isig",	MOFF(tio.c_lflag),	T_LO,	ISIG,	0,
  "icanon",	MOFF(tio.c_lflag),	T_LO,	ICANON,	0,
  "echo",	MOFF(tio.c_lflag),	T_LO,	ECHO,	0,
  "echoe",	MOFF(tio.c_lflag),	T_LO,	ECHOE,	0,
  "echok",	MOFF(tio.c_lflag),	T_LO,	ECHOK,	0,
  "echonl",	MOFF(tio.c_lflag),	T_LO,	ECHONL,	0,
  "noflsh",	MOFF(tio.c_lflag),	T_LO,	NOFLSH,	0,

  "vintr",	MOFF(tio.c_cc[VINTR]),	T_CU,	0,	0xff,
  "vquit",	MOFF(tio.c_cc[VQUIT]),	T_CU,	0,	0xff,
  "verase",	MOFF(tio.c_cc[VERASE]),	T_CU,	0,	0xff,
  "vkill",	MOFF(tio.c_cc[VKILL]),	T_CU,	0,	0xff,
  "veof",	MOFF(tio.c_cc[VEOF]),	T_CU,	0,	0xff,
  "veol",	MOFF(tio.c_cc[VEOL]),	T_CU,	0,	0xff,
#endif /* POSIX || TERMIO */
#if defined(POSIX) && !defined(TERMIO)
  "tostop",	MOFF(tio.c_lflag),	T_LO,	TOSTOP,	0,
  "iexten",	MOFF(tio.c_lflag),	T_LO,	IEXTEN,	0,

  "vstart",	MOFF(tio.c_cc[VSTART]),	T_CU,	0,	0xff,
  "vstop",	MOFF(tio.c_cc[VSTOP]),	T_CU,	0,	0xff,
  "vsusp",	MOFF(tio.c_cc[VSUSP]),	T_CU,	0,	0xff,
#endif /* POSIX && !TERMIO */
#if (defined(POSIX) && !defined(_POSIX_SOURCE)) || (!defined(POSIX) && defined(TERMIO))
  "iuclc",	MOFF(tio.c_iflag),	T_LO,	IUCLC,	0,
  "ixany",	MOFF(tio.c_iflag),	T_LO,	IXANY,	0,

  "olcuc",	MOFF(tio.c_oflag),	T_LO,	OLCUC,	0,
  "onlcr",	MOFF(tio.c_oflag),	T_LO,	ONLCR,	0,
  "ocrnl",	MOFF(tio.c_oflag),	T_LO,	OCRNL,	0,
  "onocr",	MOFF(tio.c_oflag),	T_LO,	ONOCR,	0,
  "onlret",	MOFF(tio.c_oflag),	T_LO,	ONLRET,	0,
  "ofill",	MOFF(tio.c_oflag),	T_LO,	OFILL,	0,
  "ofdel",	MOFF(tio.c_oflag),	T_LO,	OFDEL,	0,
  "nl0",	MOFF(tio.c_oflag),	T_LO,	NL0,	NLDLY,
  "nl1",	MOFF(tio.c_oflag),	T_LO,	NL1,	NLDLY,
  "cr0",	MOFF(tio.c_oflag),	T_LO,	CR0,	CRDLY,
  "cr1",	MOFF(tio.c_oflag),	T_LO,	CR1,	CRDLY,
  "cr2",	MOFF(tio.c_oflag),	T_LO,	CR2,	CRDLY,
  "cr3",	MOFF(tio.c_oflag),	T_LO,	CR3,	CRDLY,
  "tab0",	MOFF(tio.c_oflag),	T_LO,	TAB0,	TABDLY,
  "tab1",	MOFF(tio.c_oflag),	T_LO,	TAB1,	TABDLY,
  "tab2",	MOFF(tio.c_oflag),	T_LO,	TAB2,	TABDLY,
  "tab3",	MOFF(tio.c_oflag),	T_LO,	TAB3,	TABDLY,
  "bs0",	MOFF(tio.c_oflag),	T_LO,	BS0,	BSDLY,
  "bs1",	MOFF(tio.c_oflag),	T_LO,	BS1,	BSDLY,
  "vt0",	MOFF(tio.c_oflag),	T_LO,	VT0,	VTDLY,
  "vt1",	MOFF(tio.c_oflag),	T_LO,	VT1,	VTDLY,
  "ff0",	MOFF(tio.c_oflag),	T_LO,	FF0,	FFDLY,
  "ff1",	MOFF(tio.c_oflag),	T_LO,	FF1,	FFDLY,

  "loblk",	MOFF(tio.c_cflag),	T_LO,	LOBLK,	0,

  "xcase",	MOFF(tio.c_lflag),	T_LO,	XCASE,	0,

  "veol2",	MOFF(tio.c_cc[VEOL2]),		T_CU,	0,	0xff,
  "vswtch",	MOFF(tio.c_cc[VSWTCH]),		T_CU,	0,	0xff,
#endif /* (POSIX && !_POSIX_SOURCE) || (!POSIX && TERMIO) */
#if defined(POSIX) && !defined(_POSIX_SOURCE)
  "imaxbel",	MOFF(tio.c_iflag),	T_LO,	IMAXBEL,	0,

  "xtabs",	MOFF(tio.c_oflag),	T_LO,	XTABS,	TABDLY,

  "pageout",	MOFF(tio.c_oflag),	T_LO,	PAGEOUT,	0,
  "wrap",	MOFF(tio.c_oflag),	T_LO,	WRAP,	0,
  "crtscts",	MOFF(tio.c_cflag),	T_LO,	CRTSCTS,	0,
  "i0",		MOFF(tio.c_cflag),	T_LO,	B0 << IBSHIFT,	CBAUD << IBSHIFT,
  "i50",	MOFF(tio.c_cflag),      T_LO,	B50 << IBSHIFT,	CBAUD << IBSHIFT,
  "i75",	MOFF(tio.c_cflag),      T_LO,	B75 << IBSHIFT,	CBAUD << IBSHIFT,
  "i110",	MOFF(tio.c_cflag),      T_LO,	B110 << IBSHIFT,	CBAUD << IBSHIFT,
  "i134",	MOFF(tio.c_cflag),      T_LO,	B134 << IBSHIFT,	CBAUD << IBSHIFT,
  "i150",	MOFF(tio.c_cflag),      T_LO,	B150 << IBSHIFT,	CBAUD << IBSHIFT,
  "i200",	MOFF(tio.c_cflag),      T_LO,	B200 << IBSHIFT,	CBAUD << IBSHIFT,
  "i300",	MOFF(tio.c_cflag),      T_LO,	B300 << IBSHIFT,	CBAUD << IBSHIFT,
  "i600",	MOFF(tio.c_cflag),      T_LO,	B600 << IBSHIFT,	CBAUD << IBSHIFT,
  "i1200",	MOFF(tio.c_cflag),      T_LO,	B1200 << IBSHIFT,	CBAUD << IBSHIFT,
  "i2400",	MOFF(tio.c_cflag),      T_LO,	B2400 << IBSHIFT,	CBAUD << IBSHIFT,
  "i4800",	MOFF(tio.c_cflag),      T_LO,	B4800 << IBSHIFT,	CBAUD << IBSHIFT,
  "i9600",	MOFF(tio.c_cflag),      T_LO,	B9600 << IBSHIFT,	CBAUD << IBSHIFT,
  "i19200",	MOFF(tio.c_cflag),      T_LO,	B19200 << IBSHIFT,	CBAUD << IBSHIFT,
  "i34800",	MOFF(tio.c_cflag),      T_LO,	B38400 << IBSHIFT,	CBAUD << IBSHIFT,

  "echoctl",	MOFF(tio.c_lflag),	T_LO,	ECHOCTL,	0,
  "echoprt",	MOFF(tio.c_lflag),	T_LO,	ECHOPRT,	0,
  "echoke",	MOFF(tio.c_lflag),	T_LO,	ECHOKE,		0,
  "defecho",	MOFF(tio.c_lflag),	T_LO,	DEFECHO,	0,
  "flusho",	MOFF(tio.c_lflag),	T_LO,	FLUSHO,		0,
  "pendin",	MOFF(tio.c_lflag),	T_LO,	PENDIN,		0,

  "vdsusp",	MOFF(tio.c_cc[VDSUSP]),		T_CU,	0,	0xff,
  "vreprint",	MOFF(tio.c_cc[VREPRINT]),	T_CU,	0,	0xff,
  "vdiscard",	MOFF(tio.c_cc[VDISCARD]),	T_CU,	0,	0xff,
  "vwerase",	MOFF(tio.c_cc[VWERASE]),	T_CU,	0,	0xff,
  "vlnext",	MOFF(tio.c_cc[VLNEXT]),		T_CU,	0,	0xff,
  "vstatus",	MOFF(tio.c_cc[VSTATUS]),	T_CU,	0,	0xff,
#endif  /* POSIX && !_POSIX_SOURCE */
#if defined(POSIX) && defined(hpux)
  /* struct ltchars m_ltchars */
#endif /* POSIX && hpux */
#if !defined(POSIX) && defined(TERMIO)
  /* struct termio but not struct termios*/
  "exta",	MOFF(tio.c_cflag),	T_LO,	EXTA,	CBAUD,
  "extb",	MOFF(tio.c_cflag),	T_LO,	EXTB,	CBAUD,
  "rcv1en",	MOFF(tio.c_cflag),	T_LO,	RCV1EN,	0,
  "xmt1en",	MOFF(tio.c_cflag),	T_LO,	XMT1EN,	0,
#endif /* !POSIX && TERMIO */
#if !defined(POSIX) &&  !defined(TERMIO)
  /* struct sgttyb m_ttyb */
  /* struct tchars m_tchars */
  /* struct ltchars m_ltchars */
  /* int m_ldisc */
  /* int m_lmode */
#endif /* !POSIX && !TERMIO */
  0, 0, 0, 0
};

struct ttyflagmacro
{
  char *name, *macro;
} tmacs[] =
{
#ifdef POSIX
# ifndef _POSIX_SOURCE
  "sane", 	"brkint ignpar istrip icrnl ixon opost onlcr tab3 cs8 cread parenb isig icanon echo echoe echok iexten vintr=^C vquit=^\ verase =^? vkill=^H veof=^D vstart=^Q vstop=^S vsusp=^Z vdsusp=^Y vreprint=^R vdiscard=^O vwerase=^W vlnext=^V vstatus=^T",
# else /* _POSIX_SOURCE */
  "sane", 	"brkint ignpar istrip icrnl ixon opost cs8 cread parenb isig icanon echo echoe echok iexten vintr=^C vquit=^\ verase =^? vkill=^H veof=^D vstart=^Q vstop=^S vsusp=^Z",
# endif /* _POSIX_SOURCE */
#else /* POSIX */
# ifdef TERMIO
  "sane", 	"istrip icrnl ixon opost onlcr tab3 cs8 cread isig icanon echo echoe echok vintr=^C vquit=^\ verase =^? vkill=^H veof=^D",
# else /* TERMIO */
# endif /* TERMIO */
#endif /* POSIX */
  0, 0
};
  
void
Mode2StringTTY(s, m)
char *s;
struct mode *m;
{
#ifdef POSIX
  /* struct termios tio */
# ifdef hpux
  /* struct ltchars m_ltchars */
# endif /* hpux */
#else /* POSIX */
# ifdef TERMIO
  /* struct termio tio */
# else /* TERMIO */
  /* struct sgttyb m_ttyb */
  /* struct tchars m_tchars */
  /* struct ltchars m_ltchars */
  /* int m_ldisc */
  /* int m_lmode */
# endif /* TERMIO */
#endif /* POSIX */
}

struct ttyflagtab *
findttyflag(s, l)
{
  struct ttyflagtab *t;

  if (!s || l <= 0)
    return 0;
  for (t = tflags; t->name; t++)
    {
      if (!strncmp(t->name, s, l))
	return t;
    }
  return 0;
}

void
String2ModeTTY(s, m)
char *s;
struct mode *m;
{
  char *e, *p;
  int not;
  struct ttyflagtab *t;
  unsigned long *lh;
  unsigned short *sh;
  unsigned char *ch;

  if (!s || !m)
    return;
  while (*s)
    {
      while (*s == ' ')
	s++;
      for (e = s; *e != ' '; e++)
	;
      not = (*s == '-') ? 1 : 0;
      if (not)
	s++;
      for (p = s; p < e; p++)
	if (*p == '=')
	  break;
      if ((t = findttyflag(s, p - s - 1)) == 0)
	{
	  debug1("String2ModeTTY: %s not found\n", s);
	  s = e;
	  continue;
	}
      debug3("String2ModeTTY: %s, %#x, %#x\n", t->name, t->type, t->flag); 
      switch (t->type & (T_L | T_S | T_C))
	{
	case T_L:
	  lh = (unsigned long *)((char *)m + t->off);
	  if (t->clear)
	    *lh &= ~((unsigned long)t->clear);
	  else
	    *lh &= ~((unsigned long)t->flag);
	  if (!not)
	    *lh |= (unsigned long)t->flag;
	  break;
	case T_S:
	  sh = (unsigned short *)((char *)m + t->off);
	  if (t->clear)
	    *sh &= ~((unsigned short)t->clear);
	  else
	    *sh &= ~((unsigned short)t->flag);
	  if (!not)
	    *sh |= (unsigned short)t->flag;
	  break;
	case T_C:
	  ch = (unsigned char *)((char *)m + t->off);
	  if (t->clear)
	    *ch &= ~((unsigned char)t->clear);
	  else
	    *ch &= ~((unsigned char)t->flag);
	  if (!not)
	    *ch |= (unsigned char)t->flag;
	  if (p < e)
	    {
	      if (*p == '=')
		p++;
	      *ch = '\0';
	      *ch = (unsigned char)*p;
	      p++;
	      if (*ch == '^' && p < e)
		{
		  *ch = Ctrl(*p);
		}
	    }
	  break;
	}
      s = e;
    }
#ifdef DEBUG
  DebugTTY(m);
#endif
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
  debug1("c_line = %#x\n", m->tio.c_line);
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
  /* struct sgttyb m_ttyb */
  /* struct tchars m_tchars */
  /* struct ltchars m_ltchars */
  /* int m_ldisc */
  /* int m_lmode */
# endif /* TERMIO */
#endif /* POSIX */
}
#else /* DEBUG */
void
DebugTTY(m)
struct mode *m;
{
}
#endif /* DEBUG */
