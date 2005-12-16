/* Copyright (c) 1987,1988 Oliver Laumann, Technical University of Berlin.
 * Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the author is not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 *
 *	Modified by Patrick Wolfe (pat@kai.com, kailand!pat)
 *	Do whatever you want with (my modifications of) this program, but
 *	don't claim you wrote them, don't try to make money from them, and
 *	don't remove this notice.
 *
 *   And another modification (Dec90):
 *   Copy & paste added by hackers@immd4
 */

char AnsiVersion[] = "ansi 2.1 27-Feb-1989";

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/ioctl.h>
#include "screen.h"

#define MAXLEN 1024
#ifdef TIOCSWINSZ
#define LASTMESSAGES	10	/* number of messages that will be kept */

struct	winsize	Window_Size;
#endif
struct	{ int rows, cols; } TS[2];
int	TermSizeLocked;
int	lastsize = -1;
extern	struct	win	*other;
/* extern struct win *wtab[]; * we really dont need it here */
static	char	*Z[2];
int	NWokay = 0;

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

#define EXPENSIVE    1000

#define G0           0
#define G1           1
#define G2           2
#define G3           3

#define ASCII        0

extern char *getenv(), *tgetstr(), *tgoto();
#ifndef USEBCOPY
extern void bcopy();
#endif
extern void bclear();
/* extern char *malloc(); */

#ifndef SYSV
extern void bclear(), bzero();
#else
void bzero(b, len)
	char *b;
	int len;
{
	memset(b, 0, len);
}
#endif
static resize();

/*
 * trows and tcols are the size of the terminal.
 */
int trows, tcols;
int status;
int flowctl;
char Term[16];
char Termcap[1024];
char *blank;
char PC;
int ISO2022;
time_t TimeDisplayed;

static char tbuf[1024], tentry[1024];
static char *tp;
static char *TI, *TE, *BL, *VB, *BC, *CR, *NL, *CL, *IS;
static char *US, *UE, *SO, *SE, *CD, *DO, *SR, *SF, *AL;
static char *CS, *DL, *DC, *IC, *IM, *EI, *UP, *ND, *KS, *KE;
static char *MB, *MD, *MH, *MR, *ME, *PO, *PF;
static char *CDC, *CDL, *CAL;
static char *WS;
static AM;
static char GlobalAttr, TmpAttr, GlobalCharset, TmpCharset;
static char *OldImage, *OldAttr, *OldFont;
static last_x, last_y;
struct win *curr;
static StrCost;
static UPcost, DOcost, LEcost, NDcost, CRcost, IMcost, EIcost;
static tcLineLen = 100;
static StatLen;
static insert;
static keypad;
static display = 1;
static initterm(), MakeString(), Special(), DoESC(), DoCSI();
static PutStr(), SetChar(), StartString(), AddChar(), PrintChar(), PrintFlush();
static InsertMode(), KeypadMode(), DesignateCharset(), MapCharset();
static NewCharset(), SaveCursor(), RestoreCursor(), CalcCost(), Goto();
static RewriteCost(), BackSpace(), Return(), LineFeed(), ReverseLineFeed();
static InsertAChar(), DeleteChar(), DeleteLine(), InsertLine(), ScrollUpMap();
static ScrollDownMap(), ForwardTab(), BackwardTab(), ClearScreen();
static ClearFromBOS(), ClearLine(), ClearToEOL(), ClearFromBOL();
static ClearInLine(), CursorRight(), CursorUp(), CursorDown(), CursorLeft();
static SetMode(), SelectRendition(), SetRendition(), NewRendition();
static FillWithEs(), Redisplay(), DisplayLine(), MakeBlankLine();
static InsertChar(), ClearToEOS();

int RedisplayLine(), SaveAttr(), RestoreAttr();
char *CM, *CE, *null;

static char *KeyCaps[] = {
    "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8", "k9",
    "kb", "kd", "kh", "kl", "ko", "kr", "ku",
    0
};

static char TermcapConst[] = "|VT 100/ANSI X3.64 virtual terminal|\\\n\
\t:DO=\\E[%dB:LE=\\E[%dD:RI=\\E[%dC:UP=\\E[%dA:bs:bt=\\E[Z:\\\n\
\t:cd=\\E[J:ce=\\E[K:cl=\\E[2J\\E[H:cm=\\E[%i%d;%dH:ct=\\E[3g:\\\n\
\t:do=\\E[B:nd=\\E[C:pt:rc=\\E8:rs=\\Ec:sc=\\E7:st=\\EH:up=\\E[A:le=^H:";

struct Msg {
    char *text;
    time_t  date;
    struct Msg *next;
    struct Msg *prev;
};

static struct Msg *FirstMsg;
static struct Msg *NextMsg;
extern struct attr defattr;

#define MSGEXPIRE (60 * 60 * 24 * 2) /* two days */


InitTerm () {
    register char *s;

    if ((s = getenv ("TERM")) == 0)
	Msg (0, "No TERM in environment.");
    initterm(s);
}

ChangeTerm (s)
register char *s;
{
    initterm (s);
}

