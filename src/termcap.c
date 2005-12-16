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
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <sys/types.h>
#include "config.h"
#include "screen.h"
#include "extern.h"

extern struct display *display;

static void  AddCap __P((char *));
static void  MakeString __P((char *, char *, int, char *));
static char *findcap __P((char *, char **, int));
static char *e_tgetstr __P((char *, char **));
static int   e_tgetflag __P((char *));
static int   e_tgetnum __P((char *));

extern struct term term[];	/* terminal capabilities */
extern struct NewWindow nwin_undef, nwin_default, nwin_options;
extern int force_vt, assume_LP;
extern int Z0width, Z1width;

char Termcap[TERMCAP_BUFSIZE + 8];	/* new termcap +8:"TERMCAP=" */
static int Termcaplen;
static int tcLineLen;
char Term[MAXSTR+5];		/* +5: "TERM=" */
char screenterm[20];		/* new $TERM, usually "screen" */

char *extra_incap, *extra_outcap;

static const char TermcapConst[] = "\\\n\
\t:DO=\\E[%dB:LE=\\E[%dD:RI=\\E[%dC:UP=\\E[%dA:bs:bt=\\E[Z:\\\n\
\t:cd=\\E[J:ce=\\E[K:cl=\\E[H\\E[J:cm=\\E[%i%d;%dH:ct=\\E[3g:\\\n\
\t:do=^J:nd=\\E[C:pt:rc=\\E8:rs=\\Ec:sc=\\E7:st=\\EH:up=\\EM:\\\n\
\t:le=^H:bl=^G:cr=^M:it#8:ho=\\E[H:nw=\\EE:ta=^I:is=\\E)0:";

char *
gettermcapstring(s)
char *s;
{
  int i;

  if (display == 0 || s == 0)
    return 0;
  for (i = 0; i < T_N; i++)
    {
      if (term[i].type != T_STR)
	continue;
      if (strcmp(term[i].tcname, s) == 0)
	return d_tcs[i].str;
    }
  return 0;
}

