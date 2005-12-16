/* Copyright (c) 1987, Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 */

#ifndef NO_SCCS_ID
static char sccs_id[]="@(#)ansi.c	1.3 88/07/20 01:08:40";
#endif /* NO_SCCS_ID */

char AnsiVersion[] = "ansi 1.0g 1.3 88/07/20";

#include <stdio.h>
#include <sys/types.h>
#include "screen.h"

#define A_SO     0x1    /* Standout mode */
#define A_US     0x2    /* Underscore mode */
#define A_BL     0x4    /* Blinking */
#define A_BD     0x8    /* Bold mode */
#define A_DI    0x10    /* Dim mode */
#define A_RV    0x20    /* Reverse mode */
#define A_MAX   0x20

/* Types of movement used by Goto() */
enum move_t {
    M_NONE,
    M_UP,
    M_DO,
    M_LE,
    M_RI,
    M_RW,
    M_CR,
};

#define MAXARGS      64
#define EXPENSIVE    1000

extern char *getenv(), *tgetstr(), *tgoto(), *malloc();

int rows, cols;
int status;
int flowctl;
char Term[] = "TERM=screen";
char Termcap[1024];
char *blank;
char PC;
time_t TimeDisplayed;

static char tbuf[1024], tentry[1024];
static char *tp = tentry;
static char *TI, *TE, *BL, *VB, *BC, *CR, *NL, *CL, *IS, *CM;
static char *US, *UE, *SO, *SE, *CE, *CD, *DO, *SR, *SF, *AL;
static char *CS, *DL, *DC, *IC, *IM, *EI, *UP, *ND, *KS, *KE;
static char *MB, *MD, *MH, *MR, *ME;
static AM;
static args[MAXARGS];
static char GotArg[MAXARGS];
static NumArgs;
static char GlobalAttr, TmpAttr;
static char *OldImage, *OldAttr;
static last_x, last_y;
static struct win *curr;
static display = 1;
static StrCost;
static UPcost, DOcost, LEcost, NDcost, CRcost, IMcost, EIcost;
static tcLineLen = 100;
static char *null;
static StatLen;
static insert;
static keypad;

static char *KeyCaps[] = {
    "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8", "k9",
    "kb", "kd", "kh", "kl", "ko", "kr", "ku",
    0
};

static char TermcapConst[] = "TERMCAP=\
SC|screen|VT 100/ANSI X3.64 virtual terminal|\\\n\
\t:DO=\\E[%dB:LE=\\E[%dD:RI=\\E[%dC:UP=\\E[%dA:bs:bt=\\E[Z:\\\n\
\t:cd=\\E[J:ce=\\E[K:cl=\\E[2J\\E[H:cm=\\E[%i%d;%dH:ct=\\E[3g:\\\n\
\t:do=\\E[B:nd=\\E[C:pt:rc=\\E8:rs=\\Ec:sc=\\E7:st=\\EH:up=\\E[A:";

InitTerm () {
    register char *s;

    if ((s = getenv ("TERM")) == 0)
	Msg (0, "No TERM in environment.");
    if (tgetent (tbuf, s) != 1)
	Msg (0, "Cannot find termcap entry for %s.", s);
    cols = tgetnum ("co");
    rows = tgetnum ("li");
    if (cols <= 0)
	cols = 80;
    if (rows <= 0)
	rows = 24;
    if (tgetflag ("hc"))
	Msg (0, "You can't run screen on a hardcopy terminal.");
    if (tgetflag ("os"))
	Msg (0, "You can't run screen on a terminal that overstrikes.");
    if (tgetflag ("ns"))
	Msg (0, "Terminal must support scrolling.");
    if (!(CL = tgetstr ("cl", &tp)))
	Msg (0, "Clear screen capability required.");
    if (!(CM = tgetstr ("cm", &tp)))
	Msg (0, "Addressable cursor capability required.");
    if (s = tgetstr ("ps", &tp))
	PC = s[0];
    flowctl = !tgetflag ("NF");
    AM = tgetflag ("am");
    if (tgetflag ("LP"))
	AM = 0;
    TI = tgetstr ("ti", &tp);
    TE = tgetstr ("te", &tp);
    if (!(BL = tgetstr ("bl", &tp)))
	BL = "\007";
    VB = tgetstr ("vb", &tp);
    if (!(BC = tgetstr ("bc", &tp))) {
	if (tgetflag ("bs"))
	    BC = "\b";
	else
	    BC = tgetstr ("le", &tp);
    }
    if (!(CR = tgetstr ("cr", &tp)))
	CR = "\r";
    if (!(NL = tgetstr ("nl", &tp)))
	NL = "\n";
    IS = tgetstr ("is", &tp);
    if (tgetnum ("sg") <= 0) {
	US = tgetstr ("us", &tp);
	UE = tgetstr ("ue", &tp);
	SO = tgetstr ("so", &tp);
	SE = tgetstr ("se", &tp);
	MB = tgetstr ("mb", &tp);
	MD = tgetstr ("md", &tp);
	MH = tgetstr ("mh", &tp);
	MR = tgetstr ("mr", &tp);
	ME = tgetstr ("me", &tp);
	/*
	 * Does ME also reverse the effect of SO and/or US?  This is not
	 * clearly specified by the termcap manual.
	 * Anyway, we should at least look whether ME and SE/UE are equal:
	 */
	if (SE && UE && ME && (strcmp (SE, UE) == 0 || strcmp (ME, UE) == 0))
	    UE = 0;
	if (SE && ME && strcmp (SE, ME) == 0)
	    SE = 0;
    }
    CE = tgetstr ("ce", &tp);
    CD = tgetstr ("cd", &tp);
    if (!(DO = tgetstr ("do", &tp)))
	DO = NL;
    UP = tgetstr ("up", &tp);
    ND = tgetstr ("nd", &tp);
    SR = tgetstr ("sr", &tp);
    if (!(SF = tgetstr ("sf", &tp)))
	SF = NL;
    AL = tgetstr ("al", &tp);
    DL = tgetstr ("dl", &tp);
    CS = tgetstr ("cs", &tp);
    DC = tgetstr ("dc", &tp);
    IC = tgetstr ("ic", &tp);
    IM = tgetstr ("im", &tp);
    EI = tgetstr ("ei", &tp);
    if (tgetflag ("in"))
	IC = IM = 0;
    if (IC && IC[0] == '\0')
	IC = 0;
    if (IM && IM[0] == '\0')
	IM = 0;
    if (EI && EI[0] == '\0')
	EI = 0;
    KS = tgetstr ("ks", &tp);
    KE = tgetstr ("ke", &tp);
    blank = malloc (cols);
    null = malloc (cols);
    OldImage = malloc (cols);
    OldAttr = malloc (cols);
    if (!(blank && null && OldImage && OldAttr))
	Msg (0, "Out of memory.");
    MakeBlankLine (blank, cols);
    bzero (null, cols);
    UPcost = CalcCost (UP);
    DOcost = CalcCost (DO);
    LEcost = CalcCost (BC);
    NDcost = CalcCost (ND);
    CRcost = CalcCost (CR);
    IMcost = CalcCost (IM);
    EIcost = CalcCost (EI);
    PutStr (IS);
    PutStr (TI);
    PutStr (CL);
}

