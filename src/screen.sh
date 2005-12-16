echo x - Makefile
sed 's/^X//' >Makefile <<'*-*-END-of-Makefile-*-*'
X# The following options can be set:
X#
X# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
X#                (an array of three doubles called _avenrun; it is read
X#                from /dev/kmem; _avenrun is taken from the namelist of
X#                /vmunix).  Don't set this on a Sun.
X# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
X#                and the getttyent(3) library functions.
X#
X# You should install as set-uid with owner root, so that it can read/write
X# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.
X
XOPTIONS= -DLOADAV
X
XCFILES= screen.c ansi.c
XOFILES= screen.o ansi.o
X
Xscreen: $(OFILES)
X	cc $(CFLAGS) -o screen $(OFILES) -ltermcap
X
Xscreen.o: screen.c screen.h
X	cc $(OPTIONS) $(CFLAGS) -c screen.c
X
Xansi.o: ansi.c screen.h
X	cc $(CFLAGS) -c ansi.c
*-*-END-of-Makefile-*-*
echo x - README
sed 's/^X//' >README <<'*-*-END-of-README-*-*'
X"screen" is a window manager that allows you to handle several independent
Xscreens (UNIX ttys) on a single physical terminal; each screen has its own
Xset of processes connected to it (typically interactive shells).  Each
Xvirtual terminal created by "screen" emulates a DEC VT100 plus several ANSI
XX3.64 functions (including DEC VT102 features such as line and character
Xdeletion and insertion).
X
XSince "screen" uses pseudo-ttys, the select system call, and UNIX-domain
Xsockets, it will not run under a system that does not include these
Xfeatures of 4.2 and 4.3 BSD UNIX.
X
XIf you want to get a quick idea how "screen" works but don't want to read
Xthe entire manual, do the following:
X 
X     -  call "screen" without arguments
X     -  wait for the shell prompt; execute some commands
X     -  type ^A ^C (Control-A followed by Control-C)
X     -  wait for the shell prompt; do something in the new window
X     -  type ^A ^A repeatedly to switch between the two windows
X     -  terminate the first shell ("screen" switches to the other window)
X     -  terminate the second shell
X
XIf you have got "vttest" (the VT100 test program from mod.sources) you
Xmay want to run it from within "screen" to verify that it correctly
Xemulates a VT100 on your terminal (except for 132 column mode and
Xdouble width/height characters, of course).
X
XBy the way, "screen" can be used to compensate for certain bugs of "real"
XVT100 terminals.  For instance, our 4.2 BSD version of mille(6) garbles
Xthe display on terminals of the VT100 family, but it works quite fine
Xwhen it is invoked from within "screen".  In addition, "screen" enables
Xyou to use EMACS on terminals that are unable to generate Control-S and
XControl-Q from the keyboard or that require flow control using Control-S
Xand Control-Q.  This is the reason why I have an alias like
X     alias emacs "screen emacs"
Xin my .cshrc file.
X
X
XI have published a beta-test release of screen in the non-moderated
Xsources newsgroup four months ago.  Since then I have received numerous
Xsuggestions for enhancements and improvements, many of which I have
Xincluded in this release of "screen".  One person reported that screen
Xdoes not work correctly on the Ann Arbor Guru-XL; I have not been able
Xto track the problem down without having more detailed information than
Xjust the termcap entry.
X
XMajor changes between this and the beta-test release are:
X
X    -  "screen" now creates an entry in /etc/utmp for each virtual
X       terminal
X    -  the owner of the tty file for a virtual terminal is set properly
X       (provided that "screen" is set-uid with owner root)
X    -  the -c option has been eliminated; you can now start a command
X       in a new window with "screen [cmd [args]]"
X    -  a (user-settable) notification can be displayed in the current
X       window's message line when the bell is rung in a background window
X    -  a "chdir" command can be placed into ".screenrc" to facilitate
X       creation of windows in specific directories
X    -  flow control can be disabled by means of a command line option or
X       a special termcap symbol (necessary for EMACS)
X    -  "C-a s" and "C-a q" can be used to send a Control-S or a Control-Q,
X       respectively (for certain terminals)
X    -  it is no longer necessary to specify the full pathname when a
X       command is started in a new window (and with the "bind" command)
X    -  "C-a ." can be used to write the current termcap entry to a file
X       (useful for "rlogin" which does not propagate TERMCAP)
X    -  "C-a C-t" displays status information (e.g. the load average and
X       the virtual terminal's parameters) in the message line
X    -  "C-a C-\" closes all windows and terminates screen
X
XBefore typing "make", you should have a look into the Makefile.
XIf your system maintains a 4.3-BSD-style load average, add -DLOADAV to
Xthe compiler options.  In addition, you must set -DGETTTYENT if your
Xsystem has the new format /etc/ttys and the getttyent(3) routines.
X
X"screen" should be granted read and write access to /etc/utmp and, if
X-DLOADAV has been specified, read access to /vmunix and /dev/kmem.
X"screen" should be installed with set-uid and owner root to enable it
Xto correctly change the owner of newly allocated virtual terminals.
XFailing to do this (e.g. if you fear a trojan horse) doesn't have any
Xmajor disadvantages, except that w(1) and some other utilities may have
Xsome problems with the tty files of your virtual terminals.
X
X
XHave fun,
X    Oliver Laumann
X    Technical University of Berlin,
X    Communications and Operating Systems Research Group.
X
X    net@tub.BITNET     US: pyramid!tub!net     Europe: unido!tub!net
X
*-*-END-of-README-*-*
echo x - ansi.c
sed 's/^X//' >ansi.c <<'*-*-END-of-ansi.c-*-*'
X/* Copyright (c) 1987, Oliver Laumann, Technical University of Berlin.
X * Not derived from licensed software.
X *
X * Permission is granted to freely use, copy, modify, and redistribute
X * this software, provided that no attempt is made to gain profit from it,
X * the author is not construed to be liable for any results of using the
X * software, alterations are clearly marked as such, and this notice is
X * not modified.
X */
X
Xchar AnsiVersion[] = "ansi 1.0g 27-Apr-87";
X
X#include <stdio.h>
X#include <sys/types.h>
X#include "screen.h"
X
X#define A_SO     0x1    /* Standout mode */
X#define A_US     0x2    /* Underscore mode */
X#define A_BL     0x4    /* Blinking */
X#define A_BD     0x8    /* Bold mode */
X#define A_DI    0x10    /* Dim mode */
X#define A_RV    0x20    /* Reverse mode */
X#define A_MAX   0x20
X
X/* Types of movement used by Goto() */
Xenum move_t {
X    M_NONE,
X    M_UP,
X    M_DO,
X    M_LE,
X    M_RI,
X    M_RW,
X    M_CR,
X};
X
X#define MAXARGS      64
X#define EXPENSIVE    1000
X
Xextern char *getenv(), *tgetstr(), *tgoto(), *malloc();
X
Xint rows, cols;
Xint status;
Xint flowctl;
Xchar Term[] = "TERM=screen";
Xchar Termcap[1024];
Xchar *blank;
Xchar PC;
Xtime_t TimeDisplayed;
X
Xstatic char tbuf[1024], tentry[1024];
Xstatic char *tp = tentry;
Xstatic char *TI, *TE, *BL, *VB, *BC, *CR, *NL, *CL, *IS, *CM;
Xstatic char *US, *UE, *SO, *SE, *CE, *CD, *DO, *SR, *SF, *AL;
Xstatic char *CS, *DL, *DC, *IC, *IM, *EI, *UP, *ND, *KS, *KE;
Xstatic char *MB, *MD, *MH, *MR, *ME;
Xstatic AM;
Xstatic args[MAXARGS];
Xstatic char GotArg[MAXARGS];
Xstatic NumArgs;
Xstatic char GlobalAttr, TmpAttr;
Xstatic char *OldImage, *OldAttr;
Xstatic last_x, last_y;
Xstatic struct win *curr;
Xstatic display = 1;
Xstatic StrCost;
Xstatic UPcost, DOcost, LEcost, NDcost, CRcost, IMcost, EIcost;
Xstatic tcLineLen = 100;
Xstatic char *null;
Xstatic StatLen;
Xstatic insert;
Xstatic keypad;
X
Xstatic char *KeyCaps[] = {
X    "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8", "k9",
X    "kb", "kd", "kh", "kl", "ko", "kr", "ku",
X    0
X};
X
Xstatic char TermcapConst[] = "TERMCAP=\
XSC|screen|VT 100/ANSI X3.64 virtual terminal|\\\n\
X\t:DO=\\E[%dB:LE=\\E[%dD:RI=\\E[%dC:UP=\\E[%dA:bs:bt=\\E[Z:\\\n\
X\t:cd=\\E[J:ce=\\E[K:cl=\\E[2J\\E[H:cm=\\E[%i%d;%dH:ct=\\E[3g:\\\n\
X\t:do=\\E[B:nd=\\E[C:pt:rc=\\E8:rs=\\Ec:sc=\\E7:st=\\EH:up=\\E[A:";
X
XInitTerm () {
X    register char *s;
X
X    if ((s = getenv ("TERM")) == 0)
X	Msg (0, "No TERM in environment.");
X    if (tgetent (tbuf, s) != 1)
X	Msg (0, "Cannot find termcap entry for %s.", s);
X    cols = tgetnum ("co");
X    rows = tgetnum ("li");
X    if (cols <= 0)
X	cols = 80;
X    if (rows <= 0)
X	rows = 24;
X    if (tgetflag ("hc"))
X	Msg (0, "You can't run screen on a hardcopy terminal.");
X    if (tgetflag ("os"))
X	Msg (0, "You can't run screen on a terminal that overstrikes.");
X    if (tgetflag ("ns"))
X	Msg (0, "Terminal must support scrolling.");
X    if (!(CL = tgetstr ("cl", &tp)))
X	Msg (0, "Clear screen capability required.");
X    if (!(CM = tgetstr ("cm", &tp)))
X	Msg (0, "Addressable cursor capability required.");
X    if (s = tgetstr ("ps", &tp))
X	PC = s[0];
X    flowctl = !tgetflag ("NF");
X    AM = tgetflag ("am");
X    if (tgetflag ("LP"))
X	AM = 0;
X    TI = tgetstr ("ti", &tp);
X    TE = tgetstr ("te", &tp);
X    if (!(BL = tgetstr ("bl", &tp)))
X	BL = "\007";
X    VB = tgetstr ("vb", &tp);
X    if (!(BC = tgetstr ("bc", &tp))) {
X	if (tgetflag ("bs"))
X	    BC = "\b";
X	else
X	    BC = tgetstr ("le", &tp);
X    }
X    if (!(CR = tgetstr ("cr", &tp)))
X	CR = "\r";
X    if (!(NL = tgetstr ("nl", &tp)))
X	NL = "\n";
X    IS = tgetstr ("is", &tp);
X    if (tgetnum ("sg") <= 0) {
X	US = tgetstr ("us", &tp);
X	UE = tgetstr ("ue", &tp);
X	SO = tgetstr ("so", &tp);
X	SE = tgetstr ("se", &tp);
X	MB = tgetstr ("mb", &tp);
X	MD = tgetstr ("md", &tp);
X	MH = tgetstr ("mh", &tp);
X	MR = tgetstr ("mr", &tp);
X	ME = tgetstr ("me", &tp);
X	/*
X	 * Does ME also reverse the effect of SO and/or US?  This is not
X	 * clearly specified by the termcap manual.
X	 * Anyway, we should at least look whether ME and SE/UE are equal:
X	 */
X	if (SE && UE && ME && (strcmp (SE, UE) == 0 || strcmp (ME, UE) == 0))
X	    UE = 0;
X	if (SE && ME && strcmp (SE, ME) == 0)
X	    SE = 0;
X    }
X    CE = tgetstr ("ce", &tp);
X    CD = tgetstr ("cd", &tp);
X    if (!(DO = tgetstr ("do", &tp)))
X	DO = NL;
X    UP = tgetstr ("up", &tp);
X    ND = tgetstr ("nd", &tp);
X    SR = tgetstr ("sr", &tp);
X    if (!(SF = tgetstr ("sf", &tp)))
X	SF = NL;
X    AL = tgetstr ("al", &tp);
X    DL = tgetstr ("dl", &tp);
X    CS = tgetstr ("cs", &tp);
X    DC = tgetstr ("dc", &tp);
X    IC = tgetstr ("ic", &tp);
X    IM = tgetstr ("im", &tp);
X    EI = tgetstr ("ei", &tp);
X    if (tgetflag ("in"))
X	IC = IM = 0;
X    if (IC && IC[0] == '\0')
X	IC = 0;
X    if (IM && IM[0] == '\0')
X	IM = 0;
X    if (EI && EI[0] == '\0')
X	EI = 0;
X    KS = tgetstr ("ks", &tp);
X    KE = tgetstr ("ke", &tp);
X    blank = malloc (cols);
X    null = malloc (cols);
X    OldImage = malloc (cols);
X    OldAttr = malloc (cols);
X    if (!(blank && null && OldImage && OldAttr))
X	Msg (0, "Out of memory.");
X    MakeBlankLine (blank, cols);
X    bzero (null, cols);
X    UPcost = CalcCost (UP);
X    DOcost = CalcCost (DO);
X    LEcost = CalcCost (BC);
X    NDcost = CalcCost (ND);
X    CRcost = CalcCost (CR);
X    IMcost = CalcCost (IM);
X    EIcost = CalcCost (EI);
X    PutStr (IS);
X    PutStr (TI);
X    PutStr (CL);
X}
X
XFinitTerm () {
X    PutStr (TE);
X    PutStr (IS);
X}
X
Xstatic AddCap (s) char *s; {
X    register n;
X
X    if (tcLineLen + (n = strlen (s)) > 55) {
X	strcat (Termcap, "\\\n\t:");
X	tcLineLen = 0;
X    }
X    strcat (Termcap, s);
X    tcLineLen += n;
X}
X
Xchar *MakeTermcap (aflag) {
X    char buf[1024];
X    register char **pp, *p;
X
X    strcpy (Termcap, TermcapConst);
X    sprintf (buf, "li#%d:co#%d:", rows, cols);
X    AddCap (buf);
X    if (VB)
X	AddCap ("vb=\\E[?5h\\E[?5l:");
X    if (US) {
X	AddCap ("us=\\E[4m:");
X	AddCap ("ue=\\E[24m:");
X    }
X    if (SO) {
X	AddCap ("so=\\E[3m:");
X	AddCap ("se=\\E[23m:");
X    }
X    if (MB)
X	AddCap ("mb=\\E[5m:");
X    if (MD)
X	AddCap ("md=\\E[1m:");
X    if (MH)
X	AddCap ("mh=\\E[2m:");
X    if (MR)
X	AddCap ("mr=\\E[7m:");
X    if (MB || MD || MH || MR)
X	AddCap ("me=\\E[0m:");
X    if ((CS && SR) || AL || aflag) {
X	AddCap ("sr=\\EM:");
X	AddCap ("al=\\E[L:");
X	AddCap ("AL=\\E[%dL:");
X    }
X    if (CS || DL || aflag) {
X	AddCap ("dl=\\E[M:");
X	AddCap ("DL=\\E[%dM:");
X    }
X    if (CS)
X	AddCap ("cs=\\E[%i%d;%dr:");
X    if (DC || aflag) {
X	AddCap ("dc=\\E[P:");
X	AddCap ("DC=\\E[%dP:");
X    }
X    if (IC || IM || aflag) {
X	AddCap ("im=\\E[4h:");
X	AddCap ("ei=\\E[4l:");
X	AddCap ("ic=:");
X	AddCap ("IC=\\E[%d@:");
X    }
X    if (KS)
X	AddCap ("ks=\\E=:");
X    if (KE)
X	AddCap ("ke=\\E>:");
X    for (pp = KeyCaps; *pp; ++pp)
X	if (p = tgetstr (*pp, &tp)) {
X	    MakeString (*pp, buf, p);
X	    AddCap (buf);
X	}
X    return Termcap;
X}
X
XMakeString (cap, buf, s) char *cap, *buf; register char *s; {
X    register char *p = buf;
X    register unsigned c;
X
X    *p++ = *cap++; *p++ = *cap; *p++ = '=';
X    while (c = *s++) {
X	switch (c) {
X	case '\033':
X	    *p++ = '\\'; *p++ = 'E'; break;
X	case ':':
X	    sprintf (p, "\\072"); p += 4; break;
X	case '^': case '\\':
X	    *p++ = '\\'; *p++ = c; break;
X	default:
X	    if (c >= 200) {
X		sprintf (p, "\\%03o", c & 0377); p += 4;
X	    } else if (c < ' ') {
X		*p++ = '^'; *p++ = c + '@';
X	    } else *p++ = c;
X	}
X    }
X    *p++ = ':'; *p = '\0';
X}
X
XActivate (wp) struct win *wp; {
X    RemoveStatus (wp);
X    curr = wp;
X    display = 1;
X    NewRendition (GlobalAttr, curr->LocalAttr);
X    GlobalAttr = curr->LocalAttr;
X    InsertMode (curr->insert);
X    KeypadMode (curr->keypad);
X    if (CS)
X	PutStr (tgoto (CS, curr->bot, curr->top));
X    Redisplay ();
X}
X
XResetScreen (p) register struct win *p; {
X    register i;
X
X    bzero (p->tabs, cols);
X    for (i = 8; i < cols; i += 8)
X	p->tabs[i] = 1;
X    p->wrap = 1;
X    p->origin = 0;
X    p->insert = 0;
X    p->vbwait = 0;
X    p->keypad = 0;
X    p->top = 0;
X    p->bot = rows - 1;
X    p->saved = 0;
X    p->LocalAttr = 0;
X    p->x = p->y = 0;
X    p->state = LIT;
X    p->StringType = NONE;
X}
X
XWriteString (wp, buf, len) struct win *wp; register char *buf; {
X    register c, intermediate = 0;
X
X    if (!len)
X	return;
X    curr = wp;
X    display = curr->active;
X    if (display)
X	RemoveStatus (wp);
X    do {
X	c = *buf++;
X	if (c == '\0' || c == '\177')
X	    continue;
XNextChar:
X	switch (curr->state) {
X	case TERM:
X	    switch (c) {
X		case '\\':
X		    curr->state = LIT;
X		    *(curr->stringp) = '\0';
X		    if (curr->StringType == PM && display) {
X			MakeStatus (curr->string, curr);
X			if (status && len > 1) {
X			    curr->outlen = len-1;
X			    bcopy (buf, curr->outbuf, curr->outlen);
X			    return;
X			}
X		    }
X		    break;
X		default:
X		    curr->state = STR;
X		    AddChar ('\033');
X		    AddChar (c);
X	    }
X	    break;
X	case STR:
X	    switch (c) {
X		case '\0':
X		    break;
X		case '\033':
X		    curr->state = TERM; break;
X		default:
X		    AddChar (c);
X	    }
X	    break;
X	case ESC:
X	    switch (c) {
X	    case '[':
X		NumArgs = 0;
X		intermediate = 0;
X		bzero ((char *)args, MAXARGS * sizeof (int));
X		bzero (GotArg, MAXARGS);
X		curr->state = CSI;
X		break;
X	    case ']':
X		StartString (OSC); break;
X	    case '_':
X		StartString (APC); break;
X	    case 'P':
X		StartString (DCS); break;
X	    case '^':
X		StartString (PM); break;
X	    default:
X		if (Special (c))
X		    break;
X		if (c >= ' ' && c <= '/') {
X		    intermediate = intermediate ? -1 : c;
X		} else if (c >= '0' && c <= '~') {
X		    DoESC (c, intermediate);
X		    curr->state = LIT;
X		} else {
X		    curr->state = LIT;
X		    goto NextChar;
X		}
X	    }
X	    break;
X	case CSI:
X	    switch (c) {
X	    case '0': case '1': case '2': case '3': case '4':
X	    case '5': case '6': case '7': case '8': case '9':
X		if (NumArgs < MAXARGS) {
X		    args[NumArgs] = 10 * args[NumArgs] + c - '0';
X		    GotArg[NumArgs] = 1;
X		}
X		break;
X	    case ';': case ':':
X		NumArgs++; break;
X	    default:
X		if (Special (c))
X		    break;
X		if (c >= '@' && c <= '~') {
X		    NumArgs++;
X		    DoCSI (c, intermediate);
X		    curr->state = LIT;
X		} else if ((c >= ' ' && c <= '/') || (c >= '<' && c <= '?')) {
X		    intermediate = intermediate ? -1 : c;
X		} else {
X		    curr->state = LIT;
X		    goto NextChar;
X		}
X	    }
X	    break;
X	default:
X	    if (!Special (c)) {
X		if (c == '\033') {
X		    intermediate = 0;
X		    curr->state = ESC;
X		} else if (c < ' ') {
X		    break;
X		} else if (curr->x < cols-1) {
X		    if (curr->insert) {
X			InsertAChar (c);
X		    } else {
X			if (display)
X			    putchar (c);
X			SetChar (c);
X		    }
X		    curr->x++;
X		} else if (curr->x == cols-1) {
X		    SetChar (c);
X		    if (!(AM && curr->y == rows-1)) {
X			if (display)
X			    putchar (c);
X			Goto (-1, -1, curr->y, curr->x);
X		    }
X		    curr->x++;
X		} else {
X		    if (curr->wrap) {
X			Return ();
X			LineFeed ();
X			if (curr->insert) {
X			    InsertAChar (c);
X			} else {
X			    if (display)
X				putchar (c);
X			    SetChar (c);
X			}
X			curr->x = 1;
X		    } else curr->x = cols;
X		}
X	    }
X	}
X    } while (--len);
X    curr->outlen = 0;
X}
X
Xstatic Special (c) register c; {
X    switch (c) {
X    case '\b':
X	BackSpace (); return 1;
X    case '\r':
X	Return (); return 1;
X    case '\n':
X	LineFeed (); return 1;
X    case '\007':
X	PutStr (BL);
X	if (!display)
X	    curr->bell = 1;
X	return 1;
X    case '\t':
X	ForwardTab (); return 1;
X    }
X    return 0;
X}
X
Xstatic DoESC (c, intermediate) {
X    switch (intermediate) {
X    case 0:
X	switch (c) {
X	case 'E':
X	    Return ();
X	    LineFeed ();
X	    break;
X	case 'D':
X	    LineFeed (); 
X	    break;
X	case 'M':
X	    ReverseLineFeed ();
X	    break;
X	case 'H':
X	    curr->tabs[curr->x] = 1;
X	    break;
X	case '7':
X	    SaveCursor ();
X	    break;
X	case '8':
X	    RestoreCursor ();
X	    break;
X	case 'c':
X	    ClearScreen ();
X	    Goto (curr->y, curr->x, 0, 0);
X	    NewRendition (GlobalAttr, 0);
X	    SetRendition (0);
X	    if (curr->insert)
X		InsertMode (0);
X	    if (curr->keypad)
X		KeypadMode (0);
X	    if (CS)
X		PutStr (tgoto (CS, rows-1, 0));
X	    ResetScreen (curr);
X	    break;
X	case '=':
X	    KeypadMode (1);
X	    curr->keypad = 1;
X	    break;
X	case '>':
X	    KeypadMode (0);
X	    curr->keypad = 0;
X	    break;
X	}
X	break;
X    case '#':
X	switch (c) {
X	case '8':
X	    FillWithEs ();
X	    break;
X	}
X	break;
X    }
X}
X
Xstatic DoCSI (c, intermediate) {
X    register i, a1 = args[0], a2 = args[1];
X
X    if (NumArgs >= MAXARGS)
X	NumArgs = MAXARGS;
X    for (i = 0; i < NumArgs; ++i)
X	if (args[i] == 0)
X	    GotArg[i] = 0;
X    switch (intermediate) {
X    case 0:
X	switch (c) {
X	case 'H': case 'f':
X	    if (!GotArg[0]) a1 = 1;
X	    if (!GotArg[1]) a2 = 1;
X	    if (curr->origin)
X		a1 += curr->top;
X	    if (a1 < 1)
X		a1 = 1;
X	    if (a1 > rows)
X		a1 = rows;
X	    if (a2 < 1)
X		a2 = 1;
X	    if (a2 > cols)
X		a2 = cols;
X	    a1--; a2--;
X	    Goto (curr->y, curr->x, a1, a2);
X	    curr->y = a1;
X	    curr->x = a2;
X	    break;
X	case 'J':
X	    if (!GotArg[0] || a1 < 0 || a1 > 2)
X		a1 = 0;
X	    switch (a1) {
X	    case 0:
X		ClearToEOS (); break;
X	    case 1:
X		ClearFromBOS (); break;
X	    case 2:
X		ClearScreen ();
X		Goto (0, 0, curr->y, curr->x);
X		break;
X	    }
X	    break;
X	case 'K':
X	    if (!GotArg[0] || a1 < 0 || a2 > 2)
X		a1 = 0;
X	    switch (a1) {
X	    case 0:
X		ClearToEOL (); break;
X	    case 1:
X		ClearFromBOL (); break;
X	    case 2:
X		ClearLine (); break;
X	    }
X	    break;
X	case 'A':
X	    CursorUp (GotArg[0] ? a1 : 1);
X	    break;
X	case 'B':
X	    CursorDown (GotArg[0] ? a1 : 1);
X	    break;
X	case 'C':
X	    CursorRight (GotArg[0] ? a1 : 1);
X	    break;
X	case 'D':
X	    CursorLeft (GotArg[0] ? a1 : 1);
X	    break;
X	case 'm':
X	    SelectRendition ();
X	    break;
X	case 'g':
X	    if (!GotArg[0] || a1 == 0)
X		curr->tabs[curr->x] = 0;
X	    else if (a1 == 3)
X		bzero (curr->tabs, cols);
X	    break;
X	case 'r':
X	    if (!CS)
X		break;
X	    if (!GotArg[0]) a1 = 1;
X	    if (!GotArg[1]) a2 = rows;
X	    if (a1 < 1 || a2 > rows || a1 >= a2)
X		break;
X	    curr->top = a1-1;
X	    curr->bot = a2-1;
X	    PutStr (tgoto (CS, curr->bot, curr->top));
X	    if (curr->origin) {
X		Goto (-1, -1, curr->top, 0);
X		curr->y = curr->top;
X		curr->x = 0;
X	    } else {
X		Goto (-1, -1, 0, 0);
X		curr->y = curr->x = 0;
X	    }
X	    break;
X	case 'I':
X	    if (!GotArg[0]) a1 = 1;
X	    while (a1--)
X		ForwardTab ();
X	    break;
X	case 'Z':
X	    if (!GotArg[0]) a1 = 1;
X	    while (a1--)
X		BackwardTab ();
X	    break;
X	case 'L':
X	    InsertLine (GotArg[0] ? a1 : 1);
X	    break;
X	case 'M':
X	    DeleteLine (GotArg[0] ? a1 : 1);
X	    break;
X	case 'P':
X	    DeleteChar (GotArg[0] ? a1 : 1);
X	    break;
X	case '@':
X	    InsertChar (GotArg[0] ? a1 : 1);
X	    break;
X	case 'h':
X	    SetMode (1);
X	    break;
X	case 'l':
X	    SetMode (0);
X	    break;
X	}
X	break;
X    case '?':
X	if (c != 'h' && c != 'l')
X	    break;
X	if (!GotArg[0])
X	    break;
X	i = (c == 'h');
X	if (a1 == 5) {
X	    if (i) {
X		curr->vbwait = 1;
X	    } else {
X		if (curr->vbwait)
X		    PutStr (VB);
X		curr->vbwait = 0;
X	    }
X	} else if (a1 == 6) {
X	    curr->origin = i;
X	    if (curr->origin) {
X		Goto (curr->y, curr->x, curr->top, 0);
X		curr->y = curr->top;
X		curr->x = 0;
X	    } else {
X		Goto (curr->y, curr->x, 0, 0);
X		curr->y = curr->x = 0;
X	    }
X	} else if (a1 == 7) {
X	    curr->wrap = i;
X	}
X	break;
X    }
X}
X
Xstatic PutChar (c) {
X    putchar (c);
X}
X
Xstatic PutStr (s) char *s; {
X    if (display && s)
X	tputs (s, 1, PutChar);
X}
X
Xstatic SetChar (c) register c; {
X    register struct win *p = curr;
X
X    p->image[p->y][p->x] = c;
X    p->attr[p->y][p->x] = p->LocalAttr;
X}
X
Xstatic StartString (type) enum string_t type; {
X    curr->StringType = type;
X    curr->stringp = curr->string;
X    curr->state = STR;
X}
X
Xstatic AddChar (c) {
X    if (curr->stringp > curr->string+MAXSTR-1)
X	curr->state = LIT;
X    else
X	*(curr->stringp)++ = c;
X}
X
X/* Insert mode is a toggle on some terminals, so we need this hack:
X */
XInsertMode (on) {
X    if (on) {
X	if (!insert)
X	    PutStr (IM);
X    } else if (insert)
X	PutStr (EI);
X    insert = on;
X}
X
X/* ...and maybe keypad application mode is a toggle, too:
X */
XKeypadMode (on) {
X    if (on) {
X	if (!keypad)
X	    PutStr (KS);
X    } else if (keypad)
X	PutStr (KE);
X    keypad = on;
X}
X
Xstatic SaveCursor () {
X    curr->saved = 1;
X    curr->Saved_x = curr->x;
X    curr->Saved_y = curr->y;
X    curr->SavedLocalAttr = curr->LocalAttr;
X}
X
Xstatic RestoreCursor () {
X    if (curr->saved) {
X	curr->LocalAttr = curr->SavedLocalAttr;
X	NewRendition (GlobalAttr, curr->LocalAttr);
X	GlobalAttr = curr->LocalAttr;
X	Goto (curr->y, curr->x, curr->Saved_y, curr->Saved_x);
X	curr->x = curr->Saved_x;
X	curr->y = curr->Saved_y;
X    }
X}
X
X/*ARGSUSED*/
XCountChars (c) {
X    StrCost++;
X}
X
XCalcCost (s) register char *s; {
X    if (s) {
X	StrCost = 0;
X	tputs (s, 1, CountChars);
X	return StrCost;
X    } else return EXPENSIVE;
X}
X
Xstatic Goto (y1, x1, y2, x2) {
X    register dy, dx;
X    register cost = 0;
X    register char *s;
X    int CMcost, n, m;
X    enum move_t xm = M_NONE, ym = M_NONE;
X
X    if (!display)
X	return;
X    if (x1 == cols || x2 == cols) {
X	if (x2 == cols) --x2;
X	goto DoCM;
X    }
X    dx = x2 - x1;
X    dy = y2 - y1;
X    if (dy == 0 && dx == 0)
X	return;
X    if (y1 == -1 || x1 == -1) {
XDoCM:
X	PutStr (tgoto (CM, x2, y2));
X	return;
X    }
X    CMcost = CalcCost (tgoto (CM, x2, y2));
X    if (dx > 0) {
X	if ((n = RewriteCost (y1, x1, x2)) < (m = dx * NDcost)) {
X	    cost = n;
X	    xm = M_RW;
X	} else {
X	    cost = m;
X	    xm = M_RI;
X	}
X    } else if (dx < 0) {
X	cost = -dx * LEcost;
X	xm = M_LE;
X    }
X    if (dx && (n = RewriteCost (y1, 0, x2) + CRcost) < cost) {
X	cost = n;
X	xm = M_CR;
X    }
X    if (cost >= CMcost)
X	goto DoCM;
X    if (dy > 0) {
X	cost += dy * DOcost;
X	ym = M_DO;
X    } else if (dy < 0) {
X	cost += -dy * UPcost;
X	ym = M_UP;
X    }
X    if (cost >= CMcost)
X	goto DoCM;
X    if (xm != M_NONE) {
X	if (xm == M_LE || xm == M_RI) {
X	    if (xm == M_LE) {
X		s = BC; dx = -dx;
X	    } else s = ND;
X	    while (dx-- > 0)
X		PutStr (s);
X	} else {
X	    if (xm == M_CR) {
X		PutStr (CR);
X		x1 = 0;
X	    }
X	    if (x1 < x2) {
X		if (curr->insert)
X		    InsertMode (0);
X		for (s = curr->image[y1]+x1; x1 < x2; x1++, s++)
X		    putchar (*s);
X		if (curr->insert)
X		    InsertMode (1);
X	    }
X	}
X    }
X    if (ym != M_NONE) {
X	if (ym == M_UP) {
X	    s = UP; dy = -dy;
X	} else s = DO;
X	while (dy-- > 0)
X	    PutStr (s);
X    }
X}
X
Xstatic RewriteCost (y, x1, x2) {
X    register cost, dx;
X    register char *p = curr->attr[y]+x1;
X
X    if (AM && y == rows-1 && x2 == cols-1)
X	return EXPENSIVE;
X    cost = dx = x2 - x1;
X    if (dx == 0)
X	return 0;
X    if (curr->insert)
X	cost += EIcost + IMcost;
X    do {
X	if (*p++ != GlobalAttr)
X	    return EXPENSIVE;
X    } while (--dx);
X    return cost;
X}
X
Xstatic BackSpace () {
X    if (curr->x > 0) {
X	if (curr->x < cols) {
X	    if (BC)
X		PutStr (BC);
X	    else
X		Goto (curr->y, curr->x, curr->y, curr->x-1);
X	}
X	curr->x--;
X    }
X}
X
Xstatic Return () {
X    if (curr->x > 0) {
X	curr->x = 0;
X	PutStr (CR);
X    }
X}
X
Xstatic LineFeed () {
X    if (curr->y == curr->bot) {
X	ScrollUpMap (curr->image);
X	ScrollUpMap (curr->attr);
X    } else if (curr->y < rows-1) {
X	curr->y++;
X    }
X    PutStr (NL);
X}
X
Xstatic ReverseLineFeed () {
X    if (curr->y == curr->top) {
X	ScrollDownMap (curr->image);
X	ScrollDownMap (curr->attr);
X	if (SR) {
X	    PutStr (SR);
X	} else if (AL) {
X	    Goto (curr->top, curr->x, curr->top, 0);
X	    PutStr (AL);
X	    Goto (curr->top, 0, curr->top, curr->x);
X	} else Redisplay ();
X    } else if (curr->y > 0) {
X	CursorUp (1);
X    }
X}
X
Xstatic InsertAChar (c) {
X    register y = curr->y, x = curr->x;
X
X    if (x == cols)
X	x--;
X    bcopy (curr->image[y], OldImage, cols);
X    bcopy (curr->attr[y], OldAttr, cols);
X    bcopy (curr->image[y]+x, curr->image[y]+x+1, cols-x-1);
X    bcopy (curr->attr[y]+x, curr->attr[y]+x+1, cols-x-1);
X    SetChar (c);
X    if (!display)
X	return;
X    if (IC || IM) {
X	if (!curr->insert)
X	    InsertMode (1);
X	PutStr (IC);
X	putchar (c);
X	if (!curr->insert)
X	    InsertMode (0);
X    } else {
X	RedisplayLine (OldImage, OldAttr, y, x, cols-1);
X	++x;
X	Goto (y, last_x, y, x);
X    }
X}
X
Xstatic InsertChar (n) {
X    register i, y = curr->y, x = curr->x;
X
X    if (x == cols)
X	return;
X    bcopy (curr->image[y], OldImage, cols);
X    bcopy (curr->attr[y], OldAttr, cols);
X    if (n > cols-x)
X	n = cols-x;
X    bcopy (curr->image[y]+x, curr->image[y]+x+n, cols-x-n);
X    bcopy (curr->attr[y]+x, curr->attr[y]+x+n, cols-x-n);
X    ClearInLine (0, y, x, x+n-1);
X    if (!display)
X	return;
X    if (IC || IM) {
X	if (!curr->insert)
X	    InsertMode (1);
X	for (i = n; i; i--) {
X	    PutStr (IC);
X	    putchar (' ');
X	}
X	if (!curr->insert)
X	    InsertMode (0);
X	Goto (y, x+n, y, x);
X    } else {
X	RedisplayLine (OldImage, OldAttr, y, x, cols-1);
X	Goto (y, last_x, y, x);
X    }
X}
X
Xstatic DeleteChar (n) {
X    register i, y = curr->y, x = curr->x;
X
X    if (x == cols)
X	return;
X    bcopy (curr->image[y], OldImage, cols);
X    bcopy (curr->attr[y], OldAttr, cols);
X    if (n > cols-x)
X	n = cols-x;
X    bcopy (curr->image[y]+x+n, curr->image[y]+x, cols-x-n);
X    bcopy (curr->attr[y]+x+n, curr->attr[y]+x, cols-x-n);
X    ClearInLine (0, y, cols-n, cols-1);
X    if (!display)
X	return;
X    if (DC) {
X	for (i = n; i; i--)
X	    PutStr (DC);
X    } else {
X	RedisplayLine (OldImage, OldAttr, y, x, cols-1);
X	Goto (y, last_x, y, x);
X    }
X}
X
Xstatic DeleteLine (n) {
X    register i, old = curr->top;
X
X    if (n > curr->bot-curr->y+1)
X	n = curr->bot-curr->y+1;
X    curr->top = curr->y;
X    for (i = n; i; i--) {
X	ScrollUpMap (curr->image);
X	ScrollUpMap (curr->attr);
X    }
X    if (DL) {
X	Goto (curr->y, curr->x, curr->y, 0);
X	for (i = n; i; i--)
X	    PutStr (DL);
X	Goto (curr->y, 0, curr->y, curr->x);
X    } else if (CS) {
X	PutStr (tgoto (CS, curr->bot, curr->top));
X	Goto (-1, -1, curr->bot, 0);
X	for (i = n; i; i--)
X	    PutStr (SF);
X	PutStr (tgoto (CS, curr->bot, old));
X	Goto (-1, -1, curr->y, curr->x);
X    } else Redisplay ();
X    curr->top = old;
X}
X
Xstatic InsertLine (n) {
X    register i, old = curr->top;
X
X    if (n > curr->bot-curr->y+1)
X	n = curr->bot-curr->y+1;
X    curr->top = curr->y;
X    for (i = n; i; i--) {
X	ScrollDownMap (curr->image);
X	ScrollDownMap (curr->attr);
X    }
X    if (AL) {
X	Goto (curr->y, curr->x, curr->y, 0);
X	for (i = n; i; i--)
X	    PutStr (AL);
X	Goto (curr->y, 0, curr->y, curr->x);
X    } else if (CS && SR) {
X	PutStr (tgoto (CS, curr->bot, curr->top));
X	Goto (-1, -1, curr->y, 0);
X	for (i = n; i; i--)
X	    PutStr (SR);
X	PutStr (tgoto (CS, curr->bot, old));
X	Goto (-1, -1, curr->y, curr->x);
X    } else Redisplay ();
X    curr->top = old;
X}
X
Xstatic ScrollUpMap (pp) char **pp; {
X    register char *tmp = pp[curr->top];
X
X    bcopy (pp+curr->top+1, pp+curr->top,
X	(curr->bot-curr->top) * sizeof (char *));
X    if (pp == curr->image)
X	bclear (tmp, cols);
X    else
X	bzero (tmp, cols);
X    pp[curr->bot] = tmp;
X}
X
Xstatic ScrollDownMap (pp) char **pp; {
X    register char *tmp = pp[curr->bot];
X
X    bcopy (pp+curr->top, pp+curr->top+1,
X	(curr->bot-curr->top) * sizeof (char *));
X    if (pp == curr->image)
X	bclear (tmp, cols);
X    else
X	bzero (tmp, cols);
X    pp[curr->top] = tmp;
X}
X
Xstatic ForwardTab () {
X    register x = curr->x;
X
X    if (curr->tabs[x] && x < cols-1)
X	++x;
X    while (x < cols-1 && !curr->tabs[x])
X	x++;
X    Goto (curr->y, curr->x, curr->y, x);
X    curr->x = x;
X}
X
Xstatic BackwardTab () {
X    register x = curr->x;
X
X    if (curr->tabs[x] && x > 0)
X	x--;
X    while (x > 0 && !curr->tabs[x])
X	x--;
X    Goto (curr->y, curr->x, curr->y, x);
X    curr->x = x;
X}
X
Xstatic ClearScreen () {
X    register i;
X
X    PutStr (CL);
X    for (i = 0; i < rows; ++i) {
X	bclear (curr->image[i], cols);
X	bzero (curr->attr[i], cols);
X    }
X}
X
Xstatic ClearFromBOS () {
X    register n, y = curr->y, x = curr->x;
X
X    for (n = 0; n < y; ++n)
X	ClearInLine (1, n, 0, cols-1);
X    ClearInLine (1, y, 0, x);
X    Goto (curr->y, curr->x, y, x);
X    curr->y = y; curr->x = x;
X}
X
Xstatic ClearToEOS () {
X    register n, y = curr->y, x = curr->x;
X
X    if (CD)
X	PutStr (CD);
X    ClearInLine (!CD, y, x, cols-1);
X    for (n = y+1; n < rows; n++)
X	ClearInLine (!CD, n, 0, cols-1);
X    Goto (curr->y, curr->x, y, x);
X    curr->y = y; curr->x = x;
X}
X
Xstatic ClearLine () {
X    register y = curr->y, x = curr->x;
X
X    ClearInLine (1, y, 0, cols-1);
X    Goto (curr->y, curr->x, y, x);
X    curr->y = y; curr->x = x;
X}
X
Xstatic ClearToEOL () {
X    register y = curr->y, x = curr->x;
X
X    ClearInLine (1, y, x, cols-1);
X    Goto (curr->y, curr->x, y, x);
X    curr->y = y; curr->x = x;
X}
X
Xstatic ClearFromBOL () {
X    register y = curr->y, x = curr->x;
X
X    ClearInLine (1, y, 0, x);
X    Goto (curr->y, curr->x, y, x);
X    curr->y = y; curr->x = x;
X}
X
Xstatic ClearInLine (displ, y, x1, x2) {
X    register i, n;
X
X    if (x1 == cols) x1--;
X    if (x2 == cols) x2--;
X    if (n = x2 - x1 + 1) {
X	bclear (curr->image[y]+x1, n);
X	bzero (curr->attr[y]+x1, n);
X	if (displ && display) {
X	    if (x2 == cols-1 && CE) {
X		Goto (curr->y, curr->x, y, x1);
X		curr->y = y; curr->x = x1;
X		PutStr (CE);
X		return;
X	    }
X	    if (y == rows-1 && AM)
X		--n;
X	    if (n == 0)
X		return;
X	    SaveAttr ();
X	    Goto (curr->y, curr->x, y, x1);
X	    for (i = n; i > 0; i--)
X		putchar (' ');
X	    curr->y = y; curr->x = x1 + n;
X	    RestoreAttr ();
X	}
X    }
X}
X
Xstatic CursorRight (n) register n; {
X    register x = curr->x;
X
X    if (x == cols)
X	return;
X    if ((curr->x += n) >= cols)
X	curr->x = cols-1;
X    Goto (curr->y, x, curr->y, curr->x);
X}
X
Xstatic CursorUp (n) register n; {
X    register y = curr->y;
X
X    if ((curr->y -= n) < curr->top)
X	curr->y = curr->top;
X    Goto (y, curr->x, curr->y, curr->x);
X}
X
Xstatic CursorDown (n) register n; {
X    register y = curr->y;
X
X    if ((curr->y += n) > curr->bot)
X	curr->y = curr->bot;
X    Goto (y, curr->x, curr->y, curr->x);
X}
X
Xstatic CursorLeft (n) register n; {
X    register x = curr->x;
X
X    if ((curr->x -= n) < 0)
X	curr->x = 0;
X    Goto (curr->y, x, curr->y, curr->x);
X}
X
Xstatic SetMode (on) {
X    register i;
X
X    for (i = 0; i < NumArgs; ++i) {
X	switch (args[i]) {
X	case 4:
X	    curr->insert = on;
X	    InsertMode (on);
X	    break;
X	}
X    }
X}
X
Xstatic SelectRendition () {
X    register i, old = GlobalAttr;
X
X    if (NumArgs == 0)
X	SetRendition (0);
X    else for (i = 0; i < NumArgs; ++i)
X	SetRendition (args[i]);
X    NewRendition (old, GlobalAttr);
X}
X
Xstatic SetRendition (n) register n; {
X    switch (n) {
X    case 0:
X	GlobalAttr = 0; break;
X    case 1:
X	GlobalAttr |= A_BD; break;
X    case 2:
X	GlobalAttr |= A_DI; break;
X    case 3:
X	GlobalAttr |= A_SO; break;
X    case 4:
X	GlobalAttr |= A_US; break;
X    case 5:
X	GlobalAttr |= A_BL; break;
X    case 7:
X	GlobalAttr |= A_RV; break;
X    case 22:
X	GlobalAttr &= ~(A_BD|A_SO|A_DI); break;
X    case 23:
X	GlobalAttr &= ~A_SO; break;
X    case 24:
X	GlobalAttr &= ~A_US; break;
X    case 25:
X	GlobalAttr &= ~A_BL; break;
X    case 27:
X	GlobalAttr &= ~A_RV; break;
X    }
X    curr->LocalAttr = GlobalAttr;
X}
X
Xstatic NewRendition (old, new) register old, new; {
X    register i;
X
X    if (old == new)
X	return;
X    for (i = 1; i <= A_MAX; i <<= 1) {
X	if ((old & i) && !(new & i)) {
X	    PutStr (UE);
X	    PutStr (SE);
X	    PutStr (ME);
X	    if (new & A_US) PutStr (US);
X	    if (new & A_SO) PutStr (SO);
X	    if (new & A_BL) PutStr (MB);
X	    if (new & A_BD) PutStr (MD);
X	    if (new & A_DI) PutStr (MH);
X	    if (new & A_RV) PutStr (MR);
X	    return;
X	}
X    }
X    if ((new & A_US) && !(old & A_US))
X	PutStr (US);
X    if ((new & A_SO) && !(old & A_SO))
X	PutStr (SO);
X    if ((new & A_BL) && !(old & A_BL))
X	PutStr (MB);
X    if ((new & A_BD) && !(old & A_BD))
X	PutStr (MD);
X    if ((new & A_DI) && !(old & A_DI))
X	PutStr (MH);
X    if ((new & A_RV) && !(old & A_RV))
X	PutStr (MR);
X}
X
Xstatic SaveAttr () {
X    NewRendition (GlobalAttr, 0);
X    if (curr->insert)
X	InsertMode (0);
X}
X
Xstatic RestoreAttr () {
X    NewRendition (0, GlobalAttr);
X    if (curr->insert)
X	InsertMode (1);
X}
X
Xstatic FillWithEs () {
X    register i;
X    register char *p, *ep;
X
X    curr->y = curr->x = 0;
X    NewRendition (GlobalAttr, 0);
X    SetRendition (0);
X    for (i = 0; i < rows; ++i) {
X	bzero (curr->attr[i], cols);
X	p = curr->image[i];
X	ep = p + cols;
X	for ( ; p < ep; ++p)
X	    *p = 'E';
X    }
X    Redisplay ();
X}
X
Xstatic Redisplay () {
X    register i;
X
X    PutStr (CL);
X    TmpAttr = GlobalAttr;
X    last_x = last_y = 0;
X    for (i = 0; i < rows; ++i)
X	DisplayLine (blank, null, curr->image[i], curr->attr[i], i, 0, cols-1);
X    NewRendition (TmpAttr, GlobalAttr);
X    Goto (last_y, last_x, curr->y, curr->x);
X}
X
Xstatic DisplayLine (os, oa, s, as, y, from, to)
X	register char *os, *oa, *s, *as; {
X    register i, x, a;
X
X    if (to == cols)
X	--to;
X    if (AM && y == rows-1 && to == cols-1)
X	--to;
X    a = TmpAttr;
X    for (x = i = from; i <= to; ++i, ++x) {
X	if (s[i] == os[i] && as[i] == oa[i] && as[i] == a)
X	    continue;
X	Goto (last_y, last_x, y, x);
X	last_y = y;
X	last_x = x;
X	if ((a = as[i]) != TmpAttr) {
X	    NewRendition (TmpAttr, a);
X	    TmpAttr = a;
X	}
X	putchar (s[i]);
X	last_x++;
X    }
X}
X
Xstatic RedisplayLine (os, oa, y, from, to) char *os, *oa; {
X    if (curr->insert)
X	InsertMode (0);
X    NewRendition (GlobalAttr, 0);
X    TmpAttr = 0;
X    last_y = y;
X    last_x = from;
X    DisplayLine (os, oa, curr->image[y], curr->attr[y], y, from, to);
X    NewRendition (TmpAttr, GlobalAttr);
X    if (curr->insert)
X	InsertMode (1);
X}
X
Xstatic MakeBlankLine (p, n) register char *p; register n; {
X    do *p++ = ' ';
X    while (--n);
X}
X
XMakeStatus (msg, wp) char *msg; struct win *wp; {
X    struct win *ocurr = curr;
X    int odisplay = display;
X    register char *s, *t;
X    register max = AM ? cols-1 : cols;
X
X    for (s = t = msg; *s && t - msg < max; ++s)
X	if (*s >= ' ' && *s <= '~')
X	    *t++ = *s;
X    *t = '\0';
X    curr = wp;
X    display = 1;
X    if (status) {
X	if (time ((time_t *)0) - TimeDisplayed < 2)
X	    sleep (1);
X	RemoveStatus (wp);
X    }
X    if (t > msg) {
X	status = 1;
X	StatLen = t - msg;
X	Goto (curr->y, curr->x, rows-1, 0);
X	NewRendition (GlobalAttr, A_SO);
X	if (curr->insert)
X	    InsertMode (0);
X	printf ("%s", msg);
X	if (curr->insert)
X	    InsertMode (1);
X	NewRendition (A_SO, GlobalAttr);
X	fflush (stdout);
X	time (&TimeDisplayed);
X    }
X    curr = ocurr;
X    display = odisplay;
X}
X
XRemoveStatus (p) struct win *p; {
X    struct win *ocurr = curr;
X    int odisplay = display;
X
X    if (!status)
X	return;
X    status = 0;
X    curr = p;
X    display = 1;
X    Goto (-1, -1, rows-1, 0);
X    RedisplayLine (null, null, rows-1, 0, StatLen);
X    Goto (rows-1, last_x, curr->y, curr->x);
X    curr = ocurr;
X    display = odisplay;
X}
*-*-END-of-ansi.c-*-*
echo x - screen.1
sed 's/^X//' >screen.1 <<'*-*-END-of-screen.1-*-*'
X.if n .ds Q \&"
X.if n .ds U \&"
X.if t .ds Q ``
X.if t .ds U ''
X.TH SCREEN 1 "2 March 1987"
X.UC 4
X.SH NAME
Xscreen \- screen manager with VT100/ANSI terminal emulation
X.SH SYNOPSIS
X.B screen
X[
X.B \-a
X] [
X.B \-f
X] [
X.B \-n
X] [
X.B \-e\fIxy\fP
X] [
X.B \fIcmd args\fP ]
X.ta .5i 1.8i
X.SH DESCRIPTION
X.I screen
Xis a full-screen window manager that
Xmultiplexes a physical terminal between several processes (typically
Xinteractive shells).  Each virtual terminal provides the functions
Xof the DEC VT100 terminal and, in addition, several control functions
Xfrom the ANSI X3.64 (ISO 6429) standard (e.g. insert/delete line).
X.PP
XWhen
X.I screen
Xis called, it creates a single window with a shell; the pathname of the
Xshell is taken from the environment symbol $SHELL; if this is not
Xdefined, \*Q/bin/sh\*U is used.
XNew windows can be created at any time by calling
X.I screen
Xfrom within a previously created window.
XThe program to be started in a newly created
Xwindow and optional arguments to the program can be supplied when
X.I screen
Xis invoked.
XFor instance,
X.IP
Xscreen csh
X.PP
Xwill create a window with a C-Shell and switch to that window.
XWhen the process associated with the currently displayed window
Xterminates (e.g. ^D has been typed to a shell),
X.I screen
Xswitches to the previously displayed window;
Xwhen no more windows are left,
X.I screen
Xexits.
X.PP
XWhen \*Q/etc/utmp\*U is writable by
X.IR screen ,
Xan appropriate record is written to this file for each window and
Xremoved when the window is terminated.
X.SH "COMMAND KEYS"
XThe standard way to create a new window is to type \*QC-a c\*U (the notation
X\*QC-x\*U will be used as a shorthand for Control-x in this manual; x is
Xan arbitrary letter).
X\*QC-a c\*U creates a new window running a shell and switches to that
Xwindow immediately, regardless of the state of the process running
Xin the current window.
X.I Screen
Xrecognizes several such commands; each command consists of
X\*QC-a\*U followed by a one-letter function.
XFor convenience, the letter after a \*QC-a\*U can be entered both with or
Xwithout the control key pressed (with the exception of
X\*QC-a C-a\*U and \*QC-a a\*U; see below), thus, \*QC-a c\*U as well as
X\*QC-a C-c\*U can be used to create a window.
X.PP
XThe following commands are recognized by
X.IR screen :
X.IP "\fBC-a c\fP or \fBC-a C-c\fP"
XCreate a new window with a shell and switch to that window.
X.IP "\fBC-a k\fP or \fBC-a C-k\fP"
XKill the current window and switch to the previously displayed window.
X.IP "\fBC-a C-\e\fP"
XKill all windows and terminate
X.IR screen .
X.IP "\fBC-a C-a\fP\0\0\0\0\0"
XSwitch to the previously displayed window.
X.IP "\fBC-a 0\fP to \fBC-a 9\fP"
XSwitch to the window with the number 0 (1, 2, .., 9, respectively).
XWhen a new window is established, the first available number from the
Xrange 0..9 is assigned to this window.
XThus, the first window can be activated by \*QC-a 0\*U; at most
X10 windows can be present at any time.
X.IP "\fBC-a space\fP or \fBC-a C-space\fP or \fBC-a n\fP or \fBC-a C-n\fP"
XSwitch to the next window.  This function can be used repeatedly to
Xcycle through the list of windows.
X(Control-space is not supported by all terminals.)
X.IP "\fBC-a p\fP or \fBC-a C-p\fP or \fBC-a -\fP"
XSwitch to the previous window (the opposite of \fBC-a space\fP).
X.IP "\fBC-a l\fP or \fBC-a C-l\fP"
XRedisplay the current window.
X.IP "\fBC-a z\fP or \fBC-a C-z\fP"
XSuspend
X.IR screen .
X.IP "\fBC-a h\fP or \fBC-a C-h\fP"
XWrite a hardcopy of the current window to the file \*Qhardcopy.\fIn\fP\*U
Xin the window's current directory,
Xwhere \fIn\fP is the number of the current window.
X.IP "\fBC-a .\fP (Control-a dot)"
XWrite the termcap entry for the virtual terminal of the currently active
Xwindow to the file \*Q.termcap\*U in the directory \*Q$HOME/.screen\*U.
XThis termcap entry is identical to the value of the environment symbol
XTERMCAP that is set up by
X.I screen
Xfor each window.
X.IP "\fBC-a w\fP or \fBC-a C-w\fP"
XDisplay a list of all windows.
XFor each window, the number of the window and the process that has been
Xstarted in the window is displayed; the current window is marked with a
X`*'.
X.IP "\fBC-a t\fP or \fBC-a C-t\fP"
XPrint in the message line the time of day, the host name, the load averages
Xover 1, 5, and 15 minutes (if this is available on your system),
Xthe cursor position of the current window in the form \*Q(colum,row)\*U
Xstarting with \*U(0,0)\*U, and an indication if flow control
Xand (for the current window)
Xinsert mode, origin mode, wrap mode, and keypad application
Xmode are enabled or not (indicated by a '+' or '-').
X.IP "\fBC-a v\fP or \fBC-a C-v\fP"
XDisplay the version.
X.IP "\fBC-a a\fP\0\0\0\0\0"
XSend the character \*QC-a\*U to the processes running in the window.
X.IP "\fBC-a s\fP or \fBC-a C-s\fP"
XSend a Control-s to the program running in the window.
X.IP "\fBC-a q\fP or \fBC-a C-q\fP"
XSend a Control-q to the program running in the window.
X.IP
X.PP
XThe
X.B -e
Xoption can be used to specify a different command character and
Xa character which, when typed immediately after the command character,
Xgenerates a literal command character.
XThe defaults for these two characters are \*QC-a\*U and `a'.
X(Note that the function to switch to the previous window is actually the
Xcommand character typed twice; for instance, when
X.I screen
Xis called with the option \*Q\fB-e]x\fP\*U (or \*Q\fB-e ]x\fP\*U),
Xthis function becomes \*Q]]\*U).
X.SH CUSTOMIZATION
XWhen
X.I screen
Xis invoked, it executes initialization commands from the file
X\*Q.screenrc\*U in the user's home directory.
XCommands in \*Q.screenrc\*U are mainly used to automatically
Xestablish a number of windows each time
X.I screen
Xis called, and to bind functions to specific keys.
XEach line in \*Q.screenrc\*U contains one initialization command; lines
Xstarting with `#' are ignored.
XCommands can have arguments; arguments are separated by tabs and spaces
Xand can be surrounded by single quotes or double quotes.
X.PP
XThe following initialization commands are recognized by
X.IR screen :
X.PP
X.ne 3
X.B "escape \fIxy\fP"
X.PP
XSet the command character to \fIx\fP and the character generating a literal
Xcommand character to \fIy\fP (see the -e option above).
X.PP
X.ne 3
X.B "bell \fImessage\fP"
X.PP
XWhen a bell character is sent to a background window,
X.I screen
Xdisplays a notification in the message line (see below).
XThe notification message can be re-defined by means of the \*Qbell\*U
Xcommand; each occurrence of `%' in \fImessage\fP is replaced by
Xthe number of the window to which a bell has been sent.
XThe default message is
X.PP
X	Bell in window %
X.PP
XAn empty message can be supplied to the \*Qbell\*U command to suppress
Xoutput of a message line (bell "").
X.PP
X.ne 3
X.B "screen [\fIn\fP] [\fIcmds args\fP]"
X.PP
XEstablish a window.
XIf an optional number \fIn\fP in the range 0..9 is given, the window
Xnumber \fIn\fP is assigned to the newly created window (or, if this
Xnumber is already in use, the next higher number).
XNote that \fIn\fP has a value of zero for the standard shell window
Xcreated after \*Q.screenrc\*U has been read.
XIf a command is specified after \*Qscreen\*U, this command (with the given
Xarguments) is started in the window; if no command is given, a shell
Xis created in the window.
XThus, if your \*Q.screenrc\*U contains the lines
X.PP
X.nf
X	# example for .screenrc:
X	screen 1
X	screen 2 telnet foobar
X.fi
X.PP
X.I screen
Xcreates a shell window (window #1), a window with a TELNET connection
Xto the machine foobar (window #2), and, finally, a second shell window
X(the default window) which gets a window number of zero.
XWhen the initialization is completed,
X.I screen
Xalways switches to the default window, so window #0 is displayed
Xwhen the above \*Q.screenrc\*U is used.
X.PP
X.ne 3
X.B "chdir [\fIdirectory\fP]"
X.PP
XChange the \fIcurrent directory\fP of
X.I screen
Xto the specified directory or, if called without an argument,
Xto the home directory (the value of the environment symbol $HOME).
XAll windows that are created by means of the \*Qscreen\*U command
Xfrom within \*Q.screenrc\*U or by means of \*QC-a c'' are running
Xin the \fIcurrent directory\fP; the \fIcurrent directory\fP is
Xinitially the directory from which the shell command
X.I screen
Xhas been invoked.
XHardcopy files are always written to the directory in which the current
Xwindow has been created (that is, \fInot\fP in the current directory
Xof the shell running in the window).
X.PP
X.ne 3
X.B "bind \fIkey\fP [\fIfunction\fP | \fIcmd args\fP]"
X.PP
XBind a function to a key.
XBy default, each function provided by
X.I screen
Xis bound to one or more keys as indicated by the above table, e.g. the
Xfunction to create a new window is bound to \*QC-c\*U and \*Qc\*U.
XThe \*Qbind\*U command can be used to redefine the key bindings and to
Xdefine new bindings.
XThe \fIkey\fP
Xargument is either a single character, a sequence of the form
X\*Q^x\*U meaning \*QC-x\*U, or an octal number specifying the
XASCII code of the character.
XIf no further argument is given, any previously established binding
Xfor this key is removed.
XThe \fIfunction\fP argument can be one of the following keywords:
X.PP
X.nf
X	shell	Create new window with a shell
X	kill	Kill the current window
X	quit	Kill all windows and terminate
X	other	Switch to previously displayed window
X	next	Switch to the next window
X	prev	Switch to the previous window
X	redisplay	Redisplay current window
X	hardcopy	Make hardcopy of current window
X	termcap	Write termcap entry to $HOME/.screen/.termcap
X	suspend	Suspend \fIscreen\fP
X	windows	Display list of window
X	info	Print useful information in the message line
X	xon	Send Control-q
X	xoff	Send Control-s
X	version	Display the version
X	select0	Switch to window #0
X	\0\0...
X	select9	Switch to window #9
X.fi
X.PP
XIn addition, a key can be bound such that a window is created running
Xa different command than the shell when that key is pressed.
XIn this case, the command optionally followed by
Xarguments must be given instead of one of the above-listed keywords.
XFor example, the commands
X.PP
X.nf
X	bind ' ' windows
X	bind ^f telnet foobar
X	bind 033 su
X.fi
X.PP
Xwould bind the space key to the function that displays a list
Xof windows (that is, the function usually invoked by \*QC-a C-w\*U
Xor \*QC-a w\*U would also be available as \*QC-a space\*U),
Xbind \*QC-f\*U to the function \*Qcreate a window with a TELNET
Xconnection to foobar\*U, and bind \*Qescape\*U to the function
Xthat creates a window with a super-user shell.
X.SH "VIRTUAL TERMINAL"
X.I Screen
Xprints error messages and other diagnostics in a \fImessage line\fP above
Xthe bottom of the screen.
XThe message line is removed when a key is pressed or, automatically,
Xafter a couple of seconds.
XThe message line facility can be used by an application running in
Xthe current window by means of the ANSI \fIPrivacy message\fP
Xcontrol sequence (for instance, from within the shell, something like
X.IP
Xecho '^[^Hello world^[\e'   (where ^[ is an \fIescape\fP)
X.PP
Xcan be used to display a message line.
X.PP
XWhen the `NF' capability is found in the termcap entry of the
Xterminal on which
X.I screen
Xhas been started, flow control is turned off for the terminal.
XThis enables the user to send XON and XOFF characters to the
Xprogram running in a window (this is required by the \fIemacs\fP
Xeditor, for instance).
XThe command line options 
X.B -n
Xand
X.B -f
Xcan be used to turn flow control off or on, respectively, independently
Xof the `NF' capability.
X.PP
X.I
XScreen
Xnever writes in the last position of the screen, unless the boolean
Xcapability `LP' is found in the termcap entry of the terminal.
XUsually,
X.I screen
Xcannot predict whether or not a particular terminal scrolls when
Xa character is written in the last column of the last line;
X`LP' indicates that it is safe to write in this position.
XNote that the `LP' capability is independent of `am' (automatic
Xmargins); for certain terminals, such as the VT100, it is reasonable
Xto set `am' as well as `LP' in the corresponding termcap entry
X(the VT100 does not move the cursor when a character is written in
Xthe last column of each line).
X.PP
X.I Screen
Xputs into the environment of each process started in a newly created
Xwindow the symbols \*QWINDOW=\fIn\fP\*U (where \fIn\fP is the number
Xof the respective window), \*QTERM=screen\*U, and a TERMCAP variable
Xreflecting the capabilities of the virtual terminal emulated by
X.IR screen .
XThe actual set of capabilities supported by the virtual terminal
Xdepends on the capabilities supported by the physical terminal.
XIf, for instance, the physical terminal does not support standout mode,
X.I screen
Xdoes not put the `so' and `se' capabilities into the window's TERMCAP
Xvariable, accordingly. 
XHowever, a minimum number of capabilities must be supported by a
Xterminal in order to run
X.IR screen ,
Xnamely scrolling, clear screen, and direct cursor addressing
X(in addition,
X.I screen
Xdoes not run on hardcopy terminals or on terminals that overstrike).
X.PP
XSome capabilities are only put into the TERMCAP
Xvariable of the virtual terminal if they can be efficiently
Ximplemented by the physical terminal.
XFor instance, `dl' (delete line) is only put into the TERMCAP
Xvariable if the terminal supports either delete line itself or
Xscrolling regions.
XIf
X.I screen
Xis called with the
X.B -a
Xoption, \fIall\fP capabilities are put into the environment,
Xeven if
X.I screen
Xmust redraw parts of the display in order to implement a function.
X.PP
XThe following is a list of control sequences recognized by
X.IR screen .
X\*Q(V)\*U and \*Q(A)\*U indicate VT100-specific and ANSI-specific
Xfunctions, respectively.
X.PP
X.nf
X.TP 20
X.B "ESC E"
X	Next Line
X.TP 20
X.B "ESC D"
X	Index
X.TP 20
X.B "ESC M"
X	Reverse Index
X.TP 20
X.B "ESC H"
X	Horizontal Tab Set
X.TP 20
X.B "ESC 7"
X(V)	Save Cursor and attributes
X.TP 20
X.B "ESC 8"
X(V)	Restore Cursor and Attributes
X.TP 20
X.B "ESC c"
X	Reset to Initial State
X.TP 20
X.B "ESC ="
X(V)	Application Keypad Mode
X.TP 20
X.B "ESC >"
X(V)	Numeric Keypad Mode
X.TP 20
X.B "ESC # 8"
X(V)	Fill Screen with E's
X.TP 20
X.B "ESC \e"
X(A)	String Terminator
X.TP 20
X.B "ESC ^"
X(A)	Privacy Message (Message Line)
X.TP 20
X.B "ESC P"
X(A)	Device Control String (not used)
X.TP 20
X.B "ESC _"
X(A)	Application Program Command (not used)
X.TP 20
X.B "ESC ]"
X(A)	Operating System Command (not used)
X.TP 20
X.B "ESC [ Pn ; Pn H"
X	Direct Cursor Addressing
X.TP 20
X.B "ESC [ Pn ; Pn f"
X	Direct Cursor Addressing
X.TP 20
X.B "ESC [ Pn J"
X	Erase in Display
X.TP 20
X\h'\w'ESC 'u'Pn = None or \fB0\fP
X	From Cursor to End of Screen
X.TP 20
X\h'\w'ESC 'u'\fB1\fP
X	From Beginning of Screen to Cursor
X.TP 20
X\h'\w'ESC 'u'\fB2\fP
X	Entire Screen
X.TP 20
X.B "ESC [ Pn K"
X	Erase in Line
X.TP 20
X\h'\w'ESC 'u'Pn = None or \fB0\fP
X	From Cursor to End of Line
X.TP 20
X\h'\w'ESC 'u'\fB1\fP
X	From Beginning of Line to Cursor
X.TP 20
X\h'\w'ESC 'u'\fB2\fP
X	Entire Line
X.TP 20
X.B "ESC [ Pn A"
X	Cursor Up
X.TP 20
X.B "ESC [ Pn B"
X	Cursor Down
X.TP 20
X.B "ESC [ Pn C"
X	Cursor Right
X.TP 20
X.B "ESC [ Pn D"
X	Cursor Left
X.TP 20
X.B "ESC [ Ps ;...; Ps m"
X	Select Graphic Rendition
X.TP 20
X\h'\w'ESC 'u'Ps = None or \fB0\fP
X	Default Rendition
X.TP 20
X\h'\w'ESC 'u'\fB1\fP
X	Bold
X.TP 20
X\h'\w'ESC 'u'\fB2\fP
X(A)	Faint
X.TP 20
X\h'\w'ESC 'u'\fB3\fP
X(A)	\fIStandout\fP Mode (ANSI: Italicised)
X.TP 20
X\h'\w'ESC 'u'\fB4\fP
X	Underlined
X.TP 20
X\h'\w'ESC 'u'\fB5\fP
X	Blinking
X.TP 20
X\h'\w'ESC 'u'\fB7\fP
X	Negative Image
X.TP 20
X\h'\w'ESC 'u'\fB22\fP
X(A)	Normal Intensity
X.TP 20
X\h'\w'ESC 'u'\fB23\fP
X(A)	\fIStandout\fP Mode off (ANSI: Italicised off)
X.TP 20
X\h'\w'ESC 'u'\fB24\fP
X(A)	Not Underlined
X.TP 20
X\h'\w'ESC 'u'\fB25\fP
X(A)	Not Blinking
X.TP 20
X\h'\w'ESC 'u'\fB27\fP
X(A)	Positive Image
X.TP 20
X.B "ESC [ Pn g"
X	Tab Clear
X.TP 20
X\h'\w'ESC 'u'Pn = None or \fB0\fP
X	Clear Tab at Current Position
X.TP 20
X\h'\w'ESC 'u'\fB3\fP
X	Clear All Tabs
X.TP 20
X.B "ESC [ Pn ; Pn r"
X(V)	Set Scrolling Region
X.TP 20
X.B "ESC [ Pn I"
X(A)	Horizontal Tab
X.TP 20
X.B "ESC [ Pn Z"
X(A)	Backward Tab
X.TP 20
X.B "ESC [ Pn L"
X(A)	Insert Line
X.TP 20
X.B "ESC [ Pn M"
X(A)	Delete Line
X.TP 20
X.B "ESC [ Pn @"
X(A)	Insert Character
X.TP 20
X.B "ESC [ Pn P"
X(A)	Delete Character
X.TP 20
X.B "ESC [ Ps  ;...; Ps h"
X	Set Mode
X.TP 20
X.B "ESC [ Ps  ;...; Ps l"
X	Reset Mode
X.TP 20
X\h'\w'ESC 'u'Ps = \fB4\fP
X(A)	Insert Mode
X.TP 20
X\h'\w'ESC 'u'\fB?5\fP
X(V)	Visible Bell (\fIOn\fP followed by \fIOff\fP)
X.TP 20
X\h'\w'ESC 'u'\fB?6\fP
X(V)	\fIOrigin\fP Mode
X.TP 20
X\h'\w'ESC 'u'\fB?7\fP
X(V)	\fIWrap\fP Mode
X.fi
X.SH FILES
X.nf
X.ta 2i
X$(HOME)/.screenrc	\fIscreen\fP initialization commands
X.br
X$(HOME)/.screen	Directory created by \fIscreen\fP
X.br
X$(HOME)/.screen/\fItty\fP	Socket created by \fIscreen\fP
X.br
Xhardcopy.[0-9]	Screen images created by the hardcopy function
X.br
X/etc/termcap	Terminal capability data base
X.br
X/etc/utmp	Login records
X.br
X/etc/ttys	Terminal initialization data
X.fi
X.SH "SEE ALSO"
Xtermcap(5), utmp(5)
X.SH AUTHOR
XOliver Laumann
X.SH BUGS
XStandout mode is not cleared before newline or cursor addressing.
X.PP
XIf `LP' is not set but `am' is set, the last character in the last line is never
Xwritten, and it is not correctly re-displayed when the screen is
Xscrolled up or when a character is deleted in the last line.
X.PP
XThe VT100 \*Qwrap around with cursor addressing\*U bug is not compensated
Xwhen
X.I screen
Xis running on a VT100.
X.PP
X`AL,' `DL', and similar parameterized capabilities are not used if present.
X.PP
X`dm' (delete mode), `xn', and `xs' are not handled
Xcorrectly (they are ignored). 
X.PP
XDifferent character sets are not supported.
X.PP
X`ms' is not advertised in the termcap entry (in order to compensate
Xa bug in
X.IR curses (3X)).
X.PP
XScrolling regions are only emulated if the physical terminal supports
Xscrolling regions.
X.PP
X.I Screen
Xdoes not make use of hardware tabs.
X.PP
X.I Screen
Xmust be installed as set-uid with owner root in order to be able
Xto correctly change the owner of the tty device file for each
Xwindow.
XSpecial permission may also be required to write the file \*Q/etc/utmp\*U.
X.PP
XEntries in \*Q/etc/utmp\*U are not removed when
X.I screen
Xis killed with SIGKILL.
*-*-END-of-screen.1-*-*
echo x - screen.c
sed 's/^X//' >screen.c <<'*-*-END-of-screen.c-*-*'
X/* Copyright (c) 1987, Oliver Laumann, Technical University of Berlin.
X * Not derived from licensed software.
X *
X * Permission is granted to freely use, copy, modify, and redistribute
X * this software, provided that no attempt is made to gain profit from it,
X * the author is not construed to be liable for any results of using the
X * software, alterations are clearly marked as such, and this notice is
X * not modified.
X */
X
Xstatic char ScreenVersion[] = "screen 1.1i  1-Jul-87";
X
X#include <stdio.h>
X#include <sgtty.h>
X#include <signal.h>
X#include <errno.h>
X#include <ctype.h>
X#include <utmp.h>
X#include <pwd.h>
X#include <nlist.h>
X#include <sys/types.h>
X#include <sys/time.h>
X#include <sys/wait.h>
X#include <sys/socket.h>
X#include <sys/un.h>
X#include <sys/stat.h>
X#include <sys/file.h>
X#include "screen.h"
X
X#ifdef GETTTYENT
X#   include <ttyent.h>
X#else
X    static struct ttyent {
X	char *ty_name;
X    } *getttyent();
X    static char *tt, *ttnext;
X    static char ttys[] = "/etc/ttys";
X#endif
X
X#define MAXWIN     10
X#define MAXARGS    64
X#define MAXLINE  1024
X#define MSGWAIT     5
X
X#define Ctrl(c) ((c)&037)
X
Xextern char *blank, Term[], **environ;
Xextern rows, cols;
Xextern status;
Xextern time_t TimeDisplayed;
Xextern char AnsiVersion[];
Xextern short ospeed;
Xextern flowctl;
Xextern errno;
Xextern sys_nerr;
Xextern char *sys_errlist[];
Xextern char *index(), *rindex(), *malloc(), *getenv(), *MakeTermcap();
Xextern char *getlogin(), *ttyname();
Xstatic Finit(), SigChld();
Xstatic char *MakeBellMsg(), *Filename(), **SaveArgs();
X
Xstatic char PtyName[32], TtyName[32];
Xstatic char *ShellProg;
Xstatic char *ShellArgs[2];
Xstatic char inbuf[IOSIZE];
Xstatic inlen;
Xstatic ESCseen;
Xstatic GotSignal;
Xstatic char DefaultShell[] = "/bin/sh";
Xstatic char DefaultPath[] = ":/usr/ucb:/bin:/usr/bin";
Xstatic char PtyProto[] = "/dev/ptyXY";
Xstatic char TtyProto[] = "/dev/ttyXY";
Xstatic char SockPath[512];
Xstatic char SockDir[] = ".screen";
Xstatic char *SockName;
Xstatic char *NewEnv[MAXARGS];
Xstatic char Esc = Ctrl('a');
Xstatic char MetaEsc = 'a';
Xstatic char *home;
Xstatic HasWindow;
Xstatic utmp, utmpf;
Xstatic char UtmpName[] = "/etc/utmp";
Xstatic char *LoginName;
Xstatic char *BellString = "Bell in window %";
Xstatic mflag, nflag, fflag;
Xstatic char HostName[MAXSTR];
X#ifdef LOADAV
X    static char KmemName[] = "/dev/kmem";
X    static char UnixName[] = "/vmunix";
X    static char AvenrunSym[] = "_avenrun";
X    static struct nlist nl[2];
X    static avenrun, kmemf;
X#endif
X
Xstruct mode {
X    struct sgttyb m_ttyb;
X    struct tchars m_tchars;
X    struct ltchars m_ltchars;
X    int m_ldisc;
X    int m_lmode;
X} OldMode, NewMode;
X
Xstatic struct win *curr, *other;
Xstatic CurrNum, OtherNum;
Xstatic struct win *wtab[MAXWIN];
X
X#define MSG_CREATE    0
X#define MSG_ERROR     1
X
Xstruct msg {
X    int type;
X    union {
X	struct {
X	    int aflag;
X	    int nargs;
X	    char line[MAXLINE];
X	    char dir[1024];
X	} create;
X	char message[MAXLINE];
X    } m;
X};
X
X#define KEY_IGNORE         0
X#define KEY_HARDCOPY       1
X#define KEY_SUSPEND        2
X#define KEY_SHELL          3
X#define KEY_NEXT           4
X#define KEY_PREV           5
X#define KEY_KILL           6
X#define KEY_REDISPLAY      7
X#define KEY_WINDOWS        8
X#define KEY_VERSION        9
X#define KEY_OTHER         10
X#define KEY_0             11
X#define KEY_1             12
X#define KEY_2             13
X#define KEY_3             14
X#define KEY_4             15
X#define KEY_5             16
X#define KEY_6             17
X#define KEY_7             18
X#define KEY_8             19
X#define KEY_9             20
X#define KEY_XON           21
X#define KEY_XOFF          22
X#define KEY_INFO          23
X#define KEY_TERMCAP       24
X#define KEY_QUIT          25
X#define KEY_CREATE        26
X
Xstruct key {
X    int type;
X    char **args;
X} ktab[256];
X
Xchar *KeyNames[] = {
X    "hardcopy", "suspend", "shell", "next", "prev", "kill", "redisplay",
X    "windows", "version", "other", "select0", "select1", "select2", "select3",
X    "select4", "select5", "select6", "select7", "select8", "select9",
X    "xon", "xoff", "info", "termcap", "quit",
X    0
X};
X
Xmain (ac, av) char **av; {
X    register n, len;
X    register struct win **pp, *p;
X    char *ap;
X    int s, r, w, x = 0;
X    int aflag = 0;
X    struct timeval tv;
X    time_t now;
X    char buf[IOSIZE], *myname = (ac == 0) ? "screen" : av[0];
X    char rc[256];
X
X    while (ac > 0) {
X	ap = *++av;
X	if (--ac > 0 && *ap == '-') {
X	    switch (ap[1]) {
X	    case 'c':        /* Compatibility with older versions. */
X		break;
X	    case 'a':
X		aflag = 1;
X		break;
X	    case 'm':
X		mflag = 1;
X		break;
X	    case 'n':
X		nflag = 1;
X		break;
X	    case 'f':
X		fflag = 1;
X		break;
X	    case 'e':
X		if (ap[2]) {
X		    ap += 2;
X		} else {
X		    if (--ac == 0) goto help;
X		    ap = *++av;
X		}
X		if (strlen (ap) != 2)
X		    Msg (0, "Two characters are required with -e option.");
X		Esc = ap[0];
X		MetaEsc = ap[1];
X		break;
X	    default:
X	    help:
X		Msg (0, "Use: %s [-a] [-f] [-n] [-exy] [cmd args]", myname);
X	    }
X	} else break;
X    }
X    if (nflag && fflag)
X	Msg (0, "-f and -n are conflicting options.");
X    if ((ShellProg = getenv ("SHELL")) == 0)
X	ShellProg = DefaultShell;
X    ShellArgs[0] = ShellProg;
X    if (ac == 0) {
X	ac = 1;
X	av = ShellArgs;
X    }
X    if (GetSockName ()) {
X	/* Client */
X	s = MakeClientSocket ();
X	SendCreateMsg (s, ac, av, aflag);
X	close (s);
X	exit (0);
X    }
X    (void) gethostname (HostName, MAXSTR);
X    HostName[MAXSTR-1] = '\0';
X    if (ap = index (HostName, '.'))
X	*ap = '\0';
X    s = MakeServerSocket ();
X    InitTerm ();
X    if (fflag)
X	flowctl = 1;
X    else if (nflag)
X	flowctl = 0;
X    MakeNewEnv ();
X    GetTTY (0, &OldMode);
X    ospeed = (short)OldMode.m_ttyb.sg_ospeed;
X    InitUtmp ();
X#ifdef LOADAV
X    InitKmem ();
X#endif
X    signal (SIGHUP, Finit);
X    signal (SIGINT, Finit);
X    signal (SIGQUIT, Finit);
X    signal (SIGTERM, Finit);
X    InitKeytab ();
X    sprintf (rc, "%.*s/.screenrc", 245, home);
X    ReadRc (rc);
X    if ((n = MakeWindow (*av, av, aflag, 0, (char *)0)) == -1) {
X	SetTTY (0, &OldMode);
X	FinitTerm ();
X	exit (1);
X    }
X    SetCurrWindow (n);
X    HasWindow = 1;
X    SetMode (&OldMode, &NewMode);
X    SetTTY (0, &NewMode);
X    signal (SIGCHLD, SigChld);
X    tv.tv_usec = 0;
X    while (1) {
X	if (status) {
X	    time (&now);
X	    if (now - TimeDisplayed < MSGWAIT) {
X		tv.tv_sec = MSGWAIT - (now - TimeDisplayed);
X	    } else RemoveStatus (curr);
X	}
X	r = 0;
X	w = 0;
X	if (inlen)
X	    w |= 1 << curr->ptyfd;
X	else
X	    r |= 1 << 0;
X	for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
X	    if (!(p = *pp))
X		continue;
X	    if ((*pp)->active && status)
X		continue;
X	    if ((*pp)->outlen > 0)
X		continue;
X	    r |= 1 << (*pp)->ptyfd;
X	}
X	r |= 1 << s;
X	fflush (stdout);
X	if (select (32, &r, &w, &x, status ? &tv : (struct timeval *)0) == -1) {
X	    if (errno == EINTR)
X		continue;
X	    HasWindow = 0;
X	    Msg (errno, "select");
X	    /*NOTREACHED*/
X	}
X	if (GotSignal && !status) {
X	    SigHandler ();
X	    continue;
X	}
X	if (r & 1 << s) {
X	    RemoveStatus (curr);
X	    ReceiveMsg (s);
X	}
X	if (r & 1 << 0) {
X	    RemoveStatus (curr);
X	    if (ESCseen) {
X		inbuf[0] = Esc;
X		inlen = read (0, inbuf+1, IOSIZE-1) + 1;
X		ESCseen = 0;
X	    } else {
X		inlen = read (0, inbuf, IOSIZE);
X	    }
X	    if (inlen > 0)
X		inlen = ProcessInput (inbuf, inlen);
X	    if (inlen > 0)
X		continue;
X	}
X	if (GotSignal && !status) {
X	    SigHandler ();
X	    continue;
X	}
X	if (w & 1 << curr->ptyfd && inlen > 0) {
X	    if ((len = write (curr->ptyfd, inbuf, inlen)) > 0) {
X		inlen -= len;
X		bcopy (inbuf+len, inbuf, inlen);
X	    }
X	}
X	if (GotSignal && !status) {
X	    SigHandler ();
X	    continue;
X	}
X	for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
X	    if (!(p = *pp))
X		continue;
X	    if (p->outlen) {
X		WriteString (p, p->outbuf, p->outlen);
X	    } else if (r & 1 << p->ptyfd) {
X		if ((len = read (p->ptyfd, buf, IOSIZE)) == -1) {
X		    if (errno == EWOULDBLOCK)
X			len = 0;
X		}
X		if (len > 0)
X		    WriteString (p, buf, len);
X	    }
X	    if (p->bell) {
X		p->bell = 0;
X		Msg (0, MakeBellMsg (pp-wtab));
X	    }
X	}
X	if (GotSignal && !status)
X	    SigHandler ();
X    }
X    /*NOTREACHED*/
X}
X
Xstatic SigHandler () {
X    while (GotSignal) {
X	GotSignal = 0;
X	DoWait ();
X    }
X}
X
Xstatic SigChld () {
X    GotSignal = 1;
X}
X
Xstatic DoWait () {
X    register pid;
X    register struct win **pp;
X    union wait wstat;
X
X    while ((pid = wait3 (&wstat, WNOHANG|WUNTRACED, NULL)) > 0) {
X	for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
X	    if (*pp && pid == (*pp)->wpid) {
X		if (WIFSTOPPED (wstat)) {
X		    kill((*pp)->wpid, SIGCONT);
X		} else {
X		    if (*pp == curr)
X			curr = 0;
X		    if (*pp == other)
X			other = 0;
X		    FreeWindow (*pp);
X		    *pp = 0;
X		}
X	    }
X	}
X    }
X    CheckWindows ();
X}
X
Xstatic CheckWindows () {
X    register struct win **pp;
X
X    /* If the current window disappeared and the "other" window is still
X     * there, switch to the "other" window, else switch to the window
X     * with the lowest index.
X     * If there current window is still there, but the "other" window
X     * vanished, "SetCurrWindow" is called in order to assign a new value
X     * to "other".
X     * If no window is alive at all, exit.
X     */
X    if (!curr && other) {
X	SwitchWindow (OtherNum);
X	return;
X    }
X    if (curr && !other) {
X	SetCurrWindow (CurrNum);
X	return;
X    }
X    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
X	if (*pp) {
X	    if (!curr)
X		SwitchWindow (pp-wtab);
X	    return;
X	}
X    }
X    Finit ();
X}
X
Xstatic Finit () {
X    register struct win *p, **pp;
X
X    for (pp = wtab; pp < wtab+MAXWIN; ++pp) {
X	if (p = *pp)
X	    FreeWindow (p);
X    }
X    SetTTY (0, &OldMode);
X    FinitTerm ();
X    printf ("[screen is terminating]\n");
X    exit (0);
X}
X
Xstatic InitKeytab () {
X    register i;
X
X    ktab['h'].type = ktab[Ctrl('h')].type = KEY_HARDCOPY;
X    ktab['z'].type = ktab[Ctrl('z')].type = KEY_SUSPEND;
X    ktab['c'].type = ktab[Ctrl('c')].type = KEY_SHELL;
X    ktab[' '].type = ktab[Ctrl(' ')].type = 
X    ktab['n'].type = ktab[Ctrl('n')].type = KEY_NEXT;
X    ktab['-'].type = ktab['p'].type = ktab[Ctrl('p')].type = KEY_PREV;
X    ktab['k'].type = ktab[Ctrl('k')].type = KEY_KILL;
X    ktab['l'].type = ktab[Ctrl('l')].type = KEY_REDISPLAY;
X    ktab['w'].type = ktab[Ctrl('w')].type = KEY_WINDOWS;
X    ktab['v'].type = ktab[Ctrl('v')].type = KEY_VERSION;
X    ktab['q'].type = ktab[Ctrl('q')].type = KEY_XON;
X    ktab['s'].type = ktab[Ctrl('s')].type = KEY_XOFF;
X    ktab['t'].type = ktab[Ctrl('t')].type = KEY_INFO;
X    ktab['.'].type = KEY_TERMCAP;
X    ktab[Ctrl('\\')].type = KEY_QUIT;
X    ktab[Esc].type = KEY_OTHER;
X    for (i = 0; i <= 9; i++)
X	ktab[i+'0'].type = KEY_0+i;
X}
X
Xstatic ProcessInput (buf, len) char *buf; {
X    register n, k;
X    register char *s, *p;
X    register struct win **pp;
X
X    for (s = p = buf; len > 0; len--, s++) {
X	if (*s == Esc) {
X	    if (len > 1) {
X		len--; s++;
X		k = ktab[*s].type;
X		if (*s == MetaEsc) {
X		    *p++ = Esc;
X		} else if (k >= KEY_0 && k <= KEY_9) {
X		    p = buf;
X		    SwitchWindow (k - KEY_0);
X		} else switch (ktab[*s].type) {
X		case KEY_TERMCAP:
X		    p = buf;
X		    WriteFile (0);
X		    break;
X		case KEY_HARDCOPY:
X		    p = buf;
X		    WriteFile (1);
X		    break;
X		case KEY_SUSPEND:
X		    p = buf;
X		    SetTTY (0, &OldMode);
X		    FinitTerm ();
X		    kill (getpid (), SIGTSTP);
X		    SetTTY (0, &NewMode);
X		    Activate (wtab[CurrNum]);
X		    break;
X		case KEY_SHELL:
X		    p = buf;
X		    if ((n = MakeWindow (ShellProg, ShellArgs,
X			    0, 0, (char *)0)) != -1)
X			SwitchWindow (n);
X		    break;
X		case KEY_NEXT:
X		    p = buf;
X		    if (MoreWindows ())
X			SwitchWindow (NextWindow ());
X		    break;
X		case KEY_PREV:
X		    p = buf;
X		    if (MoreWindows ())
X			SwitchWindow (PreviousWindow ());
X		    break;
X		case KEY_KILL:
X		    p = buf;
X		    FreeWindow (wtab[CurrNum]);
X		    if (other == curr)
X			other = 0;
X		    curr = wtab[CurrNum] = 0;
X		    CheckWindows ();
X		    break;
X		case KEY_QUIT:
X		    for (pp = wtab; pp < wtab+MAXWIN; ++pp)
X			if (*pp) FreeWindow (*pp);
X		    Finit ();
X		    /*NOTREACHED*/
X		case KEY_REDISPLAY:
X		    p = buf;
X		    Activate (wtab[CurrNum]);
X		    break;
X		case KEY_WINDOWS:
X		    p = buf;
X		    ShowWindows ();
X		    break;
X		case KEY_VERSION:
X		    p = buf;
X		    Msg (0, "%s  %s", ScreenVersion, AnsiVersion);
X		    break;
X		case KEY_INFO:
X		    p = buf;
X		    ShowInfo ();
X		    break;
X		case KEY_OTHER:
X		    p = buf;
X		    if (MoreWindows ())
X			SwitchWindow (OtherNum);
X		    break;
X		case KEY_XON:
X		    *p++ = Ctrl('q');
X		    break;
X		case KEY_XOFF:
X		    *p++ = Ctrl('s');
X		    break;
X		case KEY_CREATE:
X		    p = buf;
X		    if ((n = MakeWindow (ktab[*s].args[0], ktab[*s].args,
X			    0, 0, (char *)0)) != -1)
X			SwitchWindow (n);
X		    break;
X		}
X	    } else ESCseen = 1;
X	} else *p++ = *s;
X    }
X    return p - buf;
X}
X
Xstatic SwitchWindow (n) {
X    if (!wtab[n])
X	return;
X    SetCurrWindow (n);
X    Activate (wtab[n]);
X}
X
Xstatic SetCurrWindow (n) {
X    /*
X     * If we come from another window, this window becomes the
X     * "other" window:
X     */
X    if (curr) {
X	curr->active = 0;
X	other = curr;
X	OtherNum = CurrNum;
X    }
X    CurrNum = n;
X    curr = wtab[n];
X    curr->active = 1;
X    /*
X     * If the "other" window is currently undefined (at program start
X     * or because it has died), or if the "other" window is equal to the
X     * one just selected, we try to find a new one:
X     */
X    if (other == 0 || other == curr) {
X	OtherNum = NextWindow ();
X	other = wtab[OtherNum];
X    }
X}
X
Xstatic NextWindow () {
X    register struct win **pp;
X
X    for (pp = wtab+CurrNum+1; pp != wtab+CurrNum; ++pp) {
X	if (pp == wtab+MAXWIN)
X	    pp = wtab;
X	if (*pp)
X	    break;
X    }
X    return pp-wtab;
X}
X
Xstatic PreviousWindow () {
X    register struct win **pp;
X
X    for (pp = wtab+CurrNum-1; pp != wtab+CurrNum; --pp) {
X	if (pp < wtab)
X	    pp = wtab+MAXWIN-1;
X	if (*pp)
X	    break;
X    }
X    return pp-wtab;
X}
X
Xstatic MoreWindows () {
X    register struct win **pp;
X    register n;
X
X    for (n = 0, pp = wtab; pp < wtab+MAXWIN; ++pp)
X	if (*pp) ++n;
X    if (n <= 1)
X	Msg (0, "No other window.");
X    return n > 1;
X}
X
Xstatic FreeWindow (wp) struct win *wp; {
X    register i;
X
X    RemoveUtmp (wp->slot);
X    (void) chmod (wp->tty, 0666);
X    (void) chown (wp->tty, 0, 0);
X    close (wp->ptyfd);
X    for (i = 0; i < rows; ++i) {
X	free (wp->image[i]);
X	free (wp->attr[i]);
X    }
X    free (wp->image);
X    free (wp->attr);
X    free (wp);
X}
X
Xstatic MakeWindow (prog, args, aflag, StartAt, dir)
X	char *prog, **args, *dir; {
X    register struct win **pp, *p;
X    register char **cp;
X    register n, f;
X    int tf;
X    int mypid;
X    char ebuf[10];
X
X    pp = wtab+StartAt;
X    do {
X	if (*pp == 0)
X	    break;
X	if (++pp == wtab+MAXWIN)
X	    pp = wtab;
X    } while (pp != wtab+StartAt);
X    if (*pp) {
X	Msg (0, "No more windows.");
X	return -1;
X    }
X    n = pp - wtab;
X    if ((f = OpenPTY ()) == -1) {
X	Msg (0, "No more PTYs.");
X	return -1;
X    }
X    fcntl (f, F_SETFL, FNDELAY);
X    if ((p = *pp = (struct win *)malloc (sizeof (struct win))) == 0) {
Xnomem:
X	Msg (0, "Out of memory.");
X	return -1;
X    }
X    if ((p->image = (char **)malloc (rows * sizeof (char *))) == 0)
X	goto nomem;
X    for (cp = p->image; cp < p->image+rows; ++cp) {
X	if ((*cp = malloc (cols)) == 0)
X	    goto nomem;
X	bclear (*cp, cols);
X    }
X    if ((p->attr = (char **)malloc (rows * sizeof (char *))) == 0)
X	goto nomem;
X    for (cp = p->attr; cp < p->attr+rows; ++cp) {
X	if ((*cp = malloc (cols)) == 0)
X	    goto nomem;
X	bzero (*cp, cols);
X    }
X    if ((p->tabs = malloc (cols+1)) == 0)  /* +1 because 0 <= x <= cols */
X	goto nomem;
X    ResetScreen (p);
X    p->aflag = aflag;
X    p->active = 0;
X    p->bell = 0;
X    p->outlen = 0;
X    p->ptyfd = f;
X    strncpy (p->cmd, Filename (args[0]), MAXSTR-1);
X    p->cmd[MAXSTR-1] = '\0';
X    strncpy (p->tty, TtyName, MAXSTR-1);
X    (void) chown (TtyName, getuid (), getgid ());
X    (void) chmod (TtyName, 0622);
X    p->slot = SetUtmp (TtyName);
X    switch (p->wpid = fork ()) {
X    case -1:
X	Msg (errno, "Cannot fork");
X	free (p);
X	return -1;
X    case 0:
X	signal (SIGHUP, SIG_DFL);
X	signal (SIGINT, SIG_DFL);
X	signal (SIGQUIT, SIG_DFL);
X	signal (SIGTERM, SIG_DFL);
X	setuid (getuid ());
X	setgid (getgid ());
X	if (dir && chdir (dir) == -1) {
X	    SendErrorMsg ("Cannot chdir to %s: %s", dir, sys_errlist[errno]);
X	    exit (1);
X	}
X	mypid = getpid ();
X	if ((f = open ("/dev/tty", O_RDWR)) != -1) {
X	    ioctl (f, TIOCNOTTY, (char *)0);
X	    close (f);
X	}
X	if ((tf = open (TtyName, O_RDWR)) == -1) {
X	    SendErrorMsg ("Cannot open %s: %s", TtyName, sys_errlist[errno]);
X	    exit (1);
X	}
X	dup2 (tf, 0);
X	dup2 (tf, 1);
X	dup2 (tf, 2);
X	for (f = getdtablesize () - 1; f > 2; f--)
X	    close (f);
X	ioctl (0, TIOCSPGRP, &mypid);
X	setpgrp (0, mypid);
X	SetTTY (0, &OldMode);
X	NewEnv[2] = MakeTermcap (aflag);
X	sprintf (ebuf, "WINDOW=%d", n);
X	NewEnv[3] = ebuf;
X	execvpe (prog, args, NewEnv);
X	SendErrorMsg ("Cannot exec %s: %s", prog, sys_errlist[errno]);
X	exit (1);
X    }
X    return n;
X}
X
Xexecvpe (prog, args, env) char *prog, **args, **env; {
X    register char *path, *p;
X    char buf[1024];
X    char *shargs[MAXARGS+1];
X    register i, eaccess = 0;
X
X    if (prog[0] == '/')
X	path = "";
X    else if ((path = getenv ("PATH")) == 0)
X	path = DefaultPath;
X    do {
X	p = buf;
X	while (*path && *path != ':')
X	    *p++ = *path++;
X	if (p > buf)
X	    *p++ = '/';
X	strcpy (p, prog);
X	if (*path)
X	    ++path;
X	execve (buf, args, env);
X	switch (errno) {
X	case ENOEXEC:
X	    shargs[0] = DefaultShell;
X	    shargs[1] = buf;
X	    for (i = 1; shargs[i+1] = args[i]; ++i)
X		;
X	    execve (DefaultShell, shargs, env);
X	    return;
X	case EACCES:
X	    eaccess = 1;
X	    break;
X	case ENOMEM: case E2BIG: case ETXTBSY:
X	    return;
X	}
X    } while (*path);
X    if (eaccess)
X	errno = EACCES;
X}
X
Xstatic WriteFile (dump) {   /* dump==0: create .termcap, dump==1: hardcopy */
X    register i, j, k;
X    register char *p;
X    register FILE *f;
X    char fn[1024];
X    int pid, s;
X
X    if (dump)
X	sprintf (fn, "hardcopy.%d", CurrNum);
X    else
X	sprintf (fn, "%s/%s/.termcap", home, SockDir);
X    switch (pid = fork ()) {
X    case -1:
X	Msg (errno, "fork");
X	return;
X    case 0:
X	setuid (getuid ());
X	setgid (getgid ());
X	if ((f = fopen (fn, "w")) == NULL)
X	    exit (1);
X	if (dump) {
X	    for (i = 0; i < rows; ++i) {
X		p = curr->image[i];
X		for (k = cols-1; k >= 0 && p[k] == ' '; --k) ;
X		for (j = 0; j <= k; ++j)
X		    putc (p[j], f);
X		putc ('\n', f);
X	    }
X	} else {
X	    if (p = index (MakeTermcap (curr->aflag), '=')) {
X		fputs (++p, f);
X		putc ('\n', f);
X	    }
X	}
X	fclose (f);
X	exit (0);
X    default:
X	while ((i = wait (&s)) != pid)
X	    if (i == -1) return;
X	if ((s >> 8) & 0377)
X	    Msg (0, "Cannot open \"%s\".", fn);
X	else
X	    Msg (0, "%s written to \"%s\".", dump ? "Screen image" :
X		"Termcap entry", fn);
X    }
X}
X
Xstatic ShowWindows () {
X    char buf[1024];
X    register char *s;
X    register struct win **pp, *p;
X
X    for (s = buf, pp = wtab; pp < wtab+MAXWIN; ++pp) {
X	if ((p = *pp) == 0)
X	    continue;
X	if (s - buf + 5 + strlen (p->cmd) > cols-1)
X	    break;
X	if (s > buf) {
X	    *s++ = ' '; *s++ = ' ';
X	}
X	*s++ = pp - wtab + '0';
X	if (p == curr)
X	    *s++ = '*';
X	else if (p == other)
X	    *s++ = '-';
X	*s++ = ' ';
X	strcpy (s, p->cmd);
X	s += strlen (s);
X    }
X    Msg (0, buf);
X}
X
Xstatic ShowInfo () {
X    char buf[1024], *p;
X    register struct win *wp = curr;
X    struct tm *tp;
X    time_t now;
X#ifdef LOADAV
X    double av[3];
X#endif
X
X    time (&now);
X    tp = localtime (&now);
X    sprintf (buf, "%2d:%02.2d:%02.2d %s", tp->tm_hour, tp->tm_min, tp->tm_sec,
X	HostName);
X#ifdef LOADAV
X    if (avenrun && GetAvenrun (av)) {
X	p = buf + strlen (buf);
X	sprintf (p, " %2.2f %2.2f %2.2f", av[0], av[1], av[2]);
X    }
X#endif
X    p = buf + strlen (buf);
X    sprintf (p, " (%d,%d) %cflow %cins %corg %cwrap %cpad", wp->y, wp->x,
X	flowctl ? '+' : '-',
X	wp->insert ? '+' : '-', wp->origin ? '+' : '-',
X	wp->wrap ? '+' : '-', wp->keypad ? '+' : '-');
X    Msg (0, buf);
X}
X
Xstatic OpenPTY () {
X    register char *p, *l, *d;
X    register i, f, tf;
X
X    strcpy (PtyName, PtyProto);
X    strcpy (TtyName, TtyProto);
X    for (p = PtyName, i = 0; *p != 'X'; ++p, ++i) ;
X    for (l = "pqr"; *p = *l; ++l) {
X	for (d = "0123456789abcdef"; p[1] = *d; ++d) {
X	    if ((f = open (PtyName, O_RDWR)) != -1) {
X		TtyName[i] = p[0];
X		TtyName[i+1] = p[1];
X		if ((tf = open (TtyName, O_RDWR)) != -1) {
X		    close (tf);
X		    return f;
X		}
X		close (f);
X	    }
X	}
X    }
X    return -1;
X}
X
Xstatic SetTTY (fd, mp) struct mode *mp; {
X    ioctl (fd, TIOCSETP, &mp->m_ttyb);
X    ioctl (fd, TIOCSETC, &mp->m_tchars);
X    ioctl (fd, TIOCSLTC, &mp->m_ltchars);
X    ioctl (fd, TIOCLSET, &mp->m_lmode);
X    ioctl (fd, TIOCSETD, &mp->m_ldisc);
X}
X
Xstatic GetTTY (fd, mp) struct mode *mp; {
X    ioctl (fd, TIOCGETP, &mp->m_ttyb);
X    ioctl (fd, TIOCGETC, &mp->m_tchars);
X    ioctl (fd, TIOCGLTC, &mp->m_ltchars);
X    ioctl (fd, TIOCLGET, &mp->m_lmode);
X    ioctl (fd, TIOCGETD, &mp->m_ldisc);
X}
X
Xstatic SetMode (op, np) struct mode *op, *np; {
X    *np = *op;
X    np->m_ttyb.sg_flags &= ~(CRMOD|ECHO);
X    np->m_ttyb.sg_flags |= CBREAK;
X    np->m_tchars.t_intrc = -1;
X    np->m_tchars.t_quitc = -1;
X    if (!flowctl) {
X	np->m_tchars.t_startc = -1;
X	np->m_tchars.t_stopc = -1;
X    }
X    np->m_ltchars.t_suspc = -1;
X    np->m_ltchars.t_dsuspc = -1;
X    np->m_ltchars.t_flushc = -1;
X    np->m_ltchars.t_lnextc = -1;
X}
X
Xstatic GetSockName () {
X    struct stat s;
X    register client;
X    register char *p;
X
X    if (!mflag && (SockName = getenv ("STY")) != 0 && *SockName != '\0') {
X	client = 1;
X	setuid (getuid ());
X	setgid (getgid ());
X    } else {
X	if ((p = ttyname (0)) == 0 || (p = ttyname (1)) == 0 ||
X		(p = ttyname (2)) == 0 || *p == '\0')
X	    Msg (0, "screen must run on a tty.");
X	SockName = Filename (p);
X	client = 0;
X    }
X    if ((home = getenv ("HOME")) == 0)
X	Msg (0, "$HOME is undefined.");
X    sprintf (SockPath, "%s/%s", home, SockDir);
X    if (stat (SockPath, &s) == -1) {
X	if (errno == ENOENT) {
X	    if (mkdir (SockPath, 0700) == -1)
X		Msg (errno, "Cannot make directory %s", SockPath);
X	    (void) chown (SockPath, getuid (), getgid ());
X	} else Msg (errno, "Cannot get status of %s", SockPath);
X    } else {
X	if ((s.st_mode & S_IFMT) != S_IFDIR)
X	    Msg (0, "%s is not a directory.", SockPath);
X	if ((s.st_mode & 0777) != 0700)
X	    Msg (0, "Directory %s must have mode 700.", SockPath);
X	if (s.st_uid != getuid ())
X	    Msg (0, "You are not the owner of %s.", SockPath);
X    }
X    strcat (SockPath, "/");
X    strcat (SockPath, SockName);
X    return client;
X}
X
Xstatic MakeServerSocket () {
X    register s;
X    struct sockaddr_un a;
X
X    (void) unlink (SockPath);
X    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
X	Msg (errno, "socket");
X    a.sun_family = AF_UNIX;
X    strcpy (a.sun_path, SockPath);
X    if (bind (s, (struct sockaddr *)&a, strlen (SockPath)+2) == -1)
X	Msg (errno, "bind");
X    (void) chown (SockPath, getuid (), getgid ());
X    if (listen (s, 5) == -1)
X	Msg (errno, "listen");
X    return s;
X}
X
Xstatic MakeClientSocket () {
X    register s;
X    struct sockaddr_un a;
X
X    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
X	Msg (errno, "socket");
X    a.sun_family = AF_UNIX;
X    strcpy (a.sun_path, SockPath);
X    if (connect (s, (struct sockaddr *)&a, strlen (SockPath)+2) == -1)
X	Msg (errno, "connect: %s", SockPath);
X    return s;
X}
X
Xstatic SendCreateMsg (s, ac, av, aflag) char **av; {
X    struct msg m;
X    register char *p;
X    register len, n;
X
X    m.type = MSG_CREATE;
X    p = m.m.create.line;
X    for (n = 0; ac > 0 && n < MAXARGS-1; ++av, --ac, ++n) {
X	len = strlen (*av) + 1;
X	if (p + len >= m.m.create.line+MAXLINE)
X	    break;
X	strcpy (p, *av);
X	p += len;
X    }
X    m.m.create.nargs = n;
X    m.m.create.aflag = aflag;
X    if (getwd (m.m.create.dir) == 0)
X	Msg (0, "%s", m.m.create.dir);
X    if (write (s, &m, sizeof (m)) != sizeof (m))
X	Msg (errno, "write");
X}
X
X/*VARARGS1*/
Xstatic SendErrorMsg (fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
X    register s;
X    struct msg m;
X
X    s = MakeClientSocket ();
X    m.type = MSG_ERROR;
X    sprintf (m.m.message, fmt, p1, p2, p3, p4, p5, p6);
X    (void) write (s, &m, sizeof (m));
X    close (s);
X    sleep (2);
X}
X
Xstatic ReceiveMsg (s) {
X    register ns;
X    struct sockaddr_un a;
X    int len = sizeof (a);
X    struct msg m;
X
X    if ((ns = accept (s, (struct sockaddr *)&a, &len)) == -1) {
X	Msg (errno, "accept");
X	return;
X    }
X    if ((len = read (ns, &m, sizeof (m))) != sizeof (m)) {
X	if (len == -1)
X	    Msg (errno, "read");
X	else
X	    Msg (0, "Short message (%d bytes)", len);
X	close (ns);
X	return;
X    }
X    switch (m.type) {
X    case MSG_CREATE:
X	ExecCreate (&m);
X	break;
X    case MSG_ERROR:
X	Msg (0, "%s", m.m.message);
X	break;
X    default:
X	Msg (0, "Invalid message (type %d).", m.type);
X    }
X    close (ns);
X}
X
Xstatic ExecCreate (mp) struct msg *mp; {
X    char *args[MAXARGS];
X    register n;
X    register char **pp = args, *p = mp->m.create.line;
X
X    for (n = mp->m.create.nargs; n > 0; --n) {
X	*pp++ = p;
X	p += strlen (p) + 1;
X    }
X    *pp = 0;
X    if ((n = MakeWindow (mp->m.create.line, args, mp->m.create.aflag, 0,
X	    mp->m.create.dir)) != -1)
X	SwitchWindow (n);
X}
X
Xstatic ReadRc (fn) char *fn; {
X    FILE *f;
X    register char *p, **pp, **ap;
X    register argc, num, c;
X    char buf[256];
X    char *args[MAXARGS];
X    int key;
X
X    ap = args;
X    if (access (fn, R_OK) == -1)
X	return;
X    if ((f = fopen (fn, "r")) == NULL)
X	return;
X    while (fgets (buf, 256, f) != NULL) {
X	if (p = rindex (buf, '\n'))
X	    *p = '\0';
X	if ((argc = Parse (fn, buf, ap)) == 0)
X	    continue;
X	if (strcmp (ap[0], "escape") == 0) {
X	    p = ap[1];
X	    if (argc < 2 || strlen (p) != 2)
X		Msg (0, "%s: two characters required after escape.", fn);
X	    Esc = *p++;
X	    MetaEsc = *p;
X	} else if (strcmp (ap[0], "chdir") == 0) {
X	    p = argc < 2 ? home : ap[1];
X	    if (chdir (p) == -1)
X		Msg (errno, "%s", p);
X	} else if (strcmp (ap[0], "bell") == 0) {
X	    if (argc != 2) {
X		Msg (0, "%s: bell: one argument required.", fn);
X	    } else {
X		if ((BellString = malloc (strlen (ap[1]) + 1)) == 0)
X		    Msg (0, "Out of memory.");
X		strcpy (BellString, ap[1]);
X	    }
X	} else if (strcmp (ap[0], "screen") == 0) {
X	    num = 0;
X	    if (argc > 1 && IsNum (ap[1], 10)) {
X		num = atoi (ap[1]);
X		if (num < 0 || num > MAXWIN-1)
X		    Msg (0, "%s: illegal screen number %d.", fn, num);
X		--argc; ++ap;
X	    }
X	    if (argc < 2) {
X		ap[1] = ShellProg; argc = 2;
X	    }
X	    ap[argc] = 0;
X	    (void) MakeWindow (ap[1], ap+1, 0, num, (char *)0);
X	} else if (strcmp (ap[0], "bind") == 0) {
X	    p = ap[1];
X	    if (argc < 2 || *p == '\0')
X		Msg (0, "%s: key expected after bind.", fn);
X	    if (p[1] == '\0') {
X		key = *p;
X	    } else if (p[0] == '^' && p[1] != '\0' && p[2] == '\0') {
X		c = p[1];
X		if (isupper (c))
X		    p[1] = tolower (c);    
X		key = Ctrl(c);
X	    } else if (IsNum (p, 7)) {
X		(void) sscanf (p, "%o", &key);
X	    } else {
X		Msg (0,
X		    "%s: bind: character, ^x, or octal number expected.", fn);
X	    }
X	    if (argc < 3) {
X		ktab[key].type = 0;
X	    } else {
X		for (pp = KeyNames; *pp; ++pp)
X		    if (strcmp (ap[2], *pp) == 0) break;
X		if (*pp) {
X		    ktab[key].type = pp-KeyNames+1;
X		} else {
X		    ktab[key].type = KEY_CREATE;
X		    ktab[key].args = SaveArgs (argc-2, ap+2);
X		}
X	    }
X	} else Msg (0, "%s: unknown keyword \"%s\".", fn, ap[0]);
X    }
X    fclose (f);
X}
X
Xstatic Parse (fn, buf, args) char *fn, *buf, **args; {
X    register char *p = buf, **ap = args;
X    register delim, argc = 0;
X
X    argc = 0;
X    for (;;) {
X	while (*p && (*p == ' ' || *p == '\t')) ++p;
X	if (*p == '\0' || *p == '#')
X	    return argc;
X	if (argc > MAXARGS-1)
X	    Msg (0, "%s: too many tokens.", fn);
X	delim = 0;
X	if (*p == '"' || *p == '\'') {
X	    delim = *p; *p = '\0'; ++p;
X	}
X	++argc;
X	*ap = p; ++ap;
X	while (*p && !(delim ? *p == delim : (*p == ' ' || *p == '\t')))
X	    ++p;
X	if (*p == '\0') {
X	    if (delim)
X		Msg (0, "%s: Missing quote.", fn);
X	    else
X		return argc;
X	}
X	*p++ = '\0';
X    }
X}
X
Xstatic char **SaveArgs (argc, argv) register argc; register char **argv; {
X    register char **ap, **pp;
X
X    if ((pp = ap = (char **)malloc ((argc+1) * sizeof (char **))) == 0)
X	Msg (0, "Out of memory.");
X    while (argc--) {
X	if ((*pp = malloc (strlen (*argv)+1)) == 0)
X	    Msg (0, "Out of memory.");
X	strcpy (*pp, *argv);
X	++pp; ++argv;
X    }
X    *pp = 0;
X    return ap;
X}
X
Xstatic MakeNewEnv () {
X    register char **op, **np = NewEnv;
X    static char buf[MAXSTR];
X
X    if (strlen (SockName) > MAXSTR-5)
X	SockName = "?";
X    sprintf (buf, "STY=%s", SockName);
X    *np++ = buf;
X    *np++ = Term;
X    np += 2;
X    for (op = environ; *op; ++op) {
X	if (np == NewEnv + MAXARGS - 1)
X	    break;
X	if (!IsSymbol (*op, "TERM") && !IsSymbol (*op, "TERMCAP")
X		&& !IsSymbol (*op, "STY"))
X	    *np++ = *op;
X    }
X    *np = 0;
X}
X
Xstatic IsSymbol (e, s) register char *e, *s; {
X    register char *p;
X    register n;
X
X    for (p = e; *p && *p != '='; ++p) ;
X    if (*p) {
X	*p = '\0';
X	n = strcmp (e, s);
X	*p = '=';
X	return n == 0;
X    }
X    return 0;
X}
X
X/*VARARGS2*/
XMsg (err, fmt, p1, p2, p3, p4, p5, p6) char *fmt; {
X    char buf[1024];
X    register char *p = buf;
X
X    sprintf (p, fmt, p1, p2, p3, p4, p5, p6);
X    if (err) {
X	p += strlen (p);
X	if (err > 0 && err < sys_nerr)
X	    sprintf (p, ": %s", sys_errlist[err]);
X	else
X	    sprintf (p, ": Error %d", err);
X    }
X    if (HasWindow) {
X	MakeStatus (buf, curr);
X    } else {
X	printf ("%s\r\n", buf);
X	exit (1);
X    }
X}
X
Xbclear (p, n) char *p; {
X    bcopy (blank, p, n);
X}
X
Xstatic char *Filename (s) char *s; {
X    register char *p;
X
X    p = s + strlen (s) - 1;
X    while (p >= s && *p != '/') --p;
X    return ++p;
X}
X
Xstatic IsNum (s, base) register char *s; register base; {
X    for (base += '0'; *s; ++s)
X	if (*s < '0' || *s > base)
X	    return 0;
X    return 1;
X}
X
Xstatic char *MakeBellMsg (n) {
X    static char buf[MAXSTR];
X    register char *p = buf, *s = BellString;
X
X    for (s = BellString; *s && p < buf+MAXSTR-1; s++)
X	*p++ = (*s == '%') ? n + '0' : *s;
X    *p = '\0';
X    return buf;
X}
X
Xstatic InitUtmp () {
X    struct passwd *p;
X
X    if ((utmpf = open (UtmpName, O_WRONLY)) == -1) {
X	if (errno != EACCES)
X	    Msg (errno, UtmpName);
X	return;
X    }
X    if ((LoginName = getlogin ()) == 0 || LoginName[0] == '\0') {
X	if ((p = getpwuid (getuid ())) == 0)
X	    return;
X	LoginName = p->pw_name;
X    }
X    utmp = 1;
X}
X
Xstatic SetUtmp (name) char *name; {
X    register char *p;
X    register struct ttyent *tp;
X    register slot = 1;
X    struct utmp u;
X
X    if (!utmp)
X	return 0;
X    if (p = rindex (name, '/'))
X	++p;
X    else p = name;
X    setttyent ();
X    while ((tp = getttyent ()) != NULL && strcmp (p, tp->ty_name) != 0)
X	++slot;
X    if (tp == NULL)
X	return 0;
X    strncpy (u.ut_line, p, 8);
X    strncpy (u.ut_name, LoginName, 8);
X    u.ut_host[0] = '\0';
X    time (&u.ut_time);
X    (void) lseek (utmpf, (long)(slot * sizeof (u)), 0);
X    (void) write (utmpf, &u, sizeof (u));
X    return slot;
X}
X
Xstatic RemoveUtmp (slot) {
X    struct utmp u;
X
X    if (slot) {
X	bzero (&u, sizeof (u));
X	(void) lseek (utmpf, (long)(slot * sizeof (u)), 0);
X	(void) write (utmpf, &u, sizeof (u));
X    }
X}
X
X#ifndef GETTTYENT
X
Xstatic setttyent () {
X    struct stat s;
X    register f;
X    register char *p, *ep;
X
X    if (ttnext) {
X	ttnext = tt;
X	return;
X    }
X    if ((f = open (ttys, O_RDONLY)) == -1 || fstat (f, &s) == -1)
X	Msg (errno, ttys);
X    if ((tt = malloc (s.st_size + 1)) == 0)
X	Msg (0, "Out of memory.");
X    if (read (f, tt, s.st_size) != s.st_size)
X	Msg (errno, ttys);
X    close (f);
X    for (p = tt, ep = p + s.st_size; p < ep; ++p)
X	if (*p == '\n') *p = '\0';
X    *p = '\0';
X    ttnext = tt;
X}
X
Xstatic struct ttyent *getttyent () {
X    static struct ttyent t;
X
X    if (*ttnext == '\0')
X	return NULL;
X    t.ty_name = ttnext + 2;
X    ttnext += strlen (ttnext) + 1;
X    return &t;
X}
X
X#endif
X
X#ifdef LOADAV
X
Xstatic InitKmem () {
X    if ((kmemf = open (KmemName, O_RDONLY)) == -1)
X	return;
X    nl[0].n_name = AvenrunSym;
X    nlist (UnixName, nl);
X    if (nl[0].n_type == 0 || nl[0].n_value == 0)
X	return;
X    avenrun = 1;
X}
X
Xstatic GetAvenrun (av) double av[]; {
X    if (lseek (kmemf, nl[0].n_value, 0) == -1)
X	return 0;
X    if (read (kmemf, av, 3*sizeof (double)) != 3*sizeof (double))
X	return 0;
X    return 1;
X}
X
X#endif
*-*-END-of-screen.c-*-*
echo x - screen.h
sed 's/^X//' >screen.h <<'*-*-END-of-screen.h-*-*'
X/* Copyright (c) 1987, Oliver Laumann, Technical University of Berlin.
X * Not derived from licensed software.
X *
X * Permission is granted to freely use, copy, modify, and redistribute
X * this software, provided that no attempt is made to gain profit from it,
X * the author is not construed to be liable for any results of using the
X * software, alterations are clearly marked as such, and this notice is
X * not modified.
X */
X
Xenum state_t {
X    LIT,         /* Literal input */
X    ESC,         /* Start of escape sequence */
X    STR,         /* Start of control string */
X    TERM,        /* ESC seen in control string */
X    CSI,         /* Reading arguments in "CSI Pn ; Pn ; ... ; XXX" */
X};
X
Xenum string_t {
X    NONE,
X    DCS,         /* Device control string */
X    OSC,         /* Operating system command */
X    APC,         /* Application program command */
X    PM,          /* Privacy message */
X};
X
X#define MAXSTR       128
X
X#define IOSIZE       80
X
Xstruct win {
X    int wpid;
X    int ptyfd;
X    int aflag;
X    char outbuf[IOSIZE];
X    int outlen;
X    char cmd[MAXSTR];
X    char tty[MAXSTR];
X    int slot;
X    char **image;
X    char **attr;
X    int active;
X    int x, y;
X    char LocalAttr;
X    int saved;
X    int Saved_x, Saved_y;
X    char SavedLocalAttr;
X    int top, bot;
X    int wrap;
X    int origin;
X    int insert;
X    int keypad;
X    enum state_t state;
X    enum string_t StringType;
X    char string[MAXSTR];
X    char *stringp;
X    char *tabs;
X    int vbwait;
X    int bell;
X};
*-*-END-of-screen.h-*-*
exit