int
InitTermcap(wi, he)
int wi;
int he;
{
  register char *s;
  int i;
  char tbuf[TERMCAP_BUFSIZE], *tp;

  ASSERT(display);
  bzero(tbuf, sizeof(tbuf));
  debug1("InitTermcap: looking for tgetent('%s')\n", d_termname);
  if (tgetent(tbuf, d_termname) != 1)
    {
      Msg(0, "Cannot find termcap entry for %s.", d_termname);
      return -1;
    }
  debug1("got it:\n%s\n",tbuf);
#ifdef DEBUG
  if (extra_incap)
    debug1("Extra incap: %s\n", extra_incap);
  if (extra_outcap)
    debug1("Extra outcap: %s\n", extra_outcap);
#endif
  tp = d_tentry;

  for (i = 0; i < T_N; i++)
    {
      switch(term[i].type)
	{
	case T_FLG:
	  d_tcs[i].flg = e_tgetflag(term[i].tcname);
	  break;
	case T_NUM:
	  d_tcs[i].num = e_tgetnum(term[i].tcname);
	  break;
	case T_STR:
	  d_tcs[i].str = e_tgetstr(term[i].tcname, &tp);
	  /* no empty strings, please */
	  if (d_tcs[i].str && *d_tcs[i].str == 0)
	    d_tcs[i].str = 0;
	  break;
	default:
	  Panic(0, "Illegal tc type in entry #%d", i);
	  /*NOTREACHED*/
	}
    }
  if (HC)
    {
      Msg(0, "You can't run screen on a hardcopy terminal.");
      return -1;
    }
  if (OS)
    {
      Msg(0, "You can't run screen on a terminal that overstrikes.");
      return -1;
    }
  if (NS)
    {
      Msg(0, "Terminal must support scrolling.");
      return -1;
    }
  if (!CL)
    {
      Msg(0, "Clear screen capability required.");
      return -1;
    }
  if (!CM)
    {
      Msg(0, "Addressable cursor capability required.");
      return -1;
    }
  if ((s = getenv("COLUMNS")) && (i = atoi(s)) > 0)
    CO = i;
  if ((s = getenv("LINES")) && (i = atoi(s)) > 0)
    LI = i;
  if (wi)
    CO = wi;
  if (he)
    LI = he;
  if (CO <= 0)
    CO = 80;
  if (LI <= 0)
    LI = 24;

  if (nwin_options.flowflag == nwin_undef.flowflag)
    nwin_default.flowflag = CNF ? FLOW_NOW * 0 : 
			    XO ? FLOW_NOW * 1 :
			    FLOW_AUTOFLAG;
  CLP |= assume_LP || !AM || XV || XN ||
	 (!extra_incap && !strncmp(d_termname, "vt", 2));
  if (!(BL = e_tgetstr("bl", &tp)))
  if (!BL)
    BL = "\007";
  if (!BC)
    {
      if (BS)
	BC = "\b";
      else
	BC = LE;
    }
  if (!CR)
    CR = "\r";
  if (!NL)
    NL = "\n";
  if (SG <= 0 && UG <= 0)
    {
      /*
       * Does ME also reverse the effect of SO and/or US?  This is not
       * clearly specified by the termcap manual. Anyway, we should at
       * least look whether ME and SE/UE are equal:
       */
      if (UE && ((SE && strcmp(SE, UE) == 0) || (ME && strcmp(ME, UE) == 0)))
	UE = 0;
      if (SE && (ME && strcmp(ME, SE) == 0))
	SE = 0;

      for (i = 0; i < NATTR; i++)
	d_attrtab[i] = d_tcs[T_ATTR + i].str;
      /* Set up missing entries */
      s = 0;
      for (i = NATTR-1; i >= 0; i--)
	if (d_attrtab[i])
	  s = d_attrtab[i];
      for (i = 0; i < NATTR; i++)
	{
	  if (d_attrtab[i] == 0)
	    d_attrtab[i] = s;
	  else
	    s = d_attrtab[i];
	}
    }
  else
    {
      MS = 1;
      for (i = 0; i < NATTR; i++)
	d_attrtab[i] = d_tcs[T_ATTR + i].str = 0;
    }
  if (!DO)
    DO = NL;
  if (!SF)
    SF = NL;
  if (IN)
    IC = IM = 0;
  if (EI == 0)
    IM = 0;
  /* some strange termcap entries have IC == IM */
  if (IC && IM && strcmp(IC, IM) == 0)
    IC = 0;
  if (KE == 0)
    KS = 0;
  if (CCE == 0)
    CCS = 0;
  if (CG0)
    {
      if (CS0 == 0)
#ifdef TERMINFO
        CS0 = "\033(%p1%c";
#else
        CS0 = "\033(%.";
#endif
      if (CE0 == 0)
        CE0 = "\033(B";
    }
  else if (AS && AE)
    {
      CG0 = 1;
      CS0 = AS;
      CE0 = AE;
      CC0 = AC;
    }
  else
    {
      CS0 = CE0 = "";
      CC0 = "g.h.i'j-k-l-m-n+o~p\"q-r-s_t+u+v+w+x|y<z>";
    }
  for (i = 0; i < 256; i++)
    d_c0_tab[i] = i;
  if (CC0)
    for (i = strlen(CC0) & ~1; i >= 0; i -= 2)
      d_c0_tab[(int)(unsigned char)CC0[i]] = CC0[i + 1];
  debug1("ISO2022 = %d\n", CG0);
  if (PF == 0)
    PO = 0;
  debug2("terminal size is %d, %d (says TERMCAP)\n", CO, LI);

  /* Termcap fields Z0 & Z1 contain width-changing sequences. */
  if (CZ1 == 0)
    CZ0 = 0;
  Z0width = 132;
  Z1width = 80;

  CheckScreenSize(0);

  if (TS == 0 || FS == 0 || DS == 0)
    HS = 0;
  if (HS)
    {
      debug("oy! we have a hardware status line, says termcap\n");
      if (WS <= 0)
        WS = d_width;
    }

  d_UPcost = CalcCost(UP);
  d_DOcost = CalcCost(DO);
  d_NLcost = CalcCost(NL);
  d_LEcost = CalcCost(BC);
  d_NDcost = CalcCost(ND);
  d_CRcost = CalcCost(CR);
  d_IMcost = CalcCost(IM);
  d_EIcost = CalcCost(EI);

#ifdef AUTO_NUKE
  if (CAN)
    {
      debug("termcap has AN, setting autonuke\n");
      d_auto_nuke = 1;
    }
#endif
  if (COL > 0)
    {
      debug1("termcap has OL (%d), setting limit\n", COL);
      d_obufmax = COL;
    }

  d_tcinited = 1;
  MakeTermcap(0);
  return 0;
}


static void
AddCap(s)
char *s;
{
  register int n;

  if (tcLineLen + (n = strlen(s)) > 55 && Termcaplen < TERMCAP_BUFSIZE - 4)
    {
      strcpy(Termcap + Termcaplen, "\\\n\t:");
      Termcaplen += 4;
      tcLineLen = 0;
    }
  if (Termcaplen + n < TERMCAP_BUFSIZE)
    {
      strcpy(Termcap + Termcaplen, s);
      Termcaplen += n;
      tcLineLen += n;
    }
  else
    Panic(0, "TERMCAP overflow - sorry.");
}