FinitTerm () {
    PutStr (TE);
    PutStr (IS);
}

static AddCap (s) char *s; {
    register n;

    if (tcLineLen + (n = strlen (s)) > 55) {
	strcat (Termcap, "\\\n\t:");
	tcLineLen = 0;
    }
    strcat (Termcap, s);
    tcLineLen += n;
}

char *MakeTermcap (aflag) {
    char buf[1024];
    register char **pp, *p;

    strcpy (Termcap, TermcapConst);
    sprintf (buf, "li#%d:co#%d:", rows, cols);
    AddCap (buf);
    if (VB)
	AddCap ("vb=\\E[?5h\\E[?5l:");
    if (US) {
	AddCap ("us=\\E[4m:");
	AddCap ("ue=\\E[24m:");
    }
    if (SO) {
	AddCap ("so=\\E[3m:");
	AddCap ("se=\\E[23m:");
    }
    if (MB)
	AddCap ("mb=\\E[5m:");
    if (MD)
	AddCap ("md=\\E[1m:");
    if (MH)
	AddCap ("mh=\\E[2m:");
    if (MR)
	AddCap ("mr=\\E[7m:");
    if (MB || MD || MH || MR)
	AddCap ("me=\\E[0m:");
    if ((CS && SR) || AL || aflag) {
	AddCap ("sr=\\EM:");
	AddCap ("al=\\E[L:");
	AddCap ("AL=\\E[%dL:");
    }
    if (CS || DL || aflag) {
	AddCap ("dl=\\E[M:");
	AddCap ("DL=\\E[%dM:");
    }
    if (CS)
	AddCap ("cs=\\E[%i%d;%dr:");
    if (DC || aflag) {
	AddCap ("dc=\\E[P:");
	AddCap ("DC=\\E[%dP:");
    }
    if (IC || IM || aflag) {
	AddCap ("im=\\E[4h:");
	AddCap ("ei=\\E[4l:");
	AddCap ("ic=:");
	AddCap ("IC=\\E[%d@:");
    }
    if (KS)
	AddCap ("ks=\\E=:");
    if (KE)
	AddCap ("ke=\\E>:");
    for (pp = KeyCaps; *pp; ++pp)
	if (p = tgetstr (*pp, &tp)) {
	    MakeString (*pp, buf, p);
	    AddCap (buf);
	}
    return Termcap;
}

MakeString (cap, buf, s) char *cap, *buf; register char *s; {
    register char *p = buf;
    register unsigned c;

    *p++ = *cap++; *p++ = *cap; *p++ = '=';
    while (c = *s++) {
	switch (c) {
	case '\033':
	    *p++ = '\\'; *p++ = 'E'; break;
	case ':':
	    sprintf (p, "\\072"); p += 4; break;
	case '^': case '\\':
	    *p++ = '\\'; *p++ = c; break;
	default:
	    if (c >= 200) {
		sprintf (p, "\\%03o", c & 0377); p += 4;
	    } else if (c < ' ') {
		*p++ = '^'; *p++ = c + '@';
	    } else *p++ = c;
	}
    }
    *p++ = ':'; *p = '\0';
}

