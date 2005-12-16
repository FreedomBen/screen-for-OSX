/* Copyright (c) 1987-1990 Oliver Laumann and Wayne Davison.
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 *
 * Noteworthy contributors to ansi.c's design and implementation:
 *	Patrick Wolfe (pat@kai.com, kailand!pat)
 *	Nathan Glasser (nathan@brokaw.lcs.mit.edu)
 */

char AnsiVersion[] = "ansi 2.3-PreRelease7 19-Jun-90";

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/ioctl.h>
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

#define EXPENSIVE    1000

#define G0           0
#define G1           1
#define G2           2
#define G3           3

#define ASCII        0

#ifdef TOPSTAT
#define STATLINE     (0)
#else
#define STATLINE     (rows-1)
#endif

extern char *getenv(), *tgetstr(), *tgoto(), *malloc();

extern struct win *fore;
extern force_vt, assume_LP, help_page;
int rows, cols;
int maxwidth, minwidth, default_width;
int status;
int flowctl, wrap = 1;
char Term[16];
char *Termcap, *extra_incap, *extra_outcap;
char *blank, *null, *LastMsg;
char *Z0, *Z1;
int ISO2022, HS;
time_t TimeDisplayed, time();

static char *tbuf, *tentry;
static char *tp;
static char *TI, *TE, *BL, *VB, *BC, *CR, *NL, *CL, *IS, *CM;
static char *US, *UE, *SO, *SE, *CE, *CD, *DO, *SR, *SF, *AL;
static char *CS, *DL, *DC, *IC, *IM, *EI, *UP, *ND, *KS, *KE;
static char *MB, *MD, *MH, *MR, *ME, *PO, *PF, *HO;
static char *TS, *FS, *DS;
static char *CDC, *CDL, *CAL;
static AM, LP;
static screencap = 0;
static char GlobalAttr, GlobalCharset;
static char *OldImage, *OldAttr, *OldFont;
static last_x, last_y;
static struct win *curr;
static display = 1;
static StrCost;
static UPcost, DOcost, LEcost, NDcost, CRcost, IMcost, EIcost;
static tcLineLen = 100;
static StatLen;
static insert;
static keypad;
static flow;
static lastwidth = -1;
static lp_missing = 0;

#ifdef TIOCSWINSZ
    struct winsize Window_Size;
#endif

static char *KeyCaps[] = {
    "k0", "k1", "k2", "k3", "k4", "k5", "k6", "k7", "k8", "k9",
    "kb", "kd", "kh", "kl", "ko", "kr", "ku",
    "K1", "K2", "K3", "K4", "K5",
    0
};

static char TermcapConst[] = "\\\n\
\t:DO=\\E[%dB:LE=\\E[%dD:RI=\\E[%dC:UP=\\E[%dA:bs:bt=\\E[Z:\\\n\
\t:cd=\\E[J:ce=\\E[K:cl=\\E[H\\E[J:cm=\\E[%i%d;%dH:ct=\\E[3g:\\\n\
\t:do=^J:nd=\\E[C:pt:rc=\\E8:rs=\\Ec:sc=\\E7:st=\\EH:up=\\EM:\\\n\
\t:le=^H:bl=^G:cr=^M:it#8:ho=\\E[H:nw=\\EE:ta=^I:is=\\E)0:";

InitTermcap() {
    register char *s;
    int extra_len = (extra_incap ? strlen(extra_incap) : 0);

    if ((s = getenv("SCREENCAP")) != 0) {
	if ((Termcap = malloc((unsigned)strlen(s)+10)) != 0) {
	    sprintf(Termcap, "TERMCAP=%s", s);
	    screencap = 1;
	}
    } else
	Termcap = malloc((unsigned)1024);
    tbuf = malloc((unsigned)1024 + extra_len);
    tentry = tp = malloc((unsigned)1024);
    if (!(Termcap && tbuf && tentry))
	Msg(0, "Out of memory.");
    if ((s = getenv("TERM")) == 0)
	Msg(0, "No TERM in environment.");
    if (tgetent(tbuf, s) != 1)
	Msg(0, "Cannot find termcap entry for %s.", s);
    assume_LP = assume_LP || (!extra_incap && !strncmp(s,"vt",2));
    if (extra_len) {
	int tlen = strlen(tbuf);

	if (!(s = index(tbuf, ':'))) {
	    strcat(tbuf, ":");
	    s = ++tbuf + tlen;
	} else
	    s++;
	bcopy(s, s+extra_len, tlen - (s-tbuf));
	bcopy(extra_incap, s, extra_len);
    }
    cols = tgetnum("co");
    rows = tgetnum("li");
    if (cols <= 0)
	cols = 80;
    if (rows <= 0)
	rows = 24;
    default_width = cols;
    /* Termcap fields Z0 & Z1 contain width-changing sequences. */
    if ((Z0 = tgetstr("Z0", &tp)) != NULL
     && (Z1 = tgetstr("Z1", &tp)) == NULL)
	Z0 = NULL;
    if (Z0) {
	/* ass_u_me some things about the other width */
	if (default_width >= 132) {
	    minwidth = 80;
	    maxwidth = default_width;
	} else {
	    minwidth = default_width;
	    maxwidth = 132;
	}
    } else
	minwidth = maxwidth = cols;
    if (tgetflag("hc"))
	Msg(0, "You can't run screen on a hardcopy terminal.");
    if (tgetflag("os"))
	Msg(0, "You can't run screen on a terminal that overstrikes.");
    if (tgetflag("ns"))
	Msg(0, "Terminal must support scrolling.");
    if (!(CL = tgetstr("cl", &tp)))
	Msg(0, "Clear screen capability required.");
    if (!(CM = tgetstr("cm", &tp)))
	Msg(0, "Addressable cursor capability required.");
    switch (flowctl) {
    case 0:
	flow = !tgetflag("NF");
	flowctl = flow+1;
	break;
    case 1:
	flow = 0;
	break;
    case 2:
    case 3:
	flow = 1;
	break;
    }
    AM = tgetflag("am");
    LP = assume_LP || !AM || tgetflag("LP");
    if (LP || tgetflag("OP"))
	force_vt = 0;
    HO = tgetstr("ho", &tp);
    TI = tgetstr("ti", &tp);
    TE = tgetstr("te", &tp);
    if (!(BL = tgetstr("bl", &tp)))
	BL = "\007";
    VB = tgetstr("vb", &tp);
    if (!(BC = tgetstr("bc", &tp))) {
	if (tgetflag("bs"))
	    BC = "\b";
	else
	    BC = tgetstr("le", &tp);
    }
    if (!(CR = tgetstr("cr", &tp)))
	CR = "\r";
    if (!(NL = tgetstr("nl", &tp)))
	NL = "\n";
    IS = tgetstr("is", &tp);
    if (tgetnum("sg") <= 0 && tgetnum("ug") <= 0) {
	US = tgetstr("us", &tp);
	UE = tgetstr("ue", &tp);
	SO = tgetstr("so", &tp);
	SE = tgetstr("se", &tp);
	MB = tgetstr("mb", &tp);
	MD = tgetstr("md", &tp);
	MH = tgetstr("mh", &tp);
	MR = tgetstr("mr", &tp);
	ME = tgetstr("me", &tp);
	if (!MR && SO)
	    MR = SO;
	/*
	 * Does ME also reverse the effect of SO and/or US?  This is not
	 * clearly specified by the termcap manual.
	 * Anyway, we should at least look whether ME and SE/UE are equal:
	 */
	if (UE && (SE && strcmp(SE, UE) == 0) || (ME && strcmp(ME, UE) == 0))
	    UE = 0;
	if (ME && (SE && strcmp(SE, ME) == 0))
	    ME = 0;
    }
    if ((HS = tgetflag("hs")) != 0) {
	TS = tgetstr("ts", &tp);
	FS = tgetstr("fs", &tp);
	DS = tgetstr("ds", &tp);
	if ((HS = tgetnum("ws")) <= 0)
	    HS = cols;
	if (!TS || !FS || !DS)
	    HS = 0;
    }
    CE = tgetstr("ce", &tp);
    CD = tgetstr("cd", &tp);
    if (!(DO = tgetstr("do", &tp)))
	DO = NL;
    UP = tgetstr("up", &tp);
    ND = tgetstr("nd", &tp);
    SR = tgetstr("sr", &tp);
    if (!(SF = tgetstr("sf", &tp)))
	SF = NL;
    AL = tgetstr("al", &tp);
    DL = tgetstr("dl", &tp);
    CS = tgetstr("cs", &tp);
    DC = tgetstr("dc", &tp);
    IC = tgetstr("ic", &tp);
    CDC = tgetstr("DC", &tp);
    CDL = tgetstr("DL", &tp);
    CAL = tgetstr("AL", &tp);
    IM = tgetstr("im", &tp);
    EI = tgetstr("ei", &tp);
    if (tgetflag("in"))
	IC = IM = 0;
    if (IC && IC[0] == '\0')
	IC = 0;
    if (IM && IM[0] == '\0')
	IM = 0;
    if (EI && EI[0] == '\0')
	EI = 0;
    KS = tgetstr("ks", &tp);
    KE = tgetstr("ke", &tp);
    ISO2022 = tgetflag("G0");
    PO = tgetstr("po", &tp);
    if (!(PF = tgetstr("pf", &tp)))
	PO = 0;
    blank = malloc((unsigned)maxwidth);
    null = malloc((unsigned)maxwidth);
    OldImage = malloc((unsigned)maxwidth);
    OldAttr = malloc((unsigned)maxwidth);
    OldFont = malloc((unsigned)maxwidth);
    LastMsg = malloc((unsigned)maxwidth+1);
    if (!(blank && null && OldImage && OldAttr && OldFont && LastMsg))
	Msg(0, "Out of memory.");
    MakeBlankLine(blank, maxwidth);
    bzero(null, maxwidth);
    *LastMsg = '\0';
    UPcost = CalcCost(UP);
    DOcost = CalcCost(DO);
    LEcost = CalcCost(BC);
    NDcost = CalcCost(ND);
    CRcost = CalcCost(CR);
    IMcost = CalcCost(IM);
    EIcost = CalcCost(EI);
}