char *
MakeTermcap(aflag)
int aflag;
{
  char buf[TERMCAP_BUFSIZE];
  register char *p, *cp, *s, ch, *tname;
  int i, wi, he;

  if (display)
    {
      wi = d_width;
      he = d_height;
      tname = d_termname;
    }
  else
    {
      wi = 80;
      he = 24;
      tname = "vt100";
    }
  debug1("MakeTermcap(%d)\n", aflag);
  if ((s = getenv("SCREENCAP")) && strlen(s) < TERMCAP_BUFSIZE)
    {
      sprintf(Termcap, "TERMCAP=%s", s);     /* TERMCAP_BUFSIZE + ... ? XXX */
      sprintf(Term, "TERM=screen");
      debug("getenvSCREENCAP o.k.\n");
      return Termcap;
    }
  Termcaplen = 0;
  debug1("MakeTermcap screenterm='%s'\n", screenterm);
  debug1("MakeTermcap termname='%s'\n", tname);
  if (*screenterm == '\0')
    {
      debug("MakeTermcap sets screenterm=screen\n");
      strcpy(screenterm, "screen");
    }
  do
    {
      sprintf(Term, "TERM=");
      p = Term + 5;
      if (!aflag && strlen(screenterm) + strlen(tname) < MAXSTR-1)
	{
	  sprintf(p, "%s.%s", screenterm, tname);
	  if (tgetent(buf, p) == 1)
	    break;
	}
      if (wi >= 132)
	{
	  sprintf(p, "%s-w", screenterm);
          if (tgetent(buf, p) == 1)
	    break;
	}
      sprintf(p, "%s", screenterm);
      if (tgetent(buf, p) == 1)
	break;
      sprintf(p, "vt100");
    }
  while (0);		/* Goto free programming... */
  tcLineLen = 100;	/* Force NL */
  sprintf(Termcap,
	  "TERMCAP=SC|%s|VT 100/ANSI X3.64 virtual terminal|", Term + 5);
  Termcaplen = strlen(Termcap);
  debug1("MakeTermcap decided '%s'\n", p);
  if (extra_outcap && *extra_outcap)
    {
      for (cp = extra_outcap; (p = index(cp, ':')); cp = p)
	{
	  ch = *++p;
	  *p = '\0';
	  AddCap(cp);
	  *p = ch;
	}
      tcLineLen = 100;	/* Force NL */
    }
  debug1("MakeTermcap after outcap '%s'\n", (char *)TermcapConst);
  if (Termcaplen + strlen(TermcapConst) < TERMCAP_BUFSIZE)
    {
      strcpy(Termcap + Termcaplen, (char *)TermcapConst);
      Termcaplen += strlen(TermcapConst);
    }
  sprintf(buf, "li#%d:co#%d:", he, wi);
  AddCap(buf);
  AddCap("am:");
  if (aflag || (force_vt && !COP) || CLP || !AM)
    {
      AddCap("xn:");
      AddCap("xv:");
      AddCap("LP:");
    }
  if (aflag || (CS && SR) || AL || CAL)
    {
      AddCap("sr=\\EM:");
      AddCap("al=\\E[L:");
      AddCap("AL=\\E[%dL:");
    }
  else if (SR)
    AddCap("sr=\\EM:");
  if (aflag || CS)
    AddCap("cs=\\E[%i%d;%dr:");
  if (aflag || CS || DL || CDL)
    {
      AddCap("dl=\\E[M:");
      AddCap("DL=\\E[%dM:");
    }
  if (aflag || DC || CDC)
    {
      AddCap("dc=\\E[P:");
      AddCap("DC=\\E[%dP:");
    }
  if (aflag || CIC || IC || IM)
    {
      AddCap("im=\\E[4h:");
      AddCap("ei=\\E[4l:");
      AddCap("mi:");
      AddCap("IC=\\E[%d@:");
    }
  if (display)
    {
      if (US)
	{
	  AddCap("us=\\E[4m:");
	  AddCap("ue=\\E[24m:");
	}
      if (SO)
	{
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
      if (VB)
	AddCap("vb=\\E[?5h\\E[?5l:");
      if (KS)
	{
	  AddCap("ks=\\E=:");
	  AddCap("ke=\\E>:");
	}
      if (CCS)
	{
	  AddCap("CS=\\E[?1h:");
	  AddCap("CE=\\E[?1l:");
	}
      if (CG0)
	{
	  AddCap("G0:");
	  AddCap("as=\\E(0:");
	  AddCap("ae=\\E(B:");
	}
      if (PO)
	{
	  AddCap("po=\\E[5i:");
	  AddCap("pf=\\E[4i:");
	}
      if (CZ0)
	{
	  AddCap("Z0=\\E[?3h:");
	  AddCap("Z1=\\E[?3l:");
	}
      if (CWS)
	AddCap("WS=\\E[8;%d;%dt:");
      for (i = T_CAPS; i < T_ECAPS; i++)
	{
	  switch(term[i].type)
	    {
	    case T_STR:
	      if (d_tcs[i].str == 0)
		break;
	      MakeString(term[i].tcname, buf, sizeof(buf), d_tcs[i].str);
	      AddCap(buf);
	      break;
	    case T_FLG:
	      if (d_tcs[i].flg == 0)
		break;
	      sprintf(buf, "%s:", term[i].tcname);
	      AddCap(buf);
	      break;
	    default:
	      break;
	    }
	}
    }
  debug("MakeTermcap: end\n");
  return Termcap;
}

static void
MakeString(cap, buf, buflen, s)
char *cap, *buf;
int buflen;
char *s;
{
  register char *p, *pmax;
  register unsigned int c;

  p = buf;
  pmax = p + buflen - (3+4+2);
  *p++ = *cap++;
  *p++ = *cap;
  *p++ = '=';
  while ((c = *s++) && (p < pmax))
    {
      switch (c)
	{
	case '\033':
	  *p++ = '\\';
	  *p++ = 'E';
	  break;
	case ':':
	  sprintf(p, "\\072");
	  p += 4;
	  break;
	case '^':
	case '\\':
	  *p++ = '\\';
	  *p++ = c;
	  break;
	default:
	  if (c >= 200)
	    {
	      sprintf(p, "\\%03o", c & 0377);
	      p += 4;
	    }
	  else if (c < ' ')
	    {
	      *p++ = '^';
	      *p++ = c + '@';
	    }
	  else
	    *p++ = c;
	}
    }
  *p++ = ':';
  *p = '\0';
}


/*
**
**  Termcap routines that use our extra_incap
**
*/


/* findcap:
 *   cap = capability we are looking for
 *   tepp = pointer to bufferpointer
 *   n = size of buffer (0 = infinity)
 */

static char *
findcap(cap, tepp, n)
char *cap;
char **tepp;
int n;
{
  char *tep;
  char c, *p, *cp;
  int mode;	/* mode: 0=LIT  1=^  2=\x  3,4,5=\nnn */
  int num = 0, capl;

  if (!extra_incap)
    return (0);
  tep = *tepp;
  capl = strlen(cap);
  cp = 0;
  mode = 0;
  for (p = extra_incap; *p; )
    {
      if (strncmp(p, cap, capl) == 0)
	{
	  p += capl;
	  c = *p;
	  if (c && c != ':' && c != '@')
	    p++;
	  if (c == 0 || c == '@' || c == '=' || c == ':' || c == '#')
	    cp = tep;
	}
      while ((c = *p))
	{
	  p++;
	  if (mode == 0)
	    {
	      if (c == ':')
	        break;
	      if (c == '^')
		mode = 1;
	      if (c == '\\')
		mode = 2;
	    }
	  else if (mode == 1)
	    {
	      mode = 0;
	      c = c & 0x1f;
	    }
	  else if (mode == 2)
	    {
	      mode = 0;
	      switch(c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		  mode = 3;
		  num = 0;
		  break;
		case 'E':
		  c = 27;
		  break;
		case 'n':
		  c = '\n';
		  break;
		case 'r':
		  c = '\r';
		  break;
		case 't':
		  c = '\t';
		  break;
		case 'b':
		  c = '\b';
		  break;
		case 'f':
		  c = '\f';
		  break;
		}
	    }
	  if (mode > 2)
	    {
	      num = num * 8 + (c - '0');
	      if (mode++ == 5 || (*p < '0' || *p > '9'))
		{
		  c = num;
		  mode = 0;
		}
	    }
	  if (mode)
	    continue;

	  if (cp && n != 1)
	    {
	      *cp++ = c;
	      n--;
	    }
	}
      if (cp)
	{
	  *cp++ = 0;
	  *tepp = cp;
	  debug2("'%s' found in extra_incap -> %s\n", cap, tep);
	  return(tep);
	}
    }
  return(0);
}

static char *
e_tgetstr(cap, tepp)
char *cap;
char **tepp;
{
  char *tep, *tgetstr();
  if ((tep = findcap(cap, tepp, 0)))
    return((*tep == '@') ? 0 : tep);
  return (tgetstr(cap, tepp));
}

static int
e_tgetflag(cap)
char *cap;
{
  char buf[2], *bufp;
  char *tep;
  bufp = buf;
  if ((tep = findcap(cap, &bufp, 2)))
    return((*tep == '@') ? 0 : 1);
  return (tgetflag(cap));
}

static int
e_tgetnum(cap)
char *cap;
{
  char buf[20], *bufp;
  char *tep, c;
  int res, base = 10;

  bufp = buf;
  if ((tep = findcap(cap, &bufp, 20)))
    {
      c = *tep;
      if (c == '@')
	return(-1);
      if (c == '0')
	base = 8;
      res = 0;
      while ((c = *tep++) >= '0' && c <= '9')
	res = res * base + (c - '0');
      return(res);
    }
  return (tgetnum(cap));
}