Activate (wp) struct win *wp; {
    RemoveStatus (wp);
    curr = wp;
    display = 1;
    NewRendition (GlobalAttr, curr->LocalAttr);
    GlobalAttr = curr->LocalAttr;
    InsertMode (curr->insert);
    KeypadMode (curr->keypad);
    if (CS)
	PutStr (tgoto (CS, curr->bot, curr->top));
    Redisplay ();
}

ResetScreen (p) register struct win *p; {
    register i;

    bzero (p->tabs, cols);
    for (i = 8; i < cols; i += 8)
	p->tabs[i] = 1;
    p->wrap = 1;
    p->origin = 0;
    p->insert = 0;
    p->vbwait = 0;
    p->keypad = 0;
    p->top = 0;
    p->bot = rows - 1;
    p->saved = 0;
    p->LocalAttr = 0;
    p->x = p->y = 0;
    p->state = LIT;
    p->StringType = NONE;
}

WriteString (wp, buf, len) struct win *wp; register char *buf; {
    register c, intermediate = 0;

    if (!len)
	return;
    curr = wp;
    display = curr->active;
    if (display)
	RemoveStatus (wp);
    do {
	c = *buf++;
	if (c == '\0' || c == '\177')
	    continue;
NextChar:
	switch (curr->state) {
	case TERM:
	    switch (c) {
		case '\\':
		    curr->state = LIT;
		    *(curr->stringp) = '\0';
		    if (curr->StringType == PM && display) {
			MakeStatus (curr->string, curr);
			if (status && len > 1) {
			    curr->outlen = len-1;
			    bcopy (buf, curr->outbuf, curr->outlen);
			    return;
			}
		    }
		    break;
		default:
		    curr->state = STR;
		    AddChar ('\033');
		    AddChar (c);
	    }
	    break;
	case STR:
	    switch (c) {
		case '\0':
		    break;
		case '\033':
		    curr->state = TERM; break;
		default:
		    AddChar (c);
	    }
	    break;
	case ESC:
	    switch (c) {
	    case '[':
		NumArgs = 0;
		intermediate = 0;
		bzero ((char *)args, MAXARGS * sizeof (int));
		bzero (GotArg, MAXARGS);
		curr->state = CSI;
		break;
	    case ']':
		StartString (OSC); break;
	    case '_':
		StartString (APC); break;
	    case 'P':
		StartString (DCS); break;
	    case '^':
		StartString (PM); break;
	    default:
		if (Special (c))
		    break;
		if (c >= ' ' && c <= '/') {
		    intermediate = intermediate ? -1 : c;
		} else if (c >= '0' && c <= '~') {
		    DoESC (c, intermediate);
		    curr->state = LIT;
		} else {
		    curr->state = LIT;
		    goto NextChar;
		}
	    }
	    break;
	case CSI:
	    switch (c) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
		if (NumArgs < MAXARGS) {
		    args[NumArgs] = 10 * args[NumArgs] + c - '0';
		    GotArg[NumArgs] = 1;
		}
		break;
	    case ';': case ':':
		NumArgs++; break;
	    default:
		if (Special (c))
		    break;
		if (c >= '@' && c <= '~') {
		    NumArgs++;
		    DoCSI (c, intermediate);
		    curr->state = LIT;
		} else if ((c >= ' ' && c <= '/') || (c >= '<' && c <= '?')) {
		    intermediate = intermediate ? -1 : c;
		} else {
		    curr->state = LIT;
		    goto NextChar;
		}
	    }
	    break;
	default:
	    if (!Special (c)) {
		if (c == '\033') {
		    intermediate = 0;
		    curr->state = ESC;
		} else if (c < ' ') {
		    break;
		} else if (curr->x < cols-1) {
		    if (curr->insert) {
			InsertAChar (c);
		    } else {
			if (display)
			    putchar (c);
			SetChar (c);
		    }
		    curr->x++;
		} else if (curr->x == cols-1) {
		    SetChar (c);
		    if (!(AM && curr->y == rows-1)) {
			if (display)
			    putchar (c);
			Goto (-1, -1, curr->y, curr->x);
		    }
		    curr->x++;
		} else {
		    if (curr->wrap) {
			Return ();
			LineFeed ();
			if (curr->insert) {
			    InsertAChar (c);
			} else {
			    if (display)
				putchar (c);
			    SetChar (c);
			}
			curr->x = 1;
		    } else curr->x = cols;
		}
	    }
	}
    } while (--len);
    curr->outlen = 0;
}

static Special (c) register c; {
    switch (c) {
    case '\b':
	BackSpace (); return 1;
    case '\r':
	Return (); return 1;
    case '\n':
	LineFeed (); return 1;
    case '\007':
	PutStr (BL);
	if (!display)
	    curr->bell = 1;
	return 1;
    case '\t':
	ForwardTab (); return 1;
    }
    return 0;
}