InitTerm() {
    display = 1;
    PutStr(IS);
    PutStr(TI);
#ifdef TIOCSWINSZ
    ioctl(0, TIOCGWINSZ, &Window_Size);
    Window_Size.ws_row = rows;
#endif
    ChangeWidth(0, default_width);
    PutStr(CL);
}

FinitTerm() {
    display = 1;
    flow = 1;
    InsertMode(0);
    KeypadMode(0);
    ChangeWidth(0, default_width);
    SaveSetAttr(0, ASCII);
    Goto(-1, -1, rows-1, 0);
    PutStr(TE);
}

static AddCap(s) char *s; {
    register n;

    if (tcLineLen + (n = strlen(s)) > 55) {
	strcat(Termcap, "\\\n\t:");
	tcLineLen = 0;
    }
    strcat(Termcap, s);
    tcLineLen += n;
}

char *MakeTermcap(aflag) {
    char buf[1024];
    register char **pp, *p, *cp, ch;

    cp = (cols >= 132 ? "-w" : "");
    sprintf(Term, "TERM=screen%s", cp);
    if (screencap)
	return Termcap;
    sprintf(Termcap,
	"TERMCAP=SC|screen%s|VT 100/ANSI X3.64 virtual terminal|", cp);
    if (extra_outcap && *extra_outcap) {
	for( cp = extra_outcap; p = index(cp,':'); cp = p ) {
	    ch = *++p;
	    *p = '\0';
	    AddCap(cp);
	    *p = ch;
	}
	tcLineLen = 100;
    }
    strcat(Termcap, TermcapConst);
    sprintf(buf, "li#%d:co#%d:ll=\\E[%dH:", rows, cols, rows);
    AddCap(buf);
    if (AM && !LP && !force_vt)
	AddCap("am:");
    if (VB)
	AddCap("vb=\\E[?5h\\E[?5l:");
    if (US) {
	AddCap("us=\\E[4m:");
	AddCap("ue=\\E[24m:");
    }
    if (SO) {
	AddCap("so=\\E[3m:");
	AddCap("se=\\E[23m:");
    }
    if (MB)
	AddCap("mb=\\E[5m:");
    if (MD)
	AddCap("md=\\E[1m:");
    if (MH)
	AddCap("mh=\\E[2m:");
    if (MR)
	AddCap("mr=\\E[7m:");
    if (MB || MD || MH || MR)
	AddCap("me=\\E[m:ms:");
    if ((CS && SR) || AL || CAL || aflag) {
	AddCap("sr=\\EM:");
	AddCap("al=\\E[L:");
	AddCap("AL=\\E[%dL:");
    }
    else if (SR)
	AddCap("sr=\\EM:");
    if (CS || DL || CDL || aflag) {
	AddCap("dl=\\E[M:");
	AddCap("DL=\\E[%dM:");
    }
    if (CS)
	AddCap("cs=\\E[%i%d;%dr:");
    if (DC || CDC || aflag) {
	AddCap("dc=\\E[P:");
	AddCap("DC=\\E[%dP:");
    }
    if (IC || IM || aflag) {
	AddCap("im=\\E[4h:");
	AddCap("ei=\\E[4l:");
	AddCap("mi:");
    }
    if (KS)
	AddCap("ks=\\E=:");
    if (KE)
	AddCap("ke=\\E>:");
    if (ISO2022)
	AddCap("G0:");
    if (PO) {
	AddCap("po=\\E[5i:");
	AddCap("pf=\\E[4i:");
    }
    for (pp = KeyCaps; *pp; ++pp)
	if (p = tgetstr(*pp, &tp)) {
	    MakeString(*pp, buf, p);
	    AddCap(buf);
	}
    return Termcap;
}

static MakeString(cap, buf, s) char *cap, *buf; register char *s; {
    register char *p = buf;
    register unsigned c;

    *p++ = *cap++; *p++ = *cap; *p++ = '=';
    while (c = *s++) {
	switch (c) {
	case '\033':
	    *p++ = '\\'; *p++ = 'E'; break;
	case ':':
	    sprintf(p, "\\072"); p += 4; break;
	case '^': case '\\':
	    *p++ = '\\'; *p++ = c; break;
	default:
	    if (c >= 200) {
		sprintf(p, "\\%03o", c & 0377); p += 4;
	    } else if (c < ' ') {
		*p++ = '^'; *p++ = c + '@';
	    } else *p++ = c;
	}
    }
    *p++ = ':'; *p = '\0';
}

Activate() {
    RemoveStatus();
    curr = fore;
    display = 1;
    cols = curr->width;
    ChangeWidth(curr->ptyfd, cols);
    if (CS)
	PutStr(tgoto(CS, curr->bot, curr->top));
    InsertMode(curr->insert);
    KeypadMode(curr->keypad);
    FlowMode(curr->flow);
    if (curr->monitor != MON_OFF)
	curr->monitor = MON_ON;
    curr->bell = BELL_OFF;
    Redisplay();
}