static initterm (s)
register char *s;
{
    if (tgetent (tbuf, s) != 1) {
	Msg (0, "Cannot find termcap entry for %s.", s);
	return;
    }
/*
 *	to make the screen switching commands and escape codes work
 *	correctly, you must tell screen what escape codes to use
 *	by adding the non-standard TERMCAP fields Z0 and Z1.
 *	VT/ANSI terminals want to add:
 *	Z0 : select wide character set.
 *	Z1 : select narrow character set.
 *		:Z0=\E[?3l:Z1=\E[?3h:
 *	users of braindamaged wyse50s want:
 *		:Z1=\E`;:Z0=\E`\072:
 */
	tp = tentry; /* we need it everytime, we come here, right? jw. */
    Z[0] = tgetstr("Z0", &tp);
    Z[1] = tgetstr("Z1", &tp);

    WS = tgetstr ("WS", &tp);

    NWokay = ( Z[0] && Z[1] ) || WS;

    tcols = tgetnum ("co");
    trows = tgetnum ("li");

#ifdef TIOCSWINSZ
    {
	struct	winsize	ws;
        ioctl (0, TIOCGWINSZ, &ws);
        /*
         * believe the Window_Size. This may be wrong, in case we work
         * on a terminal via rlogin on a pty that has previously been used
         * by some windowing system (suntools), but we must take
         * this risk, or we are not able to work correctly from a resized
         * Window.
         */
        tcols = ws.ws_col > 0 ? ws.ws_col : tcols;
        trows = ws.ws_row > 0 ? ws.ws_row : trows;
    }
#endif
    
    defattr.rows = trows;
    defattr.cols = tcols;

    /*
     * if we have a terminal, that can switch between two different
     * sizes, we look for additional capabilities "LI" and "CO",
     * which are the lines and colums for the second mode.
     */

    if( NWokay) {
        if( TS[1].cols = tgetnum ("CO") <= 0)
            TS[1].cols = 132; /* HACK */
        if( TS[1].rows = tgetnum ("LI") <= 0)
	    TS[1].rows = 24;  /* HACK */
    }

    TS[0].rows = trows;		/* this is also the saved screen size to */
    TS[0].cols = tcols;		/* which we switch back upon termination */

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
    CDC = tgetstr ("DC", &tp);
    CDL = tgetstr ("DL", &tp);
    CAL = tgetstr ("AL", &tp);
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
    /*
     * capability to change the size of the display (if your
     * are using a bitmapped terminal for example.
     */
    ISO2022 = tgetflag ("G0");
    PO = tgetstr ("po", &tp);
    if (!(PF = tgetstr ("pf", &tp)))
	PO = 0;
    blank = (char *) malloc (MAXLEN);
    null = (char *) malloc (MAXLEN);
    OldImage = (char *) malloc (MAXLEN);
    OldAttr = (char *) malloc (MAXLEN);
    OldFont = (char *) malloc (MAXLEN);
    if (!(blank && null && OldImage && OldAttr && OldFont))
	Msg (0, "Out of memory.");
    MakeBlankLine (blank, MAXLEN);
    bzero (null, MAXLEN);
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
#ifdef SIOCGWINSZ
    /*
     * we must  ignore this from here on, or we will get another
     * signal when we change the terminal size.
     */
    signale(SIGWINCH, SIG_DFL);
#endif
    PutStr (TE);
    InsertMode(0);
    KeypadMode(0);
    (void) ChangeTermSize(TS[0].rows, TS[0].cols);
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

char *MakeTermcap (wp)
struct win *wp;
{
    char buf[1024];
    register char **pp, *p;

    if( wp->term[strlen(wp->term) -1] == '-') {
        sprintf (Term, "TERM=%s%d", wp->term, wp->cols);
        sprintf (Termcap, "TERMCAP=SC|%s%d", wp->term, wp->cols);
    } else {
        sprintf (Term, "TERM=%s", wp->term);
        sprintf (Termcap, "TERMCAP=SC|%s", wp->term);
    }
    strcat (Termcap, TermcapConst);
    sprintf (buf, "li#%d:co#%d:", wp->rows, wp->cols);
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
    if ((CS && SR) || AL || CAL || wp->aflag) {
	AddCap ("sr=\\EM:");
	AddCap ("al=\\E[L:");
	AddCap ("AL=\\E[%dL:");
    }
    if (CS || DL || CDL || wp->aflag) {
	AddCap ("dl=\\E[M:");
	AddCap ("DL=\\E[%dM:");
    }
    if (CS)
	AddCap ("cs=\\E[%i%d;%dr:");
    if (DC || CDC || wp->aflag) {
	AddCap ("dc=\\E[P:");
	AddCap ("DC=\\E[%dP:");
    }
    if (IC || IM || wp->aflag) {
	AddCap ("im=\\E[4h:");
	AddCap ("ei=\\E[4l:");
	AddCap ("ic=:");
	AddCap ("IC=\\E[%d@:");
    }
    if (KS)
	AddCap ("ks=\\E=:");
    if (KE)
	AddCap ("ke=\\E>:");
    if (ISO2022)
	AddCap ("G0:");
    if (PO) {
	AddCap ("po=\\E[5i:");
	AddCap ("pf=\\E[4i:");
    }
    for (pp = KeyCaps; *pp; ++pp)
	if (p = tgetstr (*pp, &tp)) {
	    MakeString (*pp, buf, p);
	    AddCap (buf);
	}
    /*
     * I don't think the following capabilities are ansi norm,
     * so we put them in the termcap only if -a was specified.
     * Maybe we shouldn't even do this.
     */
    if (wp->aflag && NWokay ) {
	AddCap ("Z0=\\E[?3l:");
	AddCap ("Z1=\\E[?3h:");
	sprintf(buf, "CO=%d:", TS[1].cols);
	AddCap (buf);
	sprintf(buf, "LI=%d:", TS[1].rows);
	AddCap (buf);
    }
    return Termcap;
}

static MakeString (cap, buf, s) char *cap, *buf; register char *s; {
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
    NewCharset (GlobalCharset, curr->charsets[curr->LocalCharset]);
    GlobalCharset = curr->charsets[curr->LocalCharset];

    if( wp->cols != tcols || wp->rows != trows) {
        if( !ChangeTermSize( wp->rows, wp->cols)) {
            int nrows = wp->rows, ncols = wp->cols;
            BestTermSize( &nrows, &ncols);
            (void)ChangeTermSize( nrows, ncols);
	    ChangeWindowSize( wp, nrows, ncols);
	}
    }
    
    if (CS)
	PutStr (tgoto (CS, curr->bot, curr->top));/**/
    Redisplay ();
    InsertMode (curr->insert);
    KeypadMode (curr->keypad);
}

ResetScreen (p) register struct win *p; {
    register i;

    bzero (p->tabs, p->cols);
    for (i = 8; i < p->cols; i += 8)
	p->tabs[i] = 1;
    p->wrap = 1;
    p->origin = 0;
    p->insert = 0;
    p->vbwait = 0;
    p->keypad = 0;
    p->top = 0;
    p->bot = p->rows - 1;
    p->saved = 0;
    p->LocalAttr = 0;
    p->x = p->y = 0;
    p->state = LIT;
    p->StringType = NONE;
    p->ss = 0;
    p->LocalCharset = G0;
    for (i = G0; i <= G3; i++)
	p->charsets[i] = ASCII;
}

struct win * SetAnsiCurr(wp)
struct win *wp;
{
	struct win *w;
	w=curr;
    curr = wp;
    display = curr->active;
    if (display) {
	RemoveStatus (wp);
	if (curr->insert)
	    insert = 1;
	if (curr->keypad)
	    keypad = 1;
    }
	return w;
}


WriteString (wp, buf, len) struct win *wp; register char *buf; {
    register c, intermediate = 0;

    if (!len)
	return;
    curr = wp;
    display = curr->active;
    if (display) {
	RemoveStatus (wp);
	if (curr->insert)
	    insert = 1;
	if (curr->keypad)
	    keypad = 1;
    }
    do {
	c = *buf++;
	if (c == '\0' || c == '\177')
	    continue;
NextChar:
	switch (curr->state) {
	case PRIN:
	    switch (c) {
	    case '\033':
		curr->state = PRINESC; break;
	    default:
		PrintChar (c);
	    }
	    break;
	case PRINESC:
	    switch (c) {
	    case '[':
		curr->state = PRINCSI; break;
	    default:
		PrintChar ('\033'); PrintChar (c);
		curr->state = PRIN;
	    }
	    break;
	case PRINCSI:
	    switch (c) {
	    case '4':
		curr->state = PRIN4; break;
	    default:
		PrintChar ('\033'); PrintChar ('['); PrintChar (c);
		curr->state = PRIN;
	    }
	    break;
	case PRIN4:
	    switch (c) {
	    case 'i':
		curr->state = LIT;
		PrintFlush ();
		break;
	    default:
		PrintChar ('\033'); PrintChar ('['); PrintChar ('4');
		PrintChar (c);
		curr->state = PRIN;
	    }
	    break;
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
		curr->NumArgs = 0;
		intermediate = 0;
		bzero ((char *)curr->args, MAXARGS * sizeof (int));
		bzero (curr->GotArg, MAXARGS);
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
		if (curr->NumArgs < MAXARGS) {
		    curr->args[curr->NumArgs] =
                        10 * curr->args[curr->NumArgs] + c - '0';
		    curr->GotArg[curr->NumArgs] = 1;
		}
		break;
	    case ';': case ':':
		curr->NumArgs++; break;
	    default:
		if (Special (c))
		    break;
		if (c >= '@' && c <= '~') {
		    curr->NumArgs++;
		    DoCSI (c, intermediate);
		    if (curr->state != PRIN)
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
		} else {
		    if (curr->ss)
				NewCharset (GlobalCharset, curr->charsets[curr->ss]);
		    if (curr->x < curr->cols-1) {
			if (curr->insert) {
			    InsertAChar (c);
			} else {
			    if (display)
				putchar (c);
			    SetChar (c);
			}
			curr->x++;
		    } else if (curr->x == curr->cols-1) {
			SetChar (c);
			if (!(AM && curr->y == curr->bot)) {
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
			} else curr->x = curr->cols;
		    }
		    if (curr->ss) {
			NewCharset (curr->charsets[curr->ss], GlobalCharset);
			curr->ss = 0;
		    }
		}
	    }
	}
    } while (--len);
    curr->outlen = 0;
    if (curr->state == PRIN)
	PrintFlush ();
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
		if (!curr->visualbell)
			PutStr (BL);
		else
			curr->bell = 2;
		if (!display)
	    	curr->bell = 1;
		return 1;
    case '\t':
	ForwardTab (); return 1;
    case '\017':   /* SI */
	MapCharset (G0); return 1;
    case '\016':   /* SO */
	MapCharset (G1); return 1;
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
	    NewCharset (GlobalCharset, ASCII);
	    GlobalCharset = ASCII;
	    if (curr->insert)
		InsertMode (0);
	    if (curr->keypad)
		KeypadMode (0);
	    if (CS)
		PutStr (tgoto (CS, curr->rows-1, 0));/**/
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
	case 'n':   /* LS2 */
	    MapCharset (G2); break;
	case 'o':   /* LS3 */
	    MapCharset (G3); break;
	case 'N':   /* SS2 */
	    if (GlobalCharset == curr->charsets[G2])
		curr->ss = 0;
	    else
		curr->ss = G2;
	    break;
	case 'O':   /* SS3 */
	    if (GlobalCharset == curr->charsets[G3])
		curr->ss = 0;
	    else
		curr->ss = G3;
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
    case '(':
	DesignateCharset (c, G0); break;
    case ')':
	DesignateCharset (c, G1); break;
    case '*':
	DesignateCharset (c, G2); break;
    case '+':
	DesignateCharset (c, G3); break;
    }
}