static DoESC (c, intermediate) {
    switch (intermediate) {
    case 0:
	switch (c) {
	case 'E':
	    Return ();
	    LineFeed ();
	    break;
	case 'D':
	    LineFeed (); 
	    break;
	case 'M':
	    ReverseLineFeed ();
	    break;
	case 'H':
	    curr->tabs[curr->x] = 1;
	    break;
	case '7':
	    SaveCursor ();
	    break;
	case '8':
	    RestoreCursor ();
	    break;
	case 'c':
	    ClearScreen ();
	    Goto (curr->y, curr->x, 0, 0);
	    NewRendition (GlobalAttr, 0);
	    SetRendition (0);
	    if (curr->insert)
		InsertMode (0);
	    if (curr->keypad)
		KeypadMode (0);
	    if (CS)
		PutStr (tgoto (CS, rows-1, 0));
	    ResetScreen (curr);
	    break;
	case '=':
	    KeypadMode (1);
	    curr->keypad = 1;
	    break;
	case '>':
	    KeypadMode (0);
	    curr->keypad = 0;
	    break;
	}
	break;
    case '#':
	switch (c) {
	case '8':
	    FillWithEs ();
	    break;
	}
	break;
    }
}

static DoCSI (c, intermediate) {
    register i, a1 = args[0], a2 = args[1];

    if (NumArgs >= MAXARGS)
	NumArgs = MAXARGS;
    for (i = 0; i < NumArgs; ++i)
	if (args[i] == 0)
	    GotArg[i] = 0;
    switch (intermediate) {
    case 0:
	switch (c) {
	case 'H': case 'f':
	    if (!GotArg[0]) a1 = 1;
	    if (!GotArg[1]) a2 = 1;
	    if (curr->origin)
		a1 += curr->top;
	    if (a1 < 1)
		a1 = 1;
	    if (a1 > rows)
		a1 = rows;
	    if (a2 < 1)
		a2 = 1;
	    if (a2 > cols)
		a2 = cols;
	    a1--; a2--;
	    Goto (curr->y, curr->x, a1, a2);
	    curr->y = a1;
	    curr->x = a2;
	    break;
	case 'J':
	    if (!GotArg[0] || a1 < 0 || a1 > 2)
		a1 = 0;
	    switch (a1) {
	    case 0:
		ClearToEOS (); break;
	    case 1:
		ClearFromBOS (); break;
	    case 2:
		ClearScreen ();
		Goto (0, 0, curr->y, curr->x);
		break;
	    }
	    break;
	case 'K':
	    if (!GotArg[0] || a1 < 0 || a2 > 2)
		a1 = 0;
	    switch (a1) {
	    case 0:
		ClearToEOL (); break;
	    case 1:
		ClearFromBOL (); break;
	    case 2:
		ClearLine (); break;
	    }
	    break;
	case 'A':
	    CursorUp (GotArg[0] ? a1 : 1);
	    break;
	case 'B':
	    CursorDown (GotArg[0] ? a1 : 1);
	    break;
	case 'C':
	    CursorRight (GotArg[0] ? a1 : 1);
	    break;
	case 'D':
	    CursorLeft (GotArg[0] ? a1 : 1);
	    break;
	case 'm':
	    SelectRendition ();
	    break;
	case 'g':
	    if (!GotArg[0] || a1 == 0)
		curr->tabs[curr->x] = 0;
	    else if (a1 == 3)
		bzero (curr->tabs, cols);
	    break;
	case 'r':
	    if (!CS)
		break;
	    if (!GotArg[0]) a1 = 1;
	    if (!GotArg[1]) a2 = rows;
	    if (a1 < 1 || a2 > rows || a1 >= a2)
		break;
	    curr->top = a1-1;
	    curr->bot = a2-1;
	    PutStr (tgoto (CS, curr->bot, curr->top));
	    if (curr->origin) {
		Goto (-1, -1, curr->top, 0);
		curr->y = curr->top;
		curr->x = 0;
	    } else {
		Goto (-1, -1, 0, 0);
		curr->y = curr->x = 0;
	    }
	    break;
	case 'I':
	    if (!GotArg[0]) a1 = 1;
	    while (a1--)
		ForwardTab ();
	    break;
	case 'Z':
	    if (!GotArg[0]) a1 = 1;
	    while (a1--)
		BackwardTab ();
	    break;
	case 'L':
	    InsertLine (GotArg[0] ? a1 : 1);
	    break;
	case 'M':
	    DeleteLine (GotArg[0] ? a1 : 1);
	    break;
	case 'P':
	    DeleteChar (GotArg[0] ? a1 : 1);
	    break;
	case '@':
	    InsertChar (GotArg[0] ? a1 : 1);
	    break;
	case 'h':
	    SetMode (1);
	    break;
	case 'l':
	    SetMode (0);
	    break;
	}
	break;
    case '?':
	if (c != 'h' && c != 'l')
	    break;
	if (!GotArg[0])
	    break;
	i = (c == 'h');
	if (a1 == 5) {
	    if (i) {
		curr->vbwait = 1;
	    } else {
		if (curr->vbwait)
		    PutStr (VB);
		curr->vbwait = 0;
	    }
	} else if (a1 == 6) {
	    curr->origin = i;
	    if (curr->origin) {
		Goto (curr->y, curr->x, curr->top, 0);
		curr->y = curr->top;
		curr->x = 0;
	    } else {
		Goto (curr->y, curr->x, 0, 0);
		curr->y = curr->x = 0;
	    }
	} else if (a1 == 7) {
	    curr->wrap = i;
	}
	break;
    }
}