ResetScreen(p) register struct win *p; {
    register i;

    p->wrap = wrap;
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
    p->ss = 0;
    p->LocalCharset = G0;
    bzero(p->tabs, maxwidth);
    for (i = 8; i < maxwidth; i += 8)
	p->tabs[i] = 1;
    for (i = G0; i <= G3; i++)
	p->charsets[i] = ASCII;
}

WriteString(wp, buf, len) struct win *wp; register char *buf; {
    register c, intermediate = 0;

    if (!len)
	return;
    if (wp->logfp != NULL)
	if (fwrite(buf,len,1,wp->logfp) < 1) {
	    extern int errno;

	    Msg(errno, "Error writing logfile");
	    fclose(wp->logfp);
	    wp->logfp = NULL;
	}
    curr = wp;
    display = curr->active;
    cols = curr->width;
    if (display) {
	if (!HS)
	    RemoveStatus();
    } else if (curr->monitor == MON_ON)
	curr->monitor = MON_FOUND;

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
		PrintChar(c);
	    }
	    break;
	case PRINESC:
	    switch (c) {
	    case '[':
		curr->state = PRINCSI; break;
	    default:
		PrintChar('\033'); PrintChar(c);
		curr->state = PRIN;
	    }
	    break;
	case PRINCSI:
	    switch (c) {
	    case '4':
		curr->state = PRIN4; break;
	    default:
		PrintChar('\033'); PrintChar('['); PrintChar(c);
		curr->state = PRIN;
	    }
	    break;
	case PRIN4:
	    switch (c) {
	    case 'i':
		curr->state = LIT;
		PrintFlush();
		break;
	    default:
		PrintChar('\033'); PrintChar('['); PrintChar('4');
		PrintChar(c);
		curr->state = PRIN;
	    }
	    break;
	case TERM:
	    switch (c) {
	    case '\\':
		curr->state = LIT;
		*(curr->stringp) = '\0';
		switch (curr->StringType) {
		case PM:
		    if (!display)
			break;
		    MakeStatus(curr->string);
		    if (!HS && status && len > 1) {
			curr->outlen = len-1;
			bcopy(buf, curr->outbuf, curr->outlen);
			return;
		    }
		    break;
		case DCS:
		    if (!display)
			break;
		    printf("%s", curr->string);
		    break;
		case AKA:
		    strncpy(curr->cmd+curr->akapos, curr->string, 20);
		    if (!*curr->string)
			curr->autoaka = curr->y+1;
		    break;
		}
		break;
	    default:
		curr->state = STR;
		AddChar('\033');
		AddChar(c);
	    }
	    break;
	case STR:
	    switch (c) {
	    case '\0':
		break;
	    case '\033':
		curr->state = TERM; break;
	    default:
		AddChar(c);
	    }
	    break;
	case ESC:
	    switch (c) {
	    case '[':
		curr->NumArgs = 0;
		intermediate = 0;
		bzero((char *)curr->args, MAXARGS * sizeof (int));
		curr->state = CSI;
		break;
	    case ']':
		StartString(OSC); break;
	    case '_':
		StartString(APC); break;
	    case 'P':
		StartString(DCS); break;
	    case '^':
		StartString(PM); break;
	    case '"':
	    case 'k':
		StartString(AKA); break;
	    default:
		if (Special(c))
		    break;
		if (c >= ' ' && c <= '/')
		    intermediate = intermediate ? -1 : c;
		else if (c >= '0' && c <= '~') {
		    DoESC(c, intermediate);
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
		}
		break;
	    case ';': case ':':
		curr->NumArgs++; break;
	    default:
		if (Special(c))
		    break;
		if (c >= '@' && c <= '~') {
		    curr->NumArgs++;
		    DoCSI(c, intermediate);
		    if (curr->state != PRIN)
			curr->state = LIT;
		} else if ((c >= ' ' && c <= '/') || (c >= '<' && c <= '?'))
		    intermediate = intermediate ? -1 : c;
		else {
		    curr->state = LIT;
		    goto NextChar;
		}
	    }
	    break;
	case LIT:
	default:
	    if (!Special(c)) {
		if (c == '\033') {
		    intermediate = 0;
		    curr->state = ESC;
		    if (display && lp_missing && (IC || IM)) {
			Goto(curr->y, curr->x, curr->bot, cols-2);
			RedisplayLine(blank,null,null,curr->bot,cols-2,cols-1);
			Goto(last_y, last_x, curr->y, curr->x);
		    }
		    if (curr->autoaka < 0)
			curr->autoaka = 0;
		} else if (c < ' ')
		    break;
		else {
		    if (curr->ss)
			NewCharset(curr->charsets[curr->ss]);
		    if (curr->x < cols-1) {
			if (curr->insert)
			    InsertAChar(c);
			else {
			    if (display)
				putchar(c);
			    SetChar(c);
			}
			curr->x++;
		    } else if (curr->x == cols-1) {
			if (curr->wrap && !force_vt) {
			    if (display)
				putchar(c);
			    SetChar(c);
			    if (AM && !LP) {
				curr->x = 0;	/* terminal auto-wrapped */
				LineFeed(0);
			    } else
				curr->x++;
			} else {
			    if (display) {
				if (LP || curr->y != curr->bot) {
				    putchar(c);
				    if (AM && !LP)
					Goto(-1, -1, curr->y, curr->x);
				} else
				    CheckLP(c);
			    }
			    SetChar(c);
			    if (curr->wrap)
				curr->x++;
			}
		    } else {
			LineFeed(2);		/* cr+lf, handle LP */
			if (curr->insert)
			    InsertAChar(c);
			else {
			    if (display)
				putchar(c);
			    SetChar(c);
			}
			curr->x = 1;
		    }
		    if (curr->ss) {
			NewCharset(curr->charsets[curr->LocalCharset]);
			curr->ss = 0;
		    }
		}
	    }
	}
    } while (--len);
    curr->outlen = 0;
    if (curr->state == PRIN)
	PrintFlush();
}

static Special(c) register c; {
    switch (c) {
    case '\b':
	BackSpace(); return 1;
    case '\r':
	Return(); return 1;
    case '\n':
	if (curr->autoaka)
	    FindAKA();
	LineFeed(1);
	return 1;
    case '\007':
	PutStr(BL);
	if (!display)
	    curr->bell = BELL_ON;
	return 1;
    case '\t':
	ForwardTab(); return 1;
    case '\017':   /* SI */
	MapCharset(G0); return 1;
    case '\016':   /* SO */
	MapCharset(G1); return 1;
    }
    return 0;
}