static DoCSI (c, intermediate) {
    register i, a1 = curr->args[0], a2 = curr->args[1];

    if (curr->NumArgs >= MAXARGS)
	curr->NumArgs = MAXARGS;
    for (i = 0; i < curr->NumArgs; ++i)
	if (curr->args[i] == 0)
	    curr->GotArg[i] = 0;
    switch (intermediate) {
    case 0:
	switch (c) {
	case 'H': case 'f':
	    if (!curr->GotArg[0]) a1 = 1;
	    if (!curr->GotArg[1]) a2 = 1;
	    if (curr->origin)
		a1 += curr->top;
	    if (a1 < 1)
		a1 = 1;
	    if (a1 > curr->rows)
		a1 = curr->rows;
	    if (a2 < 1)
		a2 = 1;
	    if (a2 > curr->cols)
		a2 = curr->cols;
	    a1--; a2--;
	    Goto (curr->y, curr->x, a1, a2);
	    curr->y = a1;
	    curr->x = a2;
	    break;
	case 'J':
	    if (!curr->GotArg[0] || a1 < 0 || a1 > 2)
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
	    if (!curr->GotArg[0] || a1 < 0 || a1 > 2)
		a1 %= 3;
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
	    CursorUp (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'B':
	    CursorDown (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'C':
	    CursorRight (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'D':
	    CursorLeft (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'm':
	    SelectRendition ();
	    break;
	case 'g':
	    if (!curr->GotArg[0] || a1 == 0)
		curr->tabs[curr->x] = 0;
	    else if (a1 == 3)
		bzero (curr->tabs, curr->cols);
	    break;
	case 'r':
	    if (!CS)
		break;
	    if (!curr->GotArg[0]) a1 = 1;
	    if (!curr->GotArg[1]) a2 = curr->rows;
	    if (a1 < 1 || a2 > curr->rows || a1 >= a2)
		break;
	    curr->top = a1-1;
	    curr->bot = a2-1; 
	    PutStr (tgoto (CS, curr->bot, curr->top));/**/
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
	    if (!curr->GotArg[0]) a1 = 1;
	    while (a1--)
		ForwardTab ();
	    break;
	case 'Z':
	    if (!curr->GotArg[0]) a1 = 1;
	    while (a1--)
		BackwardTab ();
	    break;
	case 'L':
	    InsertLine (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'M':
	    DeleteLine (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'P':
	    DeleteChar (curr->GotArg[0] ? a1 : 1);
	    break;
	case '@':
	    InsertChar (curr->GotArg[0] ? a1 : 1);
	    break;
	case 'h':
	    SetMode (1);
	    break;
	case 'l':
	    SetMode (0);
	    break;
	case 'i':
	    if (PO && curr->GotArg[0] && a1 == 5) {
		curr->stringp = curr->string;
		curr->state = PRIN;
	    }
	    break;
	}
	break;
    case '?':
	if (c != 'h' && c != 'l')
	    break;
	if (!curr->GotArg[0])
	    break;
	i = (c == 'h') ? 1 : 0;
	if (a1 == 3) {
	   if(ChangeTermSize( TS[1-i].rows, TS[1-i].cols))
	       ChangeWindowSize( curr, TS[1-i].rows, TS[1-i].cols);
	} else if (a1 == 5) {
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

PutChar (c) {
    putchar (c);
}

static PutStr (s) char *s; {
    if (display && s)
	tputs (s, 1, PutChar);
}

static CPutStr (s, c) char *s; { /* c is a "col"? jw. */
    if (display && s)
	tputs (tgoto (s, 0, c), 1, PutChar);  /* XXX */
}

static SetChar (c) register c; {
    register struct win *p = curr;

    p->image[p->y][p->x] = c;
    p->attr[p->y][p->x] = p->LocalAttr;
    p->font[p->y][p->x] = p->charsets[p->ss ? p->ss : p->LocalCharset];
}

static StartString (type) enum string_t type; {
    curr->StringType = type;
    curr->stringp = curr->string;
    curr->state = STR;
}

static AddChar (c) {
    if (curr->stringp >= curr->string+MAXSTR-1)
	curr->state = LIT;
    else
	*(curr->stringp)++ = c;
}

static PrintChar (c) {
    if (curr->stringp >= curr->string+MAXSTR-1)
	PrintFlush ();
    else
	*(curr->stringp)++ = c;
}

static PrintFlush () {
    if (curr->stringp > curr->string) {
	tputs (PO, 1, PutChar);
	(void) fflush (stdout);
	(void) write (1, curr->string, (unsigned) (curr->stringp - curr->string));
	tputs (PF, 1, PutChar);
	(void) fflush (stdout);
	curr->stringp = curr->string;
    }
}

/* Insert mode is a toggle on some terminals, so we need this hack:
 */
static InsertMode (on) {
    if (on) {
	if (!insert)
	    PutStr (IM);
    } else if (insert)
	PutStr (EI);
    insert = on;
}

/* ...and maybe keypad application mode is a toggle, too:
 */
static KeypadMode (on) {
    if (on) {
	if (!keypad)
	    PutStr (KS);
    } else if (keypad)
	PutStr (KE);
    keypad = on;
}

static DesignateCharset (c, n) {
    curr->ss = 0;
    if (c == 'B')
	c = ASCII;
    if (curr->charsets[n] != c) {
	curr->charsets[n] = c;
	if (curr->LocalCharset == n) {
	    NewCharset (GlobalCharset, c);
	    GlobalCharset = c;
	}
    }
}

static MapCharset (n) {
    curr->ss = 0;
    if (curr->LocalCharset != n) {
	curr->LocalCharset = n;
	NewCharset (GlobalCharset, curr->charsets[n]);
	GlobalCharset = curr->charsets[n];
    }
}

static NewCharset (old, new) {
    char buf[8];

    if (old == new)
	return;
    if (ISO2022) {
	sprintf (buf, "\033(%c", new == ASCII ? 'B' : new);
	PutStr (buf);
    }
}

static SaveCursor () {
    curr->saved = 1;
    curr->Saved_x = curr->x;
    curr->Saved_y = curr->y;
    curr->SavedLocalAttr = curr->LocalAttr;
    curr->SavedLocalCharset = curr->LocalCharset;
    bcopy ((char *)curr->charsets, (char *)curr->SavedCharsets,
	4 * sizeof (int));
}

static RestoreCursor () {
    if (curr->saved) {
	curr->LocalAttr = curr->SavedLocalAttr;
	NewRendition (GlobalAttr, curr->LocalAttr);
	GlobalAttr = curr->LocalAttr;
	bcopy ((char *)curr->SavedCharsets, (char *)curr->charsets,
	    4 * sizeof (int));
	curr->LocalCharset = curr->SavedLocalCharset;
	NewCharset (GlobalCharset, curr->charsets[curr->LocalCharset]);
	GlobalCharset = curr->charsets[curr->LocalCharset];
	Goto (curr->y, curr->x, curr->Saved_y, curr->Saved_x);
	curr->x = curr->Saved_x;
	curr->y = curr->Saved_y;
    }
}

/*ARGSUSED*/
static CountChars (c) {
    StrCost++;
}

static CalcCost (s) register char *s; {
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
    if (x1 == curr->cols || x2 == curr->cols) {
	if (x2 == curr->cols) --x2;
	goto DoCM;
    }
    dx = x2 - x1;
    dy = y2 - y1;
    if (dy == 0 && dx == 0)
	return;
    if (y1 == -1 || x1 == -1 || y2 >= curr->bot || y2 <= curr->top) {
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
    register char *p = curr->attr[y]+x1, *f = curr->font[y]+x1;

    if (AM && y == curr->rows-1 && x2 == curr->cols-1)
	return EXPENSIVE;
    cost = dx = x2 - x1;
    if (dx == 0)
	return 0;
    if (curr->insert)
	cost += EIcost + IMcost;
    do {
	if (*p++ != GlobalAttr || *f++ != GlobalCharset)
	    return EXPENSIVE;
    } while (--dx);
    return cost;
}

static BackSpace () {
    if (curr->x > 0) {
	if (curr->x < curr->cols) {
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
	ScrollUpMap (curr->font);
    } else if (curr->y < curr->rows-1) {
	curr->y++;
    }
    PutStr (NL);
}

static ReverseLineFeed () {
    if (curr->y == curr->top) {
	ScrollDownMap (curr->image);
	ScrollDownMap (curr->attr);
	ScrollDownMap (curr->font);
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

    if (x == curr->cols)
	x--;
    bcopy (curr->image[y], OldImage, curr->cols);
    bcopy (curr->attr[y], OldAttr, curr->cols);
    bcopy (curr->font[y], OldFont, curr->cols);
    bcopy (curr->image[y]+x, curr->image[y]+x+1, curr->cols-x-1);
    bcopy (curr->attr[y]+x, curr->attr[y]+x+1, curr->cols-x-1);
    bcopy (curr->font[y]+x, curr->font[y]+x+1, curr->cols-x-1);
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
	RedisplayLine (OldImage, OldAttr, OldFont, y, x, curr->cols-1);
	++x;
	Goto (y, last_x, y, x);
    }
}

static InsertChar (n) {
    register i, y = curr->y, x = curr->x;

    if (x == curr->cols)
	return;
    bcopy (curr->image[y], OldImage, curr->cols);
    bcopy (curr->attr[y], OldAttr, curr->cols);
    bcopy (curr->font[y], OldFont, curr->cols);
    if (n > curr->cols-x)
	n = curr->cols-x;
    bcopy (curr->image[y]+x, curr->image[y]+x+n, curr->cols-x-n);
    bcopy (curr->attr[y]+x, curr->attr[y]+x+n, curr->cols-x-n);
    bcopy (curr->font[y]+x, curr->font[y]+x+n, curr->cols-x-n);
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
	RedisplayLine (OldImage, OldAttr, OldFont, y, x, curr->cols-1);
	Goto (y, last_x, y, x);
    }
}

static DeleteChar (n) {
    register i, y = curr->y, x = curr->x;

    if (x == curr->cols)
	return;
    bcopy (curr->image[y], OldImage, curr->cols);
    bcopy (curr->attr[y], OldAttr, curr->cols);
    bcopy (curr->font[y], OldFont, curr->cols);
    if (n > curr->cols-x)
	n = curr->cols-x;
    bcopy (curr->image[y]+x+n, curr->image[y]+x, curr->cols-x-n);
    bcopy (curr->attr[y]+x+n, curr->attr[y]+x, curr->cols-x-n);
    bcopy (curr->font[y]+x+n, curr->font[y]+x, curr->cols-x-n);
    ClearInLine (0, y, curr->cols-n, curr->cols-1);
    if (!display)
	return;
    if (CDC && !(n == 1 && DC)) {
	CPutStr (CDC, n);
    } else if (DC) {
	for (i = n; i; i--)
	    PutStr (DC);
    } else {
	RedisplayLine (OldImage, OldAttr, OldFont, y, x, curr->cols-1);
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
	ScrollUpMap (curr->font);
    }
    if (DL || CDL) {
	Goto (curr->y, curr->x, curr->y, 0);
	if (CDL && !(n == 1 && DL)) {
	    CPutStr (CDL, n);
	} else {
	    for (i = n; i; i--)
		PutStr (DL);
	}
	Goto (curr->y, 0, curr->y, curr->x);
    } else if (CS) {
	PutStr (tgoto (CS, curr->bot, curr->top));/**/
	Goto (-1, -1, curr->bot, 0);
	for (i = n; i; i--)
	    PutStr (SF);
	PutStr (tgoto (CS, curr->bot, old));/**/
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
	ScrollDownMap (curr->font);
    }
    if (AL || CAL) {
	Goto (curr->y, curr->x, curr->y, 0);
	if (CAL && !(n == 1 && AL)) {
	    CPutStr (CAL, n);
	} else {
	    for (i = n; i; i--)
		PutStr (AL);
	}
	Goto (curr->y, 0, curr->y, curr->x);
    } else if (CS && SR) {
	PutStr (tgoto (CS, curr->bot, curr->top));/**/
	Goto (-1, -1, curr->y, 0);
	for (i = n; i; i--)
	    PutStr (SR);
	PutStr (tgoto (CS, curr->bot, old));/**/
	Goto (-1, -1, curr->y, curr->x);
    } else Redisplay ();
    curr->top = old;
}

static ScrollUpMap (pp) char **pp; {
    register char *tmp = pp[curr->top];

    bcopy ((char *)(pp+curr->top+1), (char *)(pp+curr->top),
	(int)((curr->bot-curr->top) * sizeof (char *)));
    if (pp == curr->image)
	bclear (tmp, curr->cols);
    else
	bzero (tmp, curr->cols);
    pp[curr->bot] = tmp;
}

static ScrollDownMap (pp) char **pp; {
    register char *tmp = pp[curr->bot];

    bcopy ((char *)(pp+curr->top), (char *)(pp+curr->top+1),
	(int)((curr->bot-curr->top) * sizeof (char *)));
    if (pp == curr->image)
	bclear (tmp, curr->cols);
    else
	bzero (tmp, curr->cols);
    pp[curr->top] = tmp;
}

static ForwardTab () {
    register x = curr->x;

    if (curr->tabs[x] && x < curr->cols-1)
	++x;
    while (x < curr->cols-1 && !curr->tabs[x])
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
    for (i = 0; i < curr->rows; ++i) {
	bclear (curr->image[i], curr->cols);
	bzero (curr->attr[i], curr->cols);
	bzero (curr->font[i], curr->cols);
    }
}

static ClearFromBOS () {
    register n, y = curr->y, x = curr->x;

    for (n = 0; n < y; ++n)
	ClearInLine (1, n, 0, curr->cols-1);
    ClearInLine (1, y, 0, x);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearToEOS () {
    register n, y = curr->y, x = curr->x;

    if (CD)
	PutStr (CD);
    ClearInLine (!CD, y, x, curr->cols-1);
    for (n = y+1; n < curr->rows; n++)
	ClearInLine (!CD, n, 0, curr->cols-1);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearLine () {
    register y = curr->y, x = curr->x;

    ClearInLine (1, y, 0, curr->cols-1);
    Goto (curr->y, curr->x, y, x);
    curr->y = y; curr->x = x;
}

static ClearToEOL () {
    register y = curr->y, x = curr->x;

    ClearInLine (1, y, x, curr->cols-1);
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

    if (x1 == curr->cols) x1--;
    if (x2 == curr->cols) x2--;
    if (n = x2 - x1 + 1) {
	bclear (curr->image[y]+x1, n);
	bzero (curr->attr[y]+x1, n);
	bzero (curr->font[y]+x1, n);
	if (displ && display) {
	    if (x2 == curr->cols-1 && CE) {
		Goto (curr->y, curr->x, y, x1);
		curr->y = y; curr->x = x1;
		PutStr (CE);
		return;
	    }
	    if (y == curr->rows-1 && AM)
		--n;
	    if (n == 0)
		return;
	    SaveAttr (0);
	    Goto (curr->y, curr->x, y, x1);
	    for (i = n; i > 0; i--)
		putchar (' ');
	    curr->y = y; curr->x = x1 + n;
	    RestoreAttr (0);
	}
    }
}

static CursorRight (n) register n; {
    register x = curr->x;

    if (x == curr->cols)
	return;
    if ((curr->x += n) >= curr->cols)
	curr->x = curr->cols-1;
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

    for (i = 0; i < curr->NumArgs; ++i) {
	switch (curr->args[i]) {
	case 4:
	    curr->insert = on;
	    InsertMode (on);
	    break;
	}
    }
}

static SelectRendition () {
    register i, old = GlobalAttr;

    if (curr->NumArgs == 0)
	SetRendition (0);
    else for (i = 0; i < curr->NumArgs; ++i)
	SetRendition (curr->args[i]);
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

SaveAttr (newattr) {
    NewRendition (GlobalAttr, newattr);
    NewCharset (GlobalCharset, ASCII);
    if (curr->insert)
	InsertMode (0);
}

RestoreAttr (oldattr) {
    NewRendition (oldattr, GlobalAttr);
    NewCharset (ASCII, GlobalCharset);
    if (curr->insert)
	InsertMode (1);
}

static FillWithEs () {
    register i;
    register char *p, *ep;

    curr->y = curr->x = 0;
    SaveAttr (0);
    for (i = 0; i < curr->rows; ++i) {
	bzero (curr->attr[i], curr->cols);
	bzero (curr->font[i], curr->cols);
	p = curr->image[i];
	ep = p + curr->cols;
	for ( ; p < ep; ++p)
	    *p = 'E';
    }
    RestoreAttr (0);
    Redisplay ();
}

static Redisplay () {
    register i, j;

    PutStr (CL);
    TmpAttr = GlobalAttr;
    TmpCharset = GlobalCharset;
    if (curr->insert || other->insert)	/* make sure insert mode is off (if it was on) */
        InsertMode (0);
    j = curr->insert;	/* save status of insert mode */
    curr->insert = 0;
    last_x = last_y = 0;
    for (i = 0; i < curr->rows; ++i)
	DisplayLine (blank, null, null, curr->image[i], curr->attr[i],
	    curr->font[i], i, 0, curr->cols-1);
    curr->insert = j;	/* restore insert mode */
    if (curr->insert)
        InsertMode (1);	/* turn it back on, if needed */
    NewRendition (TmpAttr, GlobalAttr);
    NewCharset (TmpCharset, GlobalCharset);
    Goto (last_y, last_x, curr->y, curr->x);
}

static DisplayLine (os, oa, of, s, as, fs, y, from, to)
	register char *os, *oa, *of, *s, *as, *fs; {
    register i, x, a, f;

    if (to == curr->cols)
	--to;
    if (AM && y == curr->rows-1 && to == curr->cols-1)
	--to;
    a = TmpAttr;
    f = TmpCharset;
    for (x = i = from; i <= to; ++i, ++x) {
	if (s[i] == os[i] && as[i] == oa[i] && as[i] == a
		          && of[i] == fs[i] && fs[i] == f)
	    continue;
	Goto (last_y, last_x, y, x);
	last_y = y;
	last_x = x;
	if ((a = as[i]) != TmpAttr) {
	    NewRendition (TmpAttr, a);
	    TmpAttr = a;
	}
	if ((f = fs[i]) != TmpCharset) {
	    NewCharset (TmpCharset, f);
	    TmpCharset = f;
	}
	putchar (s[i]);
	last_x++;
    }
}

RedisplayLine (os, oa, of, y, from, to) char *os, *oa, *of; {
    if (curr->insert)
	InsertMode (0);
    NewRendition (GlobalAttr, 0);
    TmpAttr = 0;
    NewCharset (GlobalCharset, ASCII);
    TmpCharset = ASCII;
    last_y = y;
    last_x = from;
    DisplayLine (os, oa, of, curr->image[y], curr->attr[y],
	curr->font[y], y, from, to);
    NewRendition (TmpAttr, GlobalAttr);
    NewCharset (TmpCharset, GlobalCharset);
    if (curr->insert)
	InsertMode (1);
}

static MakeBlankLine (p, n) register char *p; register n; {
    do *p++ = ' ';
    while (--n);
}

MakeStatus (msg, wp)
char *msg; struct win *wp;
{
    struct win *ocurr = curr;
    int odisplay = display;
    register char *s, *t;
    register max;

    curr = wp;
    max = AM ? curr->cols-1 : curr->cols;
    for (s = t = msg; *s && t - msg < max; ++s)
	if (*s >= ' ' && *s <= '~')
	    *t++ = *s;
    *t = '\0';
    display = 1;
    if (status) {
	if (time ((time_t *)0) - TimeDisplayed < 2)
	    sleep (1);
	RemoveStatus (wp);
    }
    if (t > msg) {
	status = 1;
	StatLen = t - msg;
	Goto (curr->y, curr->x, curr->rows-1, 0);
	SaveAttr (A_SO);
	printf ("%s", msg);
	RestoreAttr (A_SO);
	(void) fflush (stdout);
	(void) time (&TimeDisplayed);
    }
    curr = ocurr;
    display = odisplay;
}

int AddMsg (str)
char *str;
{
    struct Msg *NewMsg;

    if (!(NewMsg = (struct Msg *) malloc( sizeof( struct Msg))))
	return 0;
    bzero( (char *)NewMsg, sizeof(struct Msg));
    if (!(NewMsg->text = (char *)malloc( (unsigned) (strlen(str) + 1))))
	return 0;
    strcpy( NewMsg->text, str);
    NewMsg->date = time(0);
    NewMsg->next = (struct Msg *) NULL;
    if( FirstMsg) {
			FirstMsg->prev = NewMsg;
			NewMsg->next = FirstMsg;
    }
    FirstMsg = NewMsg;
    NextMsg = FirstMsg->next;
	return 1; /* success */
}

PrevMsg(wp)
struct win *wp;
{
    char buf[1024];
    static char *tstr();
    time_t now = time(0);

    if( NextMsg == (struct Msg *) NULL) {
	NextMsg = FirstMsg;
	now -= MSGEXPIRE;
	sprintf( buf, "No more messages since %s", tstr(now));
	MakeStatus (buf , wp);
	return;
    } else
	if( time(0) - NextMsg->date > MSGEXPIRE) {
	    /*
	     * clear old Messages
	     */
            if( NextMsg->prev != (struct Msg *) NULL )
		NextMsg->prev->next = (struct Msg *) NULL;
	    else
		FirstMsg = (struct Msg *) NULL;

	    while( NextMsg->next != (struct Msg *) NULL) {
		NextMsg = NextMsg->next;
		Free( NextMsg->prev);
            }
            Free( NextMsg);
	    NextMsg = FirstMsg;
	    now -= MSGEXPIRE;
	    sprintf( buf, "No more messages since %s", tstr(now));
	    MakeStatus (buf , wp);
	    return;
        }
	else {
	    now = NextMsg->date;
	    sprintf(buf, "%s : %s", tstr(now), NextMsg->text);
	    NextMsg = NextMsg->next;
	    MakeStatus (buf , wp);
	    return;
	}
}

char *tstr(t)
time_t t;
{
    struct tm now, then;
    extern struct tm *localtime();
    char buf[27];
    int ago;
    static char *days[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

    buf[0] = '\0';
    then = *localtime(&t);
    t = time(0);
    now = *localtime(&t);

    ago = now.tm_wday != then.tm_wday;
    if( ago)  sprintf( buf + strlen(buf), "%3s ", days[then.tm_wday]);
    sprintf( buf + strlen(buf), "%02d:%02d:%02d",
				then.tm_hour, then.tm_min, then.tm_sec);
    return buf;
}

RemoveStatus (p) struct win *p; {
    struct win *ocurr = curr;
    int odisplay = display;

    if (!status)
	return;
    status = 0;
    curr = p;
    display = 1;
    Goto (-1, -1, curr->rows-1, 0);
    RedisplayLine (null, null, null, curr->rows-1, 0, StatLen);
    Goto (curr->rows-1, last_x, curr->y, curr->x);
    curr = ocurr;
    display = odisplay;
}

/*
 * Change Size of a window
 */

ChangeWindowSize (wp, nrows, ncols)
struct win *wp;
int nrows, ncols;
{
    
    int i;
    int rows = wp->rows;
    int cols = wp->cols;

    if( (rows == nrows) && (cols == ncols)) return 0;

    resize( &(wp->image), bclear, rows, cols, nrows, ncols);
    resize( &(wp->attr),  bzero, rows, cols, nrows, ncols);
    resize( &(wp->font),  bzero, rows, cols, nrows, ncols);

    if( (wp->tabs = (char *)realloc (wp->tabs, (unsigned)(ncols+1))) == 0) goto nomem;
    bzero (wp->tabs, ncols);
    for (i = 8; i < ncols; i += 8)
	wp->tabs[i] = 1;

    if( wp->bot == rows - 1 || wp->bot >= nrows)
		wp->bot = nrows - 1;
    if( wp->top >= nrows)
		wp->top = nrows - 1;

    if( wp->x >= ncols) wp->x = ncols - 1;
    if( wp->y >= nrows) wp->y = nrows - 1;

    wp->rows = nrows;
    wp->cols = ncols;

#ifdef TIOCSWINSZ
    {
	struct	winsize	ws;
        ws.ws_col = ncols;
        ws.ws_row = nrows;
    
        ioctl( wp->ptyfd, TIOCSWINSZ, &ws);
    }
#endif

    return 0;
nomem:
    Msg (0, "Out of memory.");
    return -1;

}
    
static resize( sc, clear,rows, cols, nrows, ncols)
char ***sc;
void (*clear)();
int rows, cols, nrows, ncols;
{
    register char **cp;
/*    int mincols = ncols < cols ? ncols : cols; */
    int inccols = ncols - cols;
    int minrows = nrows < rows ? nrows : rows;

    cp = *sc+nrows;
    while( cp < *sc+rows)
    {	Free(*cp); /* oh boy! we get in difficulties with that macro */
		cp++;
	}

    if ((cp = *sc = (char **) realloc (*sc, nrows * sizeof (char *))) == 0)
	goto nomem;

    if( inccols != 0)
	while( cp < *sc + minrows) {
	    if ((*cp = (char *) realloc (*cp, (unsigned)ncols)) == (char *)0) goto nomem;
	    if( inccols > 0)
	        (*clear)( *cp+cols, inccols);
	    cp++;
        }

    if( nrows > rows) {
	for (cp = *sc+minrows; cp < *sc+nrows ; cp++) {
	    if ((*cp = (char *) malloc ((unsigned)ncols)) == (char *)0) goto nomem;
	    (*clear)(*cp, ncols);
	}
    }

    return 0;
nomem:
    Msg (0, "Out of memory.");
    return -1;
}

/*
 * This function tries everything to change the size of
 * the pysical display - it does not change any internal
 * structure.
 */

int ChangeTermSize (rows, cols)
int rows, cols;
{
    register int i;
    register int okay = 0;

    if(TermSizeLocked)
	return 0;

    if( NWokay)
        for ( i = 0; i <= 1; i++)
	    if( rows == TS[i].rows && cols == TS[i].cols) {
		if( lastsize != i)
	            PutStr (Z[i]);		/* set terminal width */
	        lastsize = i;
		okay++;
		break;
	    }
    if( WS) {
	PutStr (tgoto (WS, cols, rows));/* changed from cols,rows. jw.*/
	okay++;
    }
    if( okay) {
	trows = rows;
	tcols = cols;
    }
    return okay;
}

/*
 * what is the best Terminal Size for my window ?
 * needs thaught.
 */

BestTermSize( rows, cols)
int *rows, *cols;
{
/*
    register int i;
    register int fit[2];
*/

    if( WS) return;

    if( NWokay &&
	( (TS[0].rows <= *rows) > 0 || TS[0].cols <= *cols)) {
        *rows = TS[1].rows;
        *cols = TS[1].cols;
    }
    else {
        *rows = TS[0].rows;
        *cols = TS[0].cols;
    }
}