static PutChar (c) {
    putchar (c);
}

static PutStr (s) char *s; {
    if (display && s)
	tputs (s, 1, PutChar);
}

static SetChar (c) register c; {
    register struct win *p = curr;

    p->image[p->y][p->x] = c;
    p->attr[p->y][p->x] = p->LocalAttr;
}

static StartString (type) enum string_t type; {
    curr->StringType = type;
    curr->stringp = curr->string;
    curr->state = STR;
}

static AddChar (c) {
    if (curr->stringp > curr->string+MAXSTR-1)
	curr->state = LIT;
    else
	*(curr->stringp)++ = c;
}

/* Insert mode is a toggle on some terminals, so we need this hack:
 */
InsertMode (on) {
    if (on) {
	if (!insert)
	    PutStr (IM);
    } else if (insert)
	PutStr (EI);
    insert = on;
}

/* ...and maybe keypad application mode is a toggle, too:
 */
KeypadMode (on) {
    if (on) {
	if (!keypad)
	    PutStr (KS);
    } else if (keypad)
	PutStr (KE);
    keypad = on;
}

static SaveCursor () {
    curr->saved = 1;
    curr->Saved_x = curr->x;
    curr->Saved_y = curr->y;
    curr->SavedLocalAttr = curr->LocalAttr;
}

static RestoreCursor () {
    if (curr->saved) {
	curr->LocalAttr = curr->SavedLocalAttr;
	NewRendition (GlobalAttr, curr->LocalAttr);
	GlobalAttr = curr->LocalAttr;
	Goto (curr->y, curr->x, curr->Saved_y, curr->Saved_x);
	curr->x = curr->Saved_x;
	curr->y = curr->Saved_y;
    }
}

/*ARGSUSED*/
CountChars (c) {
    StrCost++;
}

CalcCost (s) register char *s; {
    if (s) {
	StrCost = 0;
	tputs (s, 1, CountChars);
	return StrCost;
    } else return EXPENSIVE;
}

static Goto (y1, x1, y2, x2) {
    register dy, dx;
    register cost = 0;
    register char *s;
    int CMcost, n, m;
    enum move_t xm = M_NONE, ym = M_NONE;

    if (!display)
	return;
    if (x1 == cols || x2 == cols) {
	if (x2 == cols) --x2;
	goto DoCM;
    }
    dx = x2 - x1;
    dy = y2 - y1;
    if (dy == 0 && dx == 0)
	return;
    if (y1 == -1 || x1 == -1) {
DoCM:
	PutStr (tgoto (CM, x2, y2));
	return;
    }
    CMcost = CalcCost (tgoto (CM, x2, y2));
    if (dx > 0) {
	if ((n = RewriteCost (y1, x1, x2)) < (m = dx * NDcost)) {
	    cost = n;
	    xm = M_RW;
	} else {
	    cost = m;
	    xm = M_RI;
	}
    } else if (dx < 0) {
	cost = -dx * LEcost;
	xm = M_LE;
    }
    if (dx && (n = RewriteCost (y1, 0, x2) + CRcost) < cost) {
	cost = n;
	xm = M_CR;
    }
    if (cost >= CMcost)
	goto DoCM;
    if (dy > 0) {
	cost += dy * DOcost;
	ym = M_DO;
    } else if (dy < 0) {
	cost += -dy * UPcost;
	ym = M_UP;
    }
    if (cost >= CMcost)
	goto DoCM;
    if (xm != M_NONE) {
	if (xm == M_LE || xm == M_RI) {
	    if (xm == M_LE) {
		s = BC; dx = -dx;
	    } else s = ND;
	    while (dx-- > 0)
		PutStr (s);
	} else {
	    if (xm == M_CR) {
		PutStr (CR);
		x1 = 0;
	    }
	    if (x1 < x2) {
		if (curr->insert)
		    InsertMode (0);
		for (s = curr->image[y1]+x1; x1 < x2; x1++, s++)
		    putchar (*s);
		if (curr->insert)
		    InsertMode (1);
	    }
	}
    }
    if (ym != M_NONE) {
	if (ym == M_UP) {
	    s = UP; dy = -dy;
	} else s = DO;
	while (dy-- > 0)
	    PutStr (s);
    }
}

static RewriteCost (y, x1, x2) {
    register cost, dx;
    register char *p = curr->attr[y]+x1;

    if (AM && y == rows-1 && x2 == cols-1)
	return EXPENSIVE;
    cost = dx = x2 - x1;
    if (dx == 0)
	return 0;
    if (curr->insert)
	cost += EIcost + IMcost;
    do {
	if (*p++ != GlobalAttr)
	    return EXPENSIVE;
    } while (--dx);
    return cost;
}

static BackSpace () {
    if (curr->x > 0) {
	if (curr->x < cols) {
	    if (BC)
		PutStr (BC);
	    else
		Goto (curr->y, curr->x, curr->y, curr->x-1);
	}
	curr->x--;
    }
}