static DoESC(c, intermediate) {
    switch (intermediate) {
    case 0:
	switch (c) {
	case 'E':
	    LineFeed(2);
	    break;
	case 'D':
	    LineFeed(1); 
	    break;
	case 'M':
	    ReverseLineFeed();
	    break;
	case 'H':
	    curr->tabs[curr->x] = 1;
	    break;
	case '7':
	    SaveCursor();
	    break;
	case '8':
	    RestoreCursor();
	    break;
	case 'c':
	    ClearScreen();
	    ResetScreen(curr);
	    NewRendition(0);
	    NewCharset(ASCII);
	    InsertMode(0);
	    KeypadMode(0);
	    if (CS)
		PutStr(tgoto(CS, rows-1, 0));
	    break;
	case '=':
	    KeypadMode(1);
	    curr->keypad = 1;
	    if (curr->autoflow)
		FlowMode(curr->flow = 0);
	    break;
	case '>':
	    KeypadMode(0);
	    curr->keypad = 0;
	    if (curr->autoflow)
		FlowMode(curr->flow = 1);
	    break;
	case 'n':   /* LS2 */
	    MapCharset(G2); break;
	case 'o':   /* LS3 */
	    MapCharset(G3); break;
	case 'N':   /* SS2 */
	    if (curr->charsets[curr->LocalCharset] != curr->charsets[G2])
		curr->ss = G2;
	    else
		curr->ss = 0;
	    break;
	case 'O':   /* SS3 */
	    if (curr->charsets[curr->LocalCharset] != curr->charsets[G3])
		curr->ss = G3;
	    else
		curr->ss = 0;
	    break;
	}
	break;
    case '#':
	switch (c) {
	case '8':
	    FillWithEs();
	    break;
	}
	break;
    case '(':
	DesignateCharset(c, G0); break;
    case ')':
	DesignateCharset(c, G1); break;
    case '*':
	DesignateCharset(c, G2); break;
    case '+':
	DesignateCharset(c, G3); break;
    }
}

static DoCSI(c, intermediate) {
    register i, a1 = curr->args[0], a2 = curr->args[1];

    if (curr->NumArgs > MAXARGS)
	curr->NumArgs = MAXARGS;
    switch (intermediate) {
    case 0:
	switch (c) {
	case 'H': case 'f':
	    if (a1 < 1)
		a1 = 1;
	    if (curr->origin)
		a1 += curr->top;
	    if (a1 > rows)
		a1 = rows;
	    if (a2 < 1)
		a2 = 1;
	    if (a2 > cols)
		a2 = cols;
	    Goto(curr->y, curr->x, --a1, --a2);
	    curr->y = a1;
	    curr->x = a2;
	    if (curr->autoaka)
		curr->autoaka = a1+1;
	    break;
	case 'J':
	    if (a1 < 0 || a1 > 2)
		a1 = 0;
	    switch (a1) {
	    case 0:
		ClearToEOS(); break;
	    case 1:
		ClearFromBOS(); break;
	    case 2:
		ClearScreen();
		Goto(0, 0, curr->y, curr->x);
		break;
	    }
	    break;
	case 'K':
	    if (a1 < 0 || a1 > 2)
		a1 %= 3;
	    switch (a1) {
	    case 0:
		ClearToEOL(); break;
	    case 1:
		ClearFromBOL(); break;
	    case 2:
		ClearLine(); break;
	    }
	    break;
	case 'A':
	    CursorUp(a1 ? a1 : 1);
	    break;
	case 'B':
	    CursorDown(a1 ? a1 : 1);
	    break;
	case 'C':
	    CursorRight(a1 ? a1 : 1);
	    break;
	case 'D':
	    CursorLeft(a1 ? a1 : 1);
	    break;
	case 'm':
	    SelectRendition();
	    break;
	case 'g':
	    if (a1 == 0)
		curr->tabs[curr->x] = 0;
	    else if (a1 == 3)
		bzero(curr->tabs, cols);
	    break;
	case 'r':
	    if (!CS)
		break;
	    if (!a1) a1 = 1;
	    if (!a2) a2 = rows;
	    if (a1 < 1 || a2 > rows || a1 >= a2)
		break;
	    curr->top = a1-1;
	    curr->bot = a2-1;
	    PutStr(tgoto(CS, curr->bot, curr->top));
	    if (curr->origin) {
		Goto(-1, -1, curr->top, 0);
		curr->y = curr->top;
		curr->x = 0;
	    } else {
		Goto(-1, -1, 0, 0);
		curr->y = curr->x = 0;
	    }
	    break;
	case 's':
	    SaveCursor();
	    break;
	case 'u':
	    RestoreCursor();
	    break;
	case 'I':
	    if (!a1) a1 = 1;
	    while (a1--)
		ForwardTab();
	    break;
	case 'Z':
	    if (!a1) a1 = 1;
	    while (a1--)
		BackwardTab();
	    break;
	case 'L':
	    InsertLine(a1 ? a1 : 1);
	    break;
	case 'M':
	    DeleteLine(a1 ? a1 : 1);
	    break;
	case 'P':
	    DeleteChar(a1 ? a1 : 1);
	    break;
	case '@':
	    InsertChar(a1 ? a1 : 1);
	    break;
	case 'h':
	    SetMode(1);
	    break;
	case 'l':
	    SetMode(0);
	    break;
	case 'i':
	    if (PO && a1 == 5) {
		curr->stringp = curr->string;
		curr->state = PRIN;
	    }
	    break;
	}
	break;
    case '?':
	if (c != 'h' && c != 'l')
	    break;
	i = (c == 'h');
	if (a1 == 3) {
	    if (Z0 && curr->width != (i = (i ? maxwidth : minwidth))) {
		curr->width = i;
		if (display)
		    Activate();
	    }
	} else if (a1 == 5) {
	    if (i)
		curr->vbwait = 1;
	    else {
		if (curr->vbwait)
		    PutStr(VB);
		curr->vbwait = 0;
	    }
	} else if (a1 == 6) {
	    if ((curr->origin = i) != 0) {
		Goto(curr->y, curr->x, curr->top, 0);
		curr->y = curr->top;
		curr->x = 0;
	    } else {
		Goto(curr->y, curr->x, 0, 0);
		curr->y = curr->x = 0;
	    }
	} else if (a1 == 7)
	    curr->wrap = i;
	break;
    }
}

static PutChar(c) {
    putchar(c);
}

static PutStr(s) char *s; {
    if (display && s)
	tputs(s, 1, PutChar);
}

static CPutStr(s, c) char *s; {
    if (display && s)
	tputs(tgoto(s, 0, c), 1, PutChar);
}

static SetChar(c) register c; {
    register struct win *p = curr;

    p->image[p->y][p->x] = c;
    p->attr[p->y][p->x] = p->LocalAttr;
    p->font[p->y][p->x] = p->charsets[p->ss ? p->ss : p->LocalCharset];
}

static StartString(type) enum string_t type; {
    curr->StringType = type;
    curr->stringp = curr->string;
    curr->state = STR;
}

static AddChar(c) {
    if (curr->stringp >= curr->string+MAXSTR-1)
	curr->state = LIT;
    else
	*(curr->stringp)++ = c;
}

static PrintChar(c) {
    if (curr->stringp >= curr->string+MAXSTR-1)
	PrintFlush();
    *(curr->stringp)++ = c;
}

static PrintFlush() {
    if (curr->stringp > curr->string) {
	tputs(PO, 1, PutChar);
	(void) fflush(stdout);
	(void) write(1, curr->string, curr->stringp - curr->string);
	tputs(PF, 1, PutChar);
	(void) fflush(stdout);
	curr->stringp = curr->string;
    }
}

/* Insert mode is a toggle on some terminals, so we need this hack:
 */
static InsertMode(on) {
    if (display && on != insert && IM) {
	insert = on;
	if (insert)
	    PutStr(IM);
	else
	    PutStr(EI);
    }
}

/* ...and maybe keypad application mode is a toggle, too:
 */
static KeypadMode(on) {
    if (display && keypad != on) {
	keypad = on;
	if (keypad)
	    PutStr(KS);
	else
	    PutStr(KE);
    }
}

static FlowMode(on) {
    if (display && flow != on) {
	flow = on;
	SetFlow(on);
    }
}

ToggleFlow() {
    flow = fore->flow = !fore->flow;
    SetFlow(flow);
}