static Return () {
    if (curr->x > 0) {
	curr->x = 0;
	PutStr (CR);
    }
}

static LineFeed () {
    if (curr->y == curr->bot) {
	ScrollUpMap (curr->image);
	ScrollUpMap (curr->attr);
    } else if (curr->y < rows-1) {
	curr->y++;
    }
    PutStr (NL);
}

static ReverseLineFeed () {
    if (curr->y == curr->top) {
	ScrollDownMap (curr->image);
	ScrollDownMap (curr->attr);
	if (SR) {
	    PutStr (SR);
	} else if (AL) {
	    Goto (curr->top, curr->x, curr->top, 0);
	    PutStr (AL);
	    Goto (curr->top, 0, curr->top, curr->x);
	} else Redisplay ();
    } else if (curr->y > 0) {
	CursorUp (1);
    }
}

static InsertAChar (c) {
    register y = curr->y, x = curr->x;

    if (x == cols)
	x--;
    bcopy (curr->image[y], OldImage, cols);
    bcopy (curr->attr[y], OldAttr, cols);
    bcopy (curr->image[y]+x, curr->image[y]+x+1, cols-x-1);
    bcopy (curr->attr[y]+x, curr->attr[y]+x+1, cols-x-1);
    SetChar (c);
    if (!display)
	return;
    if (IC || IM) {
	if (!curr->insert)
	    InsertMode (1);
	PutStr (IC);
	putchar (c);
	if (!curr->insert)
	    InsertMode (0);
    } else {
	RedisplayLine (OldImage, OldAttr, y, x, cols-1);
	++x;
	Goto (y, last_x, y, x);
    }
}

static InsertChar (n) {
    register i, y = curr->y, x = curr->x;

    if (x == cols)
	return;
    bcopy (curr->image[y], OldImage, cols);
    bcopy (curr->attr[y], OldAttr, cols);
    if (n > cols-x)
	n = cols-x;
    bcopy (curr->image[y]+x, curr->image[y]+x+n, cols-x-n);
    bcopy (curr->attr[y]+x, curr->attr[y]+x+n, cols-x-n);
    ClearInLine (0, y, x, x+n-1);
    if (!display)
	return;
    if (IC || IM) {
	if (!curr->insert)
	    InsertMode (1);
	for (i = n; i; i--) {
	    PutStr (IC);
	    putchar (' ');
	}
	if (!curr->insert)
	    InsertMode (0);
	Goto (y, x+n, y, x);
    } else {
	RedisplayLine (OldImage, OldAttr, y, x, cols-1);
	Goto (y, last_x, y, x);
    }
}

static DeleteChar (n) {
    register i, y = curr->y, x = curr->x;

    if (x == cols)
	return;
    bcopy (curr->image[y], OldImage, cols);
    bcopy (curr->attr[y], OldAttr, cols);
    if (n > cols-x)
	n = cols-x;
    bcopy (curr->image[y]+x+n, curr->image[y]+x, cols-x-n);
    bcopy (curr->attr[y]+x+n, curr->attr[y]+x, cols-x-n);
    ClearInLine (0, y, cols-n, cols-1);
    if (!display)
	return;
    if (DC) {
	for (i = n; i; i--)
	    PutStr (DC);
    } else {
	RedisplayLine (OldImage, OldAttr, y, x, cols-1);
	Goto (y, last_x, y, x);
    }
}

static DeleteLine (n) {
    register i, old = curr->top;

    if (n > curr->bot-curr->y+1)
	n = curr->bot-curr->y+1;
    curr->top = curr->y;
    for (i = n; i; i--) {
	ScrollUpMap (curr->image);
	ScrollUpMap (curr->attr);
    }
    if (DL) {
	Goto (curr->y, curr->x, curr->y, 0);
	for (i = n; i; i--)
	    PutStr (DL);
	Goto (curr->y, 0, curr->y, curr->x);
    } else if (CS) {
	PutStr (tgoto (CS, curr->bot, curr->top));
	Goto (-1, -1, curr->bot, 0);
	for (i = n; i; i--)
	    PutStr (SF);
	PutStr (tgoto (CS, curr->bot, old));
	Goto (-1, -1, curr->y, curr->x);
    } else Redisplay ();
    curr->top = old;
}

static InsertLine (n) {
    register i, old = curr->top;

    if (n > curr->bot-curr->y+1)
	n = curr->bot-curr->y+1;
    curr->top = curr->y;
    for (i = n; i; i--) {
	ScrollDownMap (curr->image);
	ScrollDownMap (curr->attr);
    }
    if (AL) {
	Goto (curr->y, curr->x, curr->y, 0);
	for (i = n; i; i--)
	    PutStr (AL);
	Goto (curr->y, 0, curr->y, curr->x);
    } else if (CS && SR) {
	PutStr (tgoto (CS, curr->bot, curr->top));
	Goto (-1, -1, curr->y, 0);
	for (i = n; i; i--)
	    PutStr (SR);
	PutStr (tgoto (CS, curr->bot, old));
	Goto (-1, -1, curr->y, curr->x);
    } else Redisplay ();
    curr->top = old;
}

static ScrollUpMap (pp) char **pp; {
    register char *tmp = pp[curr->top];

    bcopy (pp+curr->top+1, pp+curr->top,
	(curr->bot-curr->top) * sizeof (char *));
    if (pp == curr->image)
	bclear (tmp, cols);
    else
	bzero (tmp, cols);
    pp[curr->bot] = tmp;
}

static ScrollDownMap (pp) char **pp; {
    register char *tmp = pp[curr->bot];

    bcopy (pp+curr->top, pp+curr->top+1,
	(curr->bot-curr->top) * sizeof (char *));
    if (pp == curr->image)
	bclear (tmp, cols);
    else
	bzero (tmp, cols);
    pp[curr->top] = tmp;
}

static ForwardTab () {
    register x = curr->x;

    if (curr->tabs[x] && x < cols-1)
	++x;
    while (x < cols-1 && !curr->tabs[x])
	x++;
    Goto (curr->y, curr->x, curr->y, x);
    curr->x = x;
}

static BackwardTab () {
    register x = curr->x;

    if (curr->tabs[x] && x > 0)
	x--;
    while (x > 0 && !curr->tabs[x])
	x--;
    Goto (curr->y, curr->x, curr->y, x);
    curr->x = x;
}

static ClearScreen () {
    register i;

    PutStr (CL);
    for (i = 0; i < rows; ++i) {
	bclear (curr->image[i], cols);
	bzero (curr->attr[i], cols);
    }
}