static DesignateCharset(c, n) {
    curr->ss = 0;
    if (c == 'B')
	c = ASCII;
    if (curr->charsets[n] != c) {
	curr->charsets[n] = c;
	if (curr->LocalCharset == n)
	    NewCharset(c);
    }
}

static MapCharset(n) {
    curr->ss = 0;
    if (curr->LocalCharset != n) {
	curr->LocalCharset = n;
	NewCharset(curr->charsets[n]);
    }
}

static NewCharset(new) {
    char buf[8];

    if (!display || GlobalCharset == new)
	return;
    GlobalCharset = new;
    if (ISO2022) {
	sprintf(buf, "\033(%c", new == ASCII ? 'B' : new);
	PutStr(buf);
    }
}

static SaveCursor() {
    curr->saved = 1;
    curr->Saved_x = curr->x;
    curr->Saved_y = curr->y;
    curr->SavedLocalAttr = curr->LocalAttr;
    curr->SavedLocalCharset = curr->LocalCharset;
    bcopy((char *)curr->charsets, (char *)curr->SavedCharsets,
	4 * sizeof (int));
}

static RestoreCursor() {
    if (curr->saved) {
	curr->LocalAttr = curr->SavedLocalAttr;
	NewRendition(curr->LocalAttr);
	bcopy((char *)curr->SavedCharsets, (char *)curr->charsets,
	    4 * sizeof (int));
	curr->LocalCharset = curr->SavedLocalCharset;
	NewCharset(curr->charsets[curr->LocalCharset]);
	Goto(curr->y, curr->x, curr->Saved_y, curr->Saved_x);
	curr->x = curr->Saved_x;
	curr->y = curr->Saved_y;
    }
}

/*ARGSUSED*/
static CountChars(c) {
    StrCost++;
}

static CalcCost(s) register char *s; {
    if (s) {
	StrCost = 0;
	tputs(s, 1, CountChars);
	return StrCost;
    } else
	return EXPENSIVE;
}