static ClearFromBOS () {
    register n, y = curr->y, x = curr->x;

    for (n = 0; n < y; ++n)
	ClearInLine (1, n, 0, cols-1);
    ClearInLine (1, y, 0, x);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearToEOS () {
    register n, y = curr->y, x = curr->x;

    if (CD)
	PutStr (CD);
    ClearInLine (!CD, y, x, cols-1);
    for (n = y+1; n < rows; n++)
	ClearInLine (!CD, n, 0, cols-1);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearLine () {
    register y = curr->y, x = curr->x;

    ClearInLine (1, y, 0, cols-1);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearToEOL () {
    register y = curr->y, x = curr->x;

    ClearInLine (1, y, x, cols-1);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearFromBOL () {
    register y = curr->y, x = curr->x;

    ClearInLine (1, y, 0, x);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearInLine (displ, y, x1, x2) {
    register i, n;

    if (x1 == cols) x1--;
    if (x2 == cols) x2--;
    if (n = x2 - x1 + 1) {
	bclear (curr->image[y]+x1, n);
	bzero (curr->attr[y]+x1, n);
	if (displ && display) {
	    if (x2 == cols-1 && CE) {
		Goto (curr->y, curr->x, y, x1);
		curr->y = y; curr->x = x1;
		PutStr (CE);
		return;
	    }
	    if (y == rows-1 && AM)
		--n;
	    if (n == 0)
		return;
	    SaveAttr ();
	    Goto (curr->y, curr->x, y, x1);
	    for (i = n; i > 0; i--)
		putchar (' ');
	    curr->y = y; curr->x = x1 + n;
	    RestoreAttr ();
	}
    }
}

static CursorRight (n) register n; {
    register x = curr->x;

    if (x == cols)
	return;
    if ((curr->x += n) >= cols)
	curr->x = cols-1;
    Goto (curr->y, x, curr->y, curr->x);
}

static CursorUp (n) register n; {
    register y = curr->y;

    if ((curr->y -= n) < curr->top)
	curr->y = curr->top;
    Goto (y, curr->x, curr->y, curr->x);
}

static CursorDown (n) register n; {
    register y = curr->y;

    if ((curr->y += n) > curr->bot)
	curr->y = curr->bot;
    Goto (y, curr->x, curr->y, curr->x);
}

static CursorLeft (n) register n; {
    register x = curr->x;

    if ((curr->x -= n) < 0)
	curr->x = 0;
    Goto (curr->y, x, curr->y, curr->x);
}

static SetMode (on) {
    register i;

    for (i = 0; i < NumArgs; ++i) {
	switch (args[i]) {
	case 4:
	    curr->insert = on;
	    InsertMode (on);
	    break;
	}
    }
}

static SelectRendition () {
    register i, old = GlobalAttr;

    if (NumArgs == 0)
	SetRendition (0);
    else for (i = 0; i < NumArgs; ++i)
	SetRendition (args[i]);
    NewRendition (old, GlobalAttr);
}

static SetRendition (n) register n; {
    switch (n) {
    case 0:
	GlobalAttr = 0; break;
    case 1:
	GlobalAttr |= A_BD; break;
    case 2:
	GlobalAttr |= A_DI; break;
    case 3:
	GlobalAttr |= A_SO; break;
    case 4:
	GlobalAttr |= A_US; break;
    case 5:
	GlobalAttr |= A_BL; break;
    case 7:
	GlobalAttr |= A_RV; break;
    case 22:
	GlobalAttr &= ~(A_BD|A_SO|A_DI); break;
    case 23:
	GlobalAttr &= ~A_SO; break;
    case 24:
	GlobalAttr &= ~A_US; break;
    case 25:
	GlobalAttr &= ~A_BL; break;
    case 27:
	GlobalAttr &= ~A_RV; break;
    }
    curr->LocalAttr = GlobalAttr;
}

static NewRendition (old, new) register old, new; {
    register i;

    if (old == new)
	return;
    for (i = 1; i <= A_MAX; i <<= 1) {
	if ((old & i) && !(new & i)) {
	    PutStr (UE);
	    PutStr (SE);
	    PutStr (ME);
	    if (new & A_US) PutStr (US);
	    if (new & A_SO) PutStr (SO);
	    if (new & A_BL) PutStr (MB);
	    if (new & A_BD) PutStr (MD);
	    if (new & A_DI) PutStr (MH);
	    if (new & A_RV) PutStr (MR);
	    return;
	}
    }
    if ((new & A_US) && !(old & A_US))
	PutStr (US);
    if ((new & A_SO) && !(old & A_SO))
	PutStr (SO);
    if ((new & A_BL) && !(old & A_BL))
	PutStr (MB);
    if ((new & A_BD) && !(old & A_BD))
	PutStr (MD);
    if ((new & A_DI) && !(old & A_DI))
	PutStr (MH);
    if ((new & A_RV) && !(old & A_RV))
	PutStr (MR);
}

static SaveAttr () {
    NewRendition (GlobalAttr, 0);
    if (curr->insert)
	InsertMode (0);
}

static RestoreAttr () {
    NewRendition (0, GlobalAttr);
    if (curr->insert)
	InsertMode (1);
}

static FillWithEs () {
    register i;
    register char *p, *ep;

    curr->y = curr->x = 0;
    NewRendition (GlobalAttr, 0);
    SetRendition (0);
    for (i = 0; i < rows; ++i) {
	bzero (curr->attr[i], cols);
	p = curr->image[i];
	ep = p + cols;
	for ( ; p < ep; ++p)
	    *p = 'E';
    }
    Redisplay ();
}

static Redisplay () {
    register i;

    PutStr (CL);
    TmpAttr = GlobalAttr;
    last_x = last_y = 0;
    for (i = 0; i < rows; ++i)
	DisplayLine (blank, null, curr->image[i], curr->attr[i], i, 0, cols-1);
    NewRendition (TmpAttr, GlobalAttr);
    Goto (last_y, last_x, curr->y, curr->x);
}

static DisplayLine (os, oa, s, as, y, from, to)
	register char *os, *oa, *s, *as; {
    register i, x, a;

    if (to == cols)
	--to;
    if (AM && y == rows-1 && to == cols-1)
	--to;
    a = TmpAttr;
    for (x = i = from; i <= to; ++i, ++x) {
	if (s[i] == os[i] && as[i] == oa[i] && as[i] == a)
	    continue;
	Goto (last_y, last_x, y, x);
	last_y = y;
	last_x = x;
	if ((a = as[i]) != TmpAttr) {
	    NewRendition (TmpAttr, a);
	    TmpAttr = a;
	}
	putchar (s[i]);
	last_x++;
    }
}

static RedisplayLine (os, oa, y, from, to) char *os, *oa; {
    if (curr->insert)
	InsertMode (0);
    NewRendition (GlobalAttr, 0);
    TmpAttr = 0;
    last_y = y;
    last_x = from;
    DisplayLine (os, oa, curr->image[y], curr->attr[y], y, from, to);
    NewRendition (TmpAttr, GlobalAttr);
    if (curr->insert)
	InsertMode (1);
}

static MakeBlankLine (p, n) register char *p; register n; {
    do *p++ = ' ';
    while (--n);
}

MakeStatus (msg, wp) char *msg; struct win *wp; {
    struct win *ocurr = curr;
    int odisplay = display;
    register char *s, *t;
    register max = AM ? cols-1 : cols;

    for (s = t = msg; *s && t - msg < max; ++s)
	if (*s >= ' ' && *s <= '~')
	    *t++ = *s;
    *t = '\0';
    curr = wp;
    display = 1;
    if (status) {
	if (time ((time_t *)0) - TimeDisplayed < 2)
	    sleep (1);
	RemoveStatus (wp);
    }
    if (t > msg) {
	status = 1;
	StatLen = t - msg;
	Goto (curr->y, curr->x, rows-1, 0);
	NewRendition (GlobalAttr, A_SO);
	if (curr->insert)
	    InsertMode (0);
	printf ("%s", msg);
	if (curr->insert)
	    InsertMode (1);
	NewRendition (A_SO, GlobalAttr);
	fflush (stdout);
	time (&TimeDisplayed);
    }
    curr = ocurr;
    display = odisplay;
}

RemoveStatus (p) struct win *p; {
    struct win *ocurr = curr;
    int odisplay = display;

    if (!status)
	return;
    status = 0;
    curr = p;
    display = 1;
    Goto (-1, -1, rows-1, 0);
    RedisplayLine (null, null, rows-1, 0, StatLen);
    Goto (rows-1, last_x, curr->y, curr->x);
    curr = ocurr;
    display = odisplay;
}