static Goto(y1, x1, y2, x2) {
    register dy, dx;
    register cost = 0;
    register char *s;
    int CMcost, n, m;
    enum move_t xm = M_NONE, ym = M_NONE;

    if (!display)
	return;
    if (x1 == cols)
	if (LP)
	    x1 = -1;
	else
	    x1--;
    if (x2 == cols)
	x2--;
    dx = x2 - x1;
    dy = y2 - y1;
    if (dy == 0 && dx == 0)
	return;
    if (HO && !x2 && !y2)
	s = HO;
    else
	s = tgoto(CM, x2, y2);
    if (y1 == -1 || x1 == -1
     || (y2 > curr->bot && y1 <= curr->bot)
     || (y2 < curr->top && y1 >= curr->top)) {
DoCM:
	PutStr(s);
	return;
    }
    CMcost = CalcCost(s);
    if (dx > 0) {
	if ((n = RewriteCost(y1, x1, x2)) < (m = dx * NDcost)) {
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
    if (dx && (n = RewriteCost(y1, 0, x2) + CRcost) < cost) {
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
		PutStr(s);
	} else {
	    if (xm == M_CR) {
		PutStr(CR);
		x1 = 0;
	    }
	    if (x1 < x2) {
		InsertMode(0);
		for (s = curr->image[y1]+x1; x1 < x2; x1++, s++)
		    putchar(*s);
		InsertMode(curr->insert);
	    }
	}
    }
    if (ym != M_NONE) {
	if (ym == M_UP) {
	    s = UP; dy = -dy;
	} else s = DO;
	while (dy-- > 0)
	    PutStr(s);
    }
}

static RewriteCost(y, x1, x2) {
    register cost, dx;
    register char *p = curr->attr[y]+x1, *f = curr->font[y]+x1;

    cost = dx = x2 - x1;
    if (dx == 0)
	return 0;
    if (insert)
	cost += EIcost + IMcost;
    do {
	if (*p++ != GlobalAttr || *f++ != GlobalCharset)
	    return EXPENSIVE;
    } while (--dx);
    return cost;
}

static BackSpace() {
    if (curr->x > 0) {
	if (curr->x < cols) {
	    if (BC)
		PutStr(BC);
	    else
		Goto(curr->y, curr->x, curr->y, curr->x-1);
	} else if (AM && LP)
	    Goto(-1, -1, curr->y, curr->x-1);
	curr->x--;
    } else if (curr->wrap && curr->y > curr->top) {
	curr->x = cols-1;
	curr->y--;
	Goto(curr->y+1, 0, curr->y, curr->x);
    }
}

static Return() {
    if (curr->x > 0) {
	curr->x = 0;
	PutStr(CR);
    }
}

static LineFeed(out_mode) {   /* out_mode: 0=no-output lf, 1=lf, 2=cr+lf */
    if (out_mode && display) {
	if (lp_missing && curr->y == curr->bot) {
	    register x = curr->x;
	    if (out_mode == 2)
		curr->x = 0;
	    FixLP(curr->y, x, curr->y, cols-1);   /* scrolls the screen */
	} else {
	    if (out_mode == 2)
		Return();
	    PutStr(NL);
	}
    } else if (out_mode == 2)
	Return();
    if (curr->y == curr->bot) {
	ScrollUpMap(1);
	if (curr->autoaka > 1)
	    curr->autoaka--;
    }
    else if (curr->y < rows-1)
	curr->y++;
}

static ReverseLineFeed() {
    if (curr->y == curr->top) {
	ScrollDownMap(1);
	if (!display)
	    return;
	if (SR) {
	    PutStr(SR);
	    lp_missing = 0;
	} else if (AL) {
	    Goto(curr->top, curr->x, curr->top, 0);
	    PutStr(AL);
	    Goto(curr->top, 0, curr->top, curr->x);
	    lp_missing = 0;
	} else
	    Redisplay();
    } else if (curr->y > 0)
	CursorUp(1);
}

static InsertAChar(c) {
    register y = curr->y, x = curr->x;

    if (x == cols) x--;
    bcopy(curr->image[y], OldImage, cols);
    bcopy(curr->attr[y], OldAttr, cols);
    bcopy(curr->font[y], OldFont, cols);
    bcopy(curr->image[y]+x, curr->image[y]+x+1, cols-x-1);
    bcopy(curr->attr[y]+x, curr->attr[y]+x+1, cols-x-1);
    bcopy(curr->font[y]+x, curr->font[y]+x+1, cols-x-1);
    SetChar(c);
    if (!display)
	return;
    if (IC || IM) {
	InsertMode(1);
	PutStr(IC);
	putchar(c);
	InsertMode(curr->insert);
	if (y == curr->bot)
	    lp_missing = 0;
    } else {
	RedisplayLine(OldImage, OldAttr, OldFont, y, x, cols-1);
	Goto(y, last_x, y, ++x);
    }
}

static InsertChar(n) {
    register i, y = curr->y, x = curr->x;

    if (x == cols) --x;
    bcopy(curr->image[y], OldImage, cols);
    bcopy(curr->attr[y], OldAttr, cols);
    bcopy(curr->font[y], OldFont, cols);
    if (n > cols-x)
	n = cols-x;
    bcopy(curr->image[y]+x, curr->image[y]+x+n, cols-x-n);
    bcopy(curr->attr[y]+x, curr->attr[y]+x+n, cols-x-n);
    bcopy(curr->font[y]+x, curr->font[y]+x+n, cols-x-n);
    ClearInLine(0, y, x, x+n-1);
    if (!display)
	return;
    if (IC || IM) {
	InsertMode(1);
	for (i = n; i; i--) {
	    PutStr(IC);
	    putchar(' ');
	}
	InsertMode(curr->insert);
	Goto(y, x+n, y, x);
	if (y == curr->bot)
	    lp_missing = 0;
    } else {
	RedisplayLine(OldImage, OldAttr, OldFont, y, x, cols-1);
	Goto(y, last_x, y, x);
    }
}

static DeleteChar(n) {
    register i, y = curr->y, x = curr->x;

    if (x == cols) --x;
    bcopy(curr->image[y], OldImage, cols);
    bcopy(curr->attr[y], OldAttr, cols);
    bcopy(curr->font[y], OldFont, cols);
    if (n > cols-x)
	n = cols-x;
    bcopy(curr->image[y]+x+n, curr->image[y]+x, cols-x-n);
    bcopy(curr->attr[y]+x+n, curr->attr[y]+x, cols-x-n);
    bcopy(curr->font[y]+x+n, curr->font[y]+x, cols-x-n);
    ClearInLine(0, y, cols-n, cols-1);
    if (!display)
	return;
    if (CDC && !(n == 1 && DC)) {
	CPutStr(CDC, n);
	if (lp_missing && y == curr->bot)
	    FixLP(y, x, y, cols-1 - n);
    } else if (DC) {
	for (i = n; i; i--)
	    PutStr(DC);
	if (lp_missing && y == curr->bot)
	    FixLP(y, x, y, cols-1 - n);
    } else {
	RedisplayLine(OldImage, OldAttr, OldFont, y, x, cols-1);
	Goto(y, last_x, y, x);
    }
}

static DeleteLine(n) {
    register i, old = curr->top;

    if (n > curr->bot-curr->y+1)
	n = curr->bot-curr->y+1;
    curr->top = curr->y;
    ScrollUpMap(n);
    if (!display) {
	curr->top = old;
	return;
    }
    if (DL || CDL) {
	Goto(curr->y, curr->x, curr->y, 0);
	if (CDL && !(n == 1 && DL))
	    CPutStr(CDL, n);
	else {
	    for (i = n; i; i--)
		PutStr(DL);
	}
	if (lp_missing) {
	    if (curr->y+n-1 == curr->bot)
		lp_missing = 0;
	    else
		FixLP(curr->y, 0, curr->bot - n, cols-1);
	} else
	    Goto(curr->y, 0, curr->y, curr->x);
    } else if (CS) {
	PutStr(tgoto(CS, curr->bot, curr->top));
	Goto(-1, -1, curr->bot, 0);
	for (i = n; i; i--)
	    PutStr(SF);
	PutStr(tgoto(CS, curr->bot, old));
	if (lp_missing) {
	    if (curr->y+n-1 == curr->bot)
		lp_missing = 0;
	    else
		FixLP(-1, -1, curr->bot - n, cols-1);
	} else
	    Goto(-1, -1, curr->y, curr->x);
    } else
	Redisplay();
    curr->top = old;
}

static InsertLine(n) {
    register i, old = curr->top;

    if (n > curr->bot-curr->y+1)
	n = curr->bot-curr->y+1;
    curr->top = curr->y;
    ScrollDownMap(n);
    if (!display) {
	curr->top = old;
	return;
    }
    if (AL || CAL) {
	Goto(curr->y, curr->x, curr->y, 0);
	if (CAL && !(n == 1 && AL))
	    CPutStr(CAL, n);
	else {
	    for (i = n; i; i--)
		PutStr(AL);
	}
	Goto(curr->y, 0, curr->y, curr->x);
	lp_missing = 0;
    } else if (CS && SR) {
	PutStr(tgoto(CS, curr->bot, curr->top));
	Goto(-1, -1, curr->y, 0);
	for (i = n; i; i--)
	    PutStr(SR);
	PutStr(tgoto(CS, curr->bot, old));
	Goto(-1, -1, curr->y, curr->x);
	lp_missing = 0;
    } else
	Redisplay();
    curr->top = old;
}

static ScrollUpMap(n) {
    char tmp[128*sizeof (char *)];
    register i, cnt1, cnt2;
    register char **ppi, **ppa, **ppf;

    i = curr->top+n;
    cnt1 = n * sizeof (char *);
    cnt2 = (curr->bot - i + 1) * sizeof (char *);
    ppi = curr->image+i;
    ppa = curr->attr +i;
    ppf = curr->font +i;
    for (i = n; i; --i) {
	bclear(*--ppi, cols);
	bzero(*--ppa, cols);
	bzero(*--ppf, cols);
    }
    Scroll((char *)ppi, cnt1, cnt2, tmp);
    Scroll((char *)ppa, cnt1, cnt2, tmp);
    Scroll((char *)ppf, cnt1, cnt2, tmp);
}

static ScrollDownMap(n) {
    char tmp[128*sizeof (char *)];
    register i, cnt1, cnt2;
    register char **ppi, **ppa, **ppf;

    i = curr->top;
    cnt1 = (curr->bot - i - n + 1) * sizeof (char *);
    cnt2 = n * sizeof (char *);
    Scroll((char *)(ppi = curr->image+i), cnt1, cnt2, tmp);
    Scroll((char *)(ppa = curr->attr +i), cnt1, cnt2, tmp);
    Scroll((char *)(ppf = curr->font +i), cnt1, cnt2, tmp);
    for (i = n; i; --i) {
	bclear(*ppi++, cols);
	bzero(*ppa++, cols);
	bzero(*ppf++, cols);
    }
}

Scroll(cp, cnt1, cnt2, tmp) char *cp, *tmp; int cnt1, cnt2; {
    if (!cnt1 || !cnt2)
	return;
    if (cnt1 <= cnt2) {
	bcopy(cp, tmp, cnt1);
	bcopy(cp+cnt1, cp, cnt2);
	bcopy(tmp, cp+cnt2, cnt1);
    } else {
	bcopy(cp+cnt1, tmp, cnt2);
	bcopy(cp, cp+cnt2, cnt1);
	bcopy(tmp, cp, cnt2);
    }
}

static ForwardTab() {
    register x = curr->x;

    if (x == cols) {
	LineFeed(2);
	x = 0;
    }
    if (curr->tabs[x] && x < cols-1)
	++x;
    while (x < cols-1 && !curr->tabs[x])
	x++;
    Goto(curr->y, curr->x, curr->y, x);
    curr->x = x;
}

static BackwardTab() {
    register x = curr->x;

    if (curr->tabs[x] && x > 0)
	x--;
    while (x > 0 && !curr->tabs[x])
	x--;
    Goto(curr->y, curr->x, curr->y, x);
    curr->x = x;
}

static ClearScreen() {
    register i;
    register char **ppi = curr->image, **ppa = curr->attr, **ppf = curr->font;

    if (display) {
	PutStr(CL);
	lp_missing = 0;
    }
    for (i = 0; i < rows; ++i) {
	bclear(*ppi++, cols);
	bzero(*ppa++, cols);
	bzero(*ppf++, cols);
    }
}

static ClearFromBOS() {
    register n, y = curr->y, x = curr->x;

    last_y = y; last_x = x;
    for (n = 0; n < y; ++n)
	ClearInLine(1, n, 0, cols-1);
    ClearInLine(1, y, 0, x);
    Goto(last_y, last_x, y, x);
    RestoreAttr();
}

static ClearToEOS() {
    register n, y = curr->y, x = curr->x;

    if (!y && !x) {
	ClearScreen();
	return;
    }
    if (display && CD) {
	PutStr(CD);
	lp_missing = 0;
    }
    last_y = y; last_x = x;
    ClearInLine(!CD, y, x, cols-1);
    for (n = y+1; n < rows; n++)
	ClearInLine(!CD, n, 0, cols-1);
    Goto(last_y, last_x, y, x);
    RestoreAttr();
}

static ClearLine() {
    register y = curr->y, x = curr->x;

    last_y = y; last_x = x;
    ClearInLine(1, y, 0, cols-1);
    Goto(last_y, last_x, y, x);
    RestoreAttr();
}

static ClearToEOL() {
    register y = curr->y, x = curr->x;

    last_y = y; last_x = x;
    ClearInLine(1, y, x, cols-1);
    Goto(last_y, last_x, y, x);
    RestoreAttr();
}

static ClearFromBOL() {
    register y = curr->y, x = curr->x;

    last_y = y; last_x = x;
    ClearInLine(1, y, 0, x);
    Goto(last_y, last_x, y, x);
    RestoreAttr();
}

static ClearInLine(displ, y, x1, x2) {
    register i, n;

    if (x1 == cols) x1--;
    if (x2 == cols) x2--;
    if ((n = x2 - x1 + 1) != 0) {
	if (displ && display) {
	    if (x2 == cols-1 && CE) {
		Goto(last_y, last_x, y, x1);
		last_y = y; last_x = x1;
		PutStr(CE);
		if (y == curr->bot)
		    lp_missing = 0;
	    } else
		DisplayLine(curr->image[y], curr->attr[y], curr->font[y],
			blank, null, null, y, x1, x2);
	}
	bclear(curr->image[y]+x1, n);
	bzero(curr->attr[y]+x1, n);
	bzero(curr->font[y]+x1, n);
    }
}

static CursorRight(n) register n; {
    register x = curr->x;

    if (x == cols) {
	LineFeed(2);
	x = 0;
    }
    if ((curr->x += n) >= cols)
	curr->x = cols-1;
    Goto(curr->y, x, curr->y, curr->x);
}

static CursorUp(n) register n; {
    register y = curr->y;

    if ((curr->y -= n) < curr->top)
	curr->y = curr->top;
    Goto(y, curr->x, curr->y, curr->x);
}

static CursorDown(n) register n; {
    register y = curr->y;

    if ((curr->y += n) > curr->bot)
	curr->y = curr->bot;
    Goto(y, curr->x, curr->y, curr->x);
}

static CursorLeft(n) register n; {
    register x = curr->x;

    if ((curr->x -= n) < 0)
	curr->x = 0;
    Goto(curr->y, x, curr->y, curr->x);
}

static SetMode(on) {
    register i;

    for (i = 0; i < curr->NumArgs; ++i) {
	switch (curr->args[i]) {
	case 4:
	    curr->insert = on;
	    InsertMode(on);
	    break;
	}
    }
}

static SelectRendition() {
    register i = 0, a = curr->LocalAttr;

    do {
	switch (curr->args[i]) {
	case 0:
	    a = 0; break;
	case 1:
	    a |= A_BD; break;
	case 2:
	    a |= A_DI; break;
	case 3:
	    a |= A_SO; break;
	case 4:
	    a |= A_US; break;
	case 5:
	    a |= A_BL; break;
	case 7:
	    a |= A_RV; break;
	case 22:
	    a &= ~(A_BD|A_SO|A_DI); break;
	case 23:
	    a &= ~A_SO; break;
	case 24:
	    a &= ~A_US; break;
	case 25:
	    a &= ~A_BL; break;
	case 27:
	    a &= ~A_RV; break;
	}
    } while (++i < curr->NumArgs);
    NewRendition(curr->LocalAttr = a);
}

static NewRendition(new) register new; {
    register i, old = GlobalAttr;

    if (!display || old == new)
	return;
    GlobalAttr = new;
    for (i = 1; i <= A_MAX; i <<= 1) {
	if ((old & i) && !(new & i)) {
	    PutStr(UE);
	    PutStr(SE);
	    PutStr(ME);
	    if (new & A_US) PutStr(US);
	    if (new & A_SO) PutStr(SO);
	    if (new & A_BL) PutStr(MB);
	    if (new & A_BD) PutStr(MD);
	    if (new & A_DI) PutStr(MH);
	    if (new & A_RV) PutStr(MR);
	    return;
	}
    }
    if ((new & A_US) && !(old & A_US))
	PutStr(US);
    if ((new & A_SO) && !(old & A_SO))
	PutStr(SO);
    if ((new & A_BL) && !(old & A_BL))
	PutStr(MB);
    if ((new & A_BD) && !(old & A_BD))
	PutStr(MD);
    if ((new & A_DI) && !(old & A_DI))
	PutStr(MH);
    if ((new & A_RV) && !(old & A_RV))
	PutStr(MR);
}

static SaveSetAttr(newattr, newcharset) {
    NewRendition(newattr);
    NewCharset(newcharset);
    InsertMode(0);
}

static RestoreAttr() {
    NewRendition(curr->LocalAttr);
    NewCharset(curr->charsets[curr->LocalCharset]);
    InsertMode(curr->insert);
}

static FillWithEs() {
    register i;
    register char *p, *ep;

    curr->y = curr->x = 0;
    for (i = 0; i < rows; ++i) {
	bzero(curr->attr[i], cols);
	bzero(curr->font[i], cols);
	p = curr->image[i];
	ep = p + cols;
	for ( ; p < ep; ++p)
	    *p = 'E';
    }
    if (display)
	Redisplay();
}

static Redisplay() {
    register i;

    PutStr(CL);
    lp_missing = 0;
    InsertMode(0);
    last_x = last_y = 0;
    for (i = 0; i < rows; ++i)
	DisplayLine(blank, null, null, curr->image[i], curr->attr[i],
	    curr->font[i], i, 0, cols-1);
    InsertMode(curr->insert);
    Goto(last_y, last_x, curr->y, curr->x);
    NewRendition(curr->LocalAttr);
    NewCharset(curr->charsets[curr->LocalCharset]);
}

static DisplayLine(os, oa, of, s, as, fs, y, from, to)
	register char *os, *oa, *of, *s, *as, *fs; {
    register x;
    int last2flag = 0, delete_lp = 0;

    if (!LP && y == curr->bot && to == cols-1)
	if (lp_missing
	 || s[to] != os[to] || as[to] != oa[to] || of[to] != fs[to]) {
	    if (IC || IM) {
		if ((to -= 2) < from-1)
		    from--;
		last2flag = 1;
		lp_missing = 0;
	    } else {
		to--;
		delete_lp = (CE || DC || CDC);
		lp_missing = (s[to] != ' ' || as[to] || fs[to]);
	    }
	} else
	    to--;
    for (x = from; x <= to; ++x) {
	if (s[x] == os[x] && as[x] == oa[x] && of[x] == fs[x])
	    continue;
	Goto(last_y, last_x, y, x);
	NewRendition(as[x]);
	NewCharset(fs[x]);
	putchar(s[x]);
	last_y = y;
	last_x = x+1;
    }
    if (last_x == cols) {
	if (AM && !LP) {
	    last_x = 0;
	    last_y++;
	}
    } else if (last2flag) {
	Goto(last_y, last_x, y, x);
	NewRendition(as[x+1]);
	NewCharset(fs[x+1]);
	putchar(s[x+1]);
	Goto(y, x+1, y, x);
	NewRendition(as[x]);
	NewCharset(fs[x]);
	InsertMode(1);
	PutStr(IC);
	putchar(s[x]);
	InsertMode(curr->insert);
	last_y = y;
	last_x = x+1;
    } else if (delete_lp) {
	if (DC)
	    PutStr(DC);
	else if (DC)
	    CPutStr(CDC, 1);
	else if (CE)
	    PutStr(CE);
    }
}

static RedisplayLine(os, oa, of, y, from, to) char *os, *oa, *of; {
    InsertMode(0);
    last_y = y;
    last_x = from;
    DisplayLine(os, oa, of, curr->image[y], curr->attr[y],
	curr->font[y], y, from, to);
    NewRendition(curr->LocalAttr);
    NewCharset(curr->charsets[curr->LocalCharset]);
    InsertMode(curr->insert);
}

FixLP(y1, x1, y2, x2) register y1, x1, y2, x2; {
    register struct win *p = curr;

    Goto(y1, x1, y2, x2);
    SaveSetAttr(p->attr[y2][x2], p->font[y2][x2]);
    putchar(p->image[y2][x2]);
    RestoreAttr();
    if (++x2 == cols) {
	if (y2 != p->bot)
	    y2++;
	x2 = 0;
    }
    Goto(y2, x2, p->y, p->x);
    lp_missing = 0;
}

CheckLP(n_ch) char n_ch; {
    register y = curr->bot, x = cols-1;
    register char n_at, n_fo, o_ch, o_at, o_fo;

    o_ch = curr->image[y][x];
    o_at = curr->attr[y][x];
    o_fo = curr->font[y][x];

    n_at = curr->LocalAttr;
    n_fo = curr->charsets[curr->LocalCharset];

    if (n_ch == o_ch && n_at == o_at && n_fo == o_fo) {
	lp_missing = 0;
	return;
    }
    if (n_ch != ' ' || n_at || n_fo)
	lp_missing = 1;
    else
	lp_missing = 0;
    if (o_ch != ' ' || o_at || o_fo) {
	if (DC)
	    PutStr(DC);
	else if (DC)
	    CPutStr(CDC, 1);
	else if (CE)
	    PutStr(CE);
    }
}

static FindAKA() {
    register char *cp, *line, ch;
    register struct win *wp = curr;
    register len = strlen(wp->cmd);
    int y;

    y = (wp->autoaka > 0 ? wp->autoaka-1 : wp->y);
    cols = wp->width;
  try_line:
    cp = line = wp->image[y];
    if (wp->autoaka > 0 && (ch = *wp->cmd) != '\0') {
	for (;;) {
	    if ((cp = index(cp,ch)) != NULL
	     && !strncmp(cp,wp->cmd,len))
		break;
	    if (!cp || ++cp - line >= cols-len) {
		if (++y == wp->autoaka && y < rows)
		    goto try_line;
		return;
	    }
	}
	cp += len;
    }
    for (len = cols - (cp - line); len && *cp == ' '; len--, cp++)
	;
    if (len) {
	if (wp->autoaka > 0 && (*cp == '!' || *cp == '%' || *cp == '^'))
	    wp->autoaka = -1;
	else
	    wp->autoaka = 0;
	line = wp->cmd+wp->akapos;
	while (len && *cp != ' ') {
	    if ((*line++ = *cp++) == '/')
		line = wp->cmd+wp->akapos;
	    len--;
	}
	*line = '\0';
    } else
	wp->autoaka = 0;
}

InputAKA() {
    register char *s, *cp, ch;

    display = 1;
    curr = fore;
    cols = curr->width;
    Goto(curr->y, curr->x, last_y = STATLINE, last_x = 0);
    if (CE)
	PutStr(CE);
    else {
	DisplayLine(curr->image[last_y],curr->attr[last_y],curr->font[last_y],
		blank,null,null, last_y, 0,cols-1);
	Goto(last_y, last_x, STATLINE, 0);
    }
    SaveSetAttr(A_SO, ASCII);
    printf("Set window's a.k.a. to:");
    SaveSetAttr(0, ASCII);
    putchar(' ');
    s = cp = curr->cmd+curr->akapos;
    while ((ch = getchar()) != '\n' && ch != '\r') {
	if (ch > ' ' && ch <= '~' && cp-s < 20) {
	    *cp++ = ch;
	    putchar( ch );
	} else if ((ch == '\b' || ch == 0177) && cp > s) {
	    cp--;
	    printf("\b \b");
	}
    }
    *cp = '\0';
    PutStr(CR);
    if (CE) {
	PutStr(CE);
	DisplayLine(blank,null,null,
		curr->image[last_y],curr->attr[last_y],curr->font[last_y],
		last_y, last_x = 0,cols-1);
    } else {
	int last = strlen(curr->cmd+curr->akapos) + 24;

	DisplayLine(null,null,null,
		curr->image[last_y],curr->attr[last_y],curr->font[last_y],
		last_y, last_x = 0,last-1);
	DisplayLine(blank,null,null,
		curr->image[last_y],curr->attr[last_y],curr->font[last_y],
		last_y, last,cols-1);
    }
    Goto(last_y, last_x, curr->y, curr->x);
    RestoreAttr();
}

static MakeBlankLine(p, n) register char *p; register n; {
    while (n--)
	*p++ = ' ';
}

MakeStatus(msg) char *msg; {
    register char *s, *t;
    register max;

    curr = fore;
    display = 1;
    if (!(max = HS)) {
	cols = curr->width;
	max = !LP ? cols-1 : cols;
    }
    for (s = t = msg; *s && t - msg < max; ++s)
	if (*s >= ' ' && *s <= '~' || *s == BELL)
	    *t++ = *s;
    *t = '\0';
    if (status) {
	if (time((time_t *)0) - TimeDisplayed < 2)
	    sleep(1);
	RemoveStatus();
    }
    if (t > msg) {
	if (msg != LastMsg)
	    strncpy(LastMsg, msg, maxwidth);
	status = 1;
	StatLen = t - msg;
	SaveSetAttr(A_SO, ASCII);
	if (!HS) {
	    Goto(curr->y, curr->x, STATLINE, 0);
	    printf("%s", msg);
	} else {
	    CPutStr(TS,0);
	    printf("%s", msg);
	    PutStr(FS);
	}
	RestoreAttr();
	(void) fflush(stdout);
	(void) time(&TimeDisplayed);
    }
}

RemoveStatus() {
    if (!status)
	return;
    status = 0;
    curr = fore;
    display = 1;
    if (!HS) {
	cols = curr->width;
	Goto(-1, -1, STATLINE, 0);
	if (!help_page) {
	    RedisplayLine(null, null, null, STATLINE, 0, StatLen-1);
	    Goto(last_y, last_x, curr->y, curr->x);
	} else {
	    DisplayLine(null, null, null, blank,
			null, null, last_y = STATLINE, last_x = 0, StatLen-1);
	    Goto(last_y, last_x, rows-3, 0);
	}
    } else
	PutStr(DS);
}

ChangeWidth(ptyfd, width) int ptyfd; register width; {
    if (Z0) {
	if (lastwidth != width) {
	    PutStr(width == minwidth ? Z1 : Z0);
	    lastwidth = width;
#ifdef TIOCSWINSZ
	    Window_Size.ws_col = width;
	    ioctl(ptyfd, TIOCSWINSZ, &Window_Size);
#endif
	}
    }
}

init_help_screen() {
    RemoveStatus();
    display = 1;
    NewRendition(0);
    NewCharset(ASCII);
    InsertMode(0);
    if (CS)
	PutStr(tgoto(CS, rows-1, 0));
    PutStr(CL);
}
