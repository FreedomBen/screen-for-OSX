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

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#if !defined(sun) && !defined(B43) && !defined(ISC) && !defined(pyr) && !defined(_CX_UX)
# include <time.h>
#endif
#include <sys/time.h>
#ifndef sun
#include <sys/ioctl.h>
#endif


#include "config.h"

#ifdef SVR4
# include <sys/stropts.h>
#endif

#include "screen.h"
#include "extern.h"

extern struct comm comms[];
extern char *rc_name;
extern char *RcFileName, *home;
extern char *BellString, *ActivityString, *ShellProg, *ShellArgs[];
extern char *hardcopydir, *screenlogfile;
extern char *VisualBellString;
extern int VBellWait, MsgWait, MsgMinWait, SilenceWait;
extern char SockPath[], *SockName;
extern int TtyMode, auto_detach;
extern int iflag;
extern int use_hardstatus, visual_bell;
extern char *printcmd;
extern int default_startup;
extern int slowpaste, defobuflimit;
extern int ZombieKey_destroy;
extern int ZombieKey_resurrect;
#ifdef AUTO_NUKE
extern int defautonuke;
#endif
extern int intrc, origintrc; /* display? */
extern struct NewWindow nwin_default, nwin_undef;
#ifdef COPY_PASTE
extern int join_with_cr, pastefont;
extern unsigned char mark_key_tab[];
extern char *BufferFile;
#endif
#ifdef POW_DETACH
extern char *BufferFile, *PowDetachString;
#endif
extern time_t Now;
#ifdef MAPKEYS
extern struct term term[];      /* terminal capabilities */
extern int maptimeout;
extern char *kmapdef[];
extern char *kmapadef[];
extern char *kmapmdef[];
#endif
extern struct mchar mchar_so;


static int  CheckArgNum __P((int, char **));
static void ClearAction __P((struct action *));
static int  NextWindow __P((void));
static int  PreviousWindow __P((void));
static int  MoreWindows __P((void));
static void LogToggle __P((int));
static void ShowTime __P((void));
static void ShowInfo __P((void));
static void SwitchWindow __P((int));
static char **SaveArgs __P((char **));
static struct win *WindowByName __P((char *));
static int  WindowByNumber __P((char *));
static void DoAction  __P((struct action *, int));
static int  ParseSwitch __P((struct action *, int *));
static int  ParseOnOff __P((struct action *, int *));
static int  ParseSaveStr __P((struct action *act, char **));
static int  ParseNum __P((struct action *act, int *));
static int  ParseWinNum __P((struct action *act, int *));
static int  ParseBase __P((struct action *act, char *, int *, int, char *));
static char *ParseChar __P((char *, char *));
static int  IsNum __P((char *, int));
static void Colonfin __P((char *, int, char *));
static void InputSelect __P((void));
static void InputSetenv __P((char *));
static void InputAKA __P((void));
static void AKAfin __P((char *, int, char *));
#ifdef COPY_PASTE
static void copy_reg_fn __P((char *, int, char *));
static void ins_reg_fn __P((char *, int, char *));
#endif
static void process_fn __P((char *, int, char *));
#ifdef PASSWORD
static void pass1 __P((char *, int, char *));
static void pass2 __P((char *, int, char *));
#endif
#ifdef POW_DETACH
static void pow_detach_fn __P((char *, int, char *));
#endif
static void digraph_fn __P((char *, int, char *));
#ifdef MAPKEYS
static int  StuffKey __P((int));
#endif



extern struct display *display, *displays;
extern struct win *fore, *console_window, *windows;
extern struct user *users;

extern char screenterm[], HostName[], version[];
extern struct NewWindow nwin_undef, nwin_default;
extern struct LayFuncs WinLf;
extern struct layer BlankLayer;

extern int Z0width, Z1width;
extern int real_uid, real_gid;

#ifdef NETHACK
extern int nethackflag;
#endif


struct win *wtab[MAXWIN];	/* window table, should be dynamic */

#ifdef MULTIUSER
extern char *multi;
extern int maxusercount;
#endif
char NullStr[] = "";		/* not really used */
#ifdef PASSWORD
int CheckPassword;
char Password[30];
#endif

struct plop plop_tab[MAX_PLOP_DEFS];

#ifndef PTYMODE
# define PTYMODE 0622
#endif

int TtyMode = PTYMODE;
int hardcopy_append = 0;
int all_norefresh = 0;

struct action ktab[256];	/* command key translation table */

#ifdef MAPKEYS
struct action umtab[KMAP_KEYS+KMAP_AKEYS+KMAP_EXT];
struct action dmtab[KMAP_KEYS+KMAP_AKEYS+KMAP_EXT];
struct action mmtab[KMAP_KEYS+KMAP_AKEYS+KMAP_EXT];

char *kmap_extras[KMAP_EXT];
int kmap_extras_fl[KMAP_EXT];

#endif


static const unsigned char digraphs[][3] = {
    {'~', '!', 161},	/* ¡ */
    {'!', '!', 161},	/* ¡ */
    {'c', '|', 162},	/* ¢ */
    {'$', '$', 163},	/* £ */
    {'o', 'x', 164},	/* ¤ */
    {'Y', '-', 165},	/* ¥ */
    {'|', '|', 166},	/* ¦ */
    {'p', 'a', 167},	/* § */
    {'"', '"', 168},	/* ¨ */
    {'c', 'O', 169},	/* © */
    {'a', '-', 170},	/* ª */
    {'<', '<', 171},	/* « */
    {'-', ',', 172},	/* ¬ */
    {'-', '-', 173},	/* ­ */
    {'r', 'O', 174},	/* ® */
    {'-', '=', 175},	/* ¯ */
    {'~', 'o', 176},	/* ° */
    {'+', '-', 177},	/* ± */
    {'2', '2', 178},	/* ² */
    {'3', '3', 179},	/* ³ */
    {'\'', '\'', 180},	/* ´ */
    {'j', 'u', 181},	/* µ */
    {'p', 'p', 182},	/* ¶ */
    {'~', '.', 183},	/* · */
    {',', ',', 184},	/* ¸ */
    {'1', '1', 185},	/* ¹ */
    {'o', '-', 186},	/* º */
    {'>', '>', 187},	/* » */
    {'1', '4', 188},	/* ¼ */
    {'1', '2', 189},	/* ½ */
    {'3', '4', 190},	/* ¾ */
    {'~', '?', 191},	/* ¿ */
    {'?', '?', 191},	/* ¿ */
    {'A', '`', 192},	/* À */
    {'A', '\'', 193},	/* Á */
    {'A', '^', 194},	/* Â */
    {'A', '~', 195},	/* Ã */
    {'A', '"', 196},	/* Ä */
    {'A', '@', 197},	/* Å */
    {'A', 'E', 198},	/* Æ */
    {'C', ',', 199},	/* Ç */
    {'E', '`', 200},	/* È */
    {'E', '\'', 201},	/* É */
    {'E', '^', 202},	/* Ê */
    {'E', '"', 203},	/* Ë */
    {'I', '`', 204},	/* Ì */
    {'I', '\'', 205},	/* Í */
    {'I', '^', 206},	/* Î */
    {'I', '"', 207},	/* Ï */
    {'D', '-', 208},	/* Ð */
    {'N', '~', 209},	/* Ñ */
    {'O', '`', 210},	/* Ò */
    {'O', '\'', 211},	/* Ó */
    {'O', '^', 212},	/* Ô */
    {'O', '~', 213},	/* Õ */
    {'O', '"', 214},	/* Ö */
    {'/', '\\', 215},	/* × */
    {'O', '/', 216},	/* Ø */
    {'U', '`', 217},	/* Ù */
    {'U', '\'', 218},	/* Ú */
    {'U', '^', 219},	/* Û */
    {'U', '"', 220},	/* Ü */
    {'Y', '\'', 221},	/* Ý */
    {'I', 'p', 222},	/* Þ */
    {'s', 's', 223},	/* ß */
    {'s', '"', 223},	/* ß */
    {'a', '`', 224},	/* à */
    {'a', '\'', 225},	/* á */
    {'a', '^', 226},	/* â */
    {'a', '~', 227},	/* ã */
    {'a', '"', 228},	/* ä */
    {'a', '@', 229},	/* å */
    {'a', 'e', 230},	/* æ */
    {'c', ',', 231},	/* ç */
    {'e', '`', 232},	/* è */
    {'e', '\'', 233},	/* é */
    {'e', '^', 234},	/* ê */
    {'e', '"', 235},	/* ë */
    {'i', '`', 236},	/* ì */
    {'i', '\'', 237},	/* í */
    {'i', '^', 238},	/* î */
    {'i', '"', 239},	/* ï */
    {'d', '-', 240},	/* ð */
    {'n', '~', 241},	/* ñ */
    {'o', '`', 242},	/* ò */
    {'o', '\'', 243},	/* ó */
    {'o', '^', 244},	/* ô */
    {'o', '~', 245},	/* õ */
    {'o', '"', 246},	/* ö */
    {':', '-', 247},	/* ÷ */
    {'o', '/', 248},	/* ø */
    {'u', '`', 249},	/* ù */
    {'u', '\'', 250},	/* ú */
    {'u', '^', 251},	/* û */
    {'u', '"', 252},	/* ü */
    {'y', '\'', 253},	/* ý */
    {'i', 'p', 254},	/* þ */
    {'y', '"', 255},	/* y", make poor lint happy*/
    {'"', '[', 196},	/* Ä */
    {'"', '\\', 214},	/* Ö */
    {'"', ']', 220},	/* Ü */
    {'"', '{', 228},	/* ä */
    {'"', '|', 246},	/* ö */
    {'"', '}', 252},	/* ü */
    {'"', '~', 223}	/* ß */
};


char *noargs[1];

void
InitKeytab()
{
  register unsigned int i;
#ifdef MAPKEYS
  char *argarr[2];
#endif

  for (i = 0; i < sizeof(ktab)/sizeof(*ktab); i++)
    {
      ktab[i].nr = RC_ILLEGAL;
      ktab[i].args = noargs;
    }
#ifdef MAPKEYS
  for (i = 0; i < KMAP_KEYS+KMAP_AKEYS+KMAP_EXT; i++)
    {
      umtab[i].nr = RC_ILLEGAL;
      umtab[i].args = noargs;
      dmtab[i].nr = RC_ILLEGAL;
      dmtab[i].args = noargs;
      mmtab[i].nr = RC_ILLEGAL;
      mmtab[i].args = noargs;
    }
  argarr[1] = 0;
  for (i = 0; i < NKMAPDEF; i++)
    {
      if (i + KMAPDEFSTART < T_CAPS)
	continue;
      if (i + KMAPDEFSTART >= T_CAPS + KMAP_KEYS)
	continue;
      if (kmapdef[i] == 0)
	continue;
      argarr[0] = kmapdef[i];
      dmtab[i + (KMAPDEFSTART - T_CAPS)].nr = RC_STUFF;
      dmtab[i + (KMAPDEFSTART - T_CAPS)].args = SaveArgs(argarr);
    }
  for (i = 0; i < NKMAPADEF; i++)
    {
      if (i + KMAPADEFSTART < T_CURSOR)
	continue;
      if (i + KMAPADEFSTART >= T_CURSOR + KMAP_AKEYS)
	continue;
      if (kmapadef[i] == 0)
	continue;
      argarr[0] = kmapadef[i];
      dmtab[i + (KMAPADEFSTART - T_CURSOR + KMAP_KEYS)].nr = RC_STUFF;
      dmtab[i + (KMAPADEFSTART - T_CURSOR + KMAP_KEYS)].args = SaveArgs(argarr);
    }
  for (i = 0; i < NKMAPMDEF; i++)
    {
      if (i + KMAPMDEFSTART < T_CAPS)
	continue;
      if (i + KMAPMDEFSTART >= T_CAPS + KMAP_KEYS)
	continue;
      if (kmapmdef[i] == 0)
	continue;
      argarr[0] = kmapmdef[i];
      argarr[1] = 0;
      mmtab[i + (KMAPMDEFSTART - T_CAPS)].nr = RC_STUFF;
      mmtab[i + (KMAPMDEFSTART - T_CAPS)].args = SaveArgs(argarr);
    }
#endif

  ktab['h'].nr = RC_HARDCOPY;
#ifdef BSDJOBS
  ktab['z'].nr = ktab[Ctrl('z')].nr = RC_SUSPEND;
#endif
  ktab['c'].nr = ktab[Ctrl('c')].nr = RC_SCREEN;
  ktab[' '].nr = ktab[Ctrl(' ')].nr =
    ktab['n'].nr = ktab[Ctrl('n')].nr = RC_NEXT;
  ktab['N'].nr = RC_NUMBER;
  ktab[Ctrl('h')].nr = ktab[0177].nr = ktab['p'].nr = ktab[Ctrl('p')].nr = RC_PREV;
  ktab['k'].nr = ktab[Ctrl('k')].nr = RC_KILL;
  ktab['l'].nr = ktab[Ctrl('l')].nr = RC_REDISPLAY;
  ktab['w'].nr = ktab[Ctrl('w')].nr = RC_WINDOWS;
  ktab['v'].nr = RC_VERSION;
  ktab[Ctrl('v')].nr = RC_DIGRAPH;
  ktab['q'].nr = ktab[Ctrl('q')].nr = RC_XON;
  ktab['s'].nr = ktab[Ctrl('s')].nr = RC_XOFF;
  ktab['t'].nr = ktab[Ctrl('t')].nr = RC_TIME;
  ktab['i'].nr = ktab[Ctrl('i')].nr = RC_INFO;
  ktab['m'].nr = ktab[Ctrl('m')].nr = RC_LASTMSG;
  ktab['A'].nr = RC_TITLE;
#if defined(UTMPOK) && defined(LOGOUTOK)
  ktab['L'].nr = RC_LOGIN;
#endif
  ktab[','].nr = RC_LICENSE;
  ktab['W'].nr = RC_WIDTH;
  ktab['.'].nr = RC_DUMPTERMCAP;
  ktab[Ctrl('\\')].nr = RC_QUIT;
  ktab['d'].nr = ktab[Ctrl('d')].nr = RC_DETACH;
  ktab['r'].nr = ktab[Ctrl('r')].nr = RC_WRAP;
  ktab['f'].nr = ktab[Ctrl('f')].nr = RC_FLOW;
  ktab['C'].nr = RC_CLEAR;
  ktab['Z'].nr = RC_RESET;
  ktab['H'].nr = RC_LOG;
  ktab['M'].nr = RC_MONITOR;
  ktab['?'].nr = RC_HELP;
  for (i = 0; i < ((MAXWIN < 10) ? MAXWIN : 10); i++)
    {
      char *args[2], arg1[10];
      args[0] = arg1;
      args[1] = 0;
      sprintf(arg1, "%d", i);
      ktab['0' + i].nr = RC_SELECT;
      ktab['0' + i].args = SaveArgs(args);
    }
  ktab[Ctrl('G')].nr = RC_VBELL;
  ktab[':'].nr = RC_COLON;
#ifdef COPY_PASTE
  ktab['['].nr = ktab[Ctrl('[')].nr = RC_COPY;
  {
    char *args[2];
    args[0] = ".";
    args[1] = NULL;
    ktab[']'].args = SaveArgs(args);
    ktab[Ctrl(']')].args = SaveArgs(args);
  }
  ktab[']'].nr = ktab[Ctrl(']')].nr = RC_PASTE;
  ktab['{'].nr = RC_HISTORY;
  ktab['}'].nr = RC_HISTORY;
  ktab['>'].nr = RC_WRITEBUF;
  ktab['<'].nr = RC_READBUF;
  ktab['='].nr = RC_REMOVEBUF;
  ktab['\''].nr = ktab['"'].nr = RC_SELECT; /* calling a window by name */
#endif
#ifdef POW_DETACH
  ktab['D'].nr = RC_POW_DETACH;
#endif
#ifdef LOCK
  ktab['x'].nr = ktab[Ctrl('x')].nr = RC_LOCKSCREEN;
#endif
  ktab['b'].nr = ktab[Ctrl('b')].nr = RC_BREAK;
  ktab['B'].nr = RC_POW_BREAK;
  ktab['_'].nr = RC_SILENCE;
  if (DefaultEsc >= 0)
    ktab[DefaultEsc].nr = RC_OTHER;
  if (DefaultMetaEsc >= 0)
    ktab[DefaultMetaEsc].nr = RC_META;
}

static void
ClearAction(act)
struct action *act;
{
  char **p;

  if (act->nr == RC_ILLEGAL)
    return;
  act->nr = RC_ILLEGAL;
  if (act->args == noargs)
    return;
  for (p = act->args; *p; p++)
    free(*p);
  free((char *)act->args);
  act->args = noargs;
}


#ifdef MAPKEYS

/*
 *  This ProcessInput just does the keybindings and passes
 *  everything on to ProcessInput2
 */

void
ProcessInput(ibuf, ilen)
char *ibuf;
int ilen;
{
  int ch, slen;
  unsigned char *s;
  int i, l;

  if (display == 0 || ilen == 0)
    return;
  slen = ilen;
  s = (unsigned char *)ibuf;
  while(ilen-- > 0)
    {
      ch = *s++;
      if (D_dontmap)
        D_dontmap = 0;
      else if (D_nseqs)
        for (;;)
	  {
	    if (*(unsigned char *)D_seqp != ch)
	      {
		l = *((unsigned char *)D_seqp + (KMAP_OFF - KMAP_SEQ));
		if (l)
		  {
		    D_seqp += sizeof(struct kmap) * l;
		    continue;
		  }
		if (D_seql)
		  {
		    D_seqp -= D_seql;
		    ProcessInput2(D_seqp, D_seql);
		    D_seql = 0;
		  }
		D_seqp = D_kmaps[0].seq;
		D_mapdefault = 0;
		break;
	      }
	    if (D_seql++ == 0)
	      {
		/* Finish old stuff */
		slen -= ilen + 1;
		ProcessInput2(ibuf, slen);
		D_seqruns = 0;
	      }
	    ibuf = (char *)s;
	    slen = ilen;
	    ch = -1;
	    if (*++D_seqp == 0)
	      { 
		i = (struct kmap *)(D_seqp - D_seql - KMAP_SEQ) - D_kmaps;
		debug1("Mapping #%d", i);
		i = D_kmaps[i].nr & ~KMAP_NOTIMEOUT;
		if (StuffKey(i))
		  {
		    D_seqp -= D_seql;
		    ProcessInput2(D_seqp, D_seql);
		  }
		if (display == 0)
		  return;
		D_seql = 0;
		D_seqp = D_kmaps[0].seq;
	      }
	    break;
	  }
    }
  ProcessInput2(ibuf, slen);
}

#else
# define ProcessInput2 ProcessInput
#endif


/*
 *  Here the screen escape commands are handled, the remaining
 *  chars are passed to the layer's input handler.
 */

void
ProcessInput2(ibuf, ilen)
char *ibuf;
int ilen;
{
  char *s;
  int ch, slen;

  while (display)
    {
      fore = D_fore;
      slen = ilen;
      s = ibuf;
      if (!D_ESCseen)
	{
	  while (ilen > 0)
	    {
	      if ((unsigned char)*s++ == D_user->u_Esc)
		break;
	      ilen--;
	    }
	  slen -= ilen;
	  while (slen)
	    Process(&ibuf, &slen);
	  if (--ilen == 0)
	    D_ESCseen = 1;
	}
      if (ilen <= 0)
        return;
      D_ESCseen = 0;
      ch = (unsigned char)*s;

      /* 
       * As users have different esc characters, but a common ktab[],
       * we fold back the users esc and meta-esc key to the Default keys
       * that can be looked up in the ktab[]. grmbl. jw.
       * FIXME: make ktab[] a per user thing.
       */
      if (ch == D_user->u_Esc) 
        ch = DefaultEsc;
      else if (ch == D_user->u_MetaEsc) 
        ch = DefaultMetaEsc;

      if (ch >= 0)
        DoAction(&ktab[ch], ch);
      ibuf = (char *)(s + 1);
      ilen--;
    }
}


int
FindCommnr(str)
char *str;
{
  int x, m, l = 0, r = RC_LAST;
  while (l <= r)
    {
      m = (l + r) / 2;
      x = strcmp(str, comms[m].name);
      if (x > 0)
	l = m + 1;
      else if (x < 0)
	r = m - 1;
      else
	return m;
    }
  return RC_ILLEGAL;
}

static int
CheckArgNum(nr, args)
int nr;
char **args;
{
  int i, n;
  static char *argss[] = {"no", "one", "two", "three"};

  n = comms[nr].flags & ARGS_MASK;
  for (i = 0; args[i]; i++)
    ;
  if (comms[nr].flags & ARGS_ORMORE)
    {
      if (i < n)
	{
	  Msg(0, "%s: %s: at least %s argument%s required", rc_name, comms[nr].name, argss[n], n != 1 ? "s" : "");
	  return -1;
	}
    }
  else if ((comms[nr].flags & ARGS_PLUSONE) && (comms[nr].flags & ARGS_PLUSTWO))
    {
      if (i != n && i != n + 1 && i != n + 2)
        {
	  Msg(0, "%s: %s: %s, %s or %s argument%s required", rc_name,
	      comms[nr].name, argss[n], argss[n + 1], argss[n + 2], 
	      n != 0 ? "s" : "");
	  return -1;
	}
    }
  else if (comms[nr].flags & ARGS_PLUSONE)
    {
      if (i != n && i != n + 1)
	{
	  Msg(0, "%s: %s: %s or %s argument%s required", rc_name, comms[nr].name, argss[n], argss[n + 1], n != 0 ? "s" : "");
          return -1;
	}
    }
  else if (comms[nr].flags & ARGS_PLUSTWO)
    {
      if (i != n && i != n + 2)
        {
	  Msg(0, "%s: %s: %s or %s argument%s required", rc_name, 
	      comms[nr].name, argss[n], argss[n + 2], n != 0 ? "s" : "");
	  return -1;
	}
    }
  else if (i != n)
    {
      Msg(0, "%s: %s: %s argument%s required", rc_name, comms[nr].name, argss[n], n != 1 ? "s" : "");
      return -1;
    }
  return 0;
}

/*ARGSUSED*/
static void
DoAction(act, key)
struct action *act;
int key;
{
  int nr = act->nr;
  char **args = act->args;
  struct win *p;
  int i, n, msgok;
  char *s;
  char ch;

  if (nr == RC_ILLEGAL)
    {
      debug1("key '%c': No action\n", key);
      return;
    }
  n = comms[nr].flags;
  if ((n & NEED_DISPLAY) && display == 0)
    {
      Msg(0, "%s: %s: display required", rc_name, comms[nr].name);
      return;
    }
  if ((n & NEED_FORE) && fore == 0)
    {
      Msg(0, "%s: %s: window required", rc_name, comms[nr].name);
      return;
    }
  if (CheckArgNum(nr, args))
    return;
#ifdef MULTIUSER
  if (multi && display)
    {
      if (AclCheckPermCmd(D_user, ACL_EXEC, &comms[nr]))
	return;
    }
#endif /* MULTIUSER */
  msgok = display && !*rc_name;
  switch(nr)
    {
    case RC_SELECT:
      if (!*args)
        InputSelect();
      else if (ParseWinNum(act, &n) == 0)
        SwitchWindow(n);
      break;
#ifdef AUTO_NUKE
    case RC_DEFAUTONUKE:
      if (ParseOnOff(act, &defautonuke) == 0 && msgok)
	Msg(0, "Default autonuke turned %s", defautonuke ? "on" : "off");
      if (display && *rc_name)
	D_auto_nuke = defautonuke;
      break;
    case RC_AUTONUKE:
      if (ParseOnOff(act, &D_auto_nuke) == 0 && msgok)
	Msg(0, "Autonuke turned %s", D_auto_nuke ? "on" : "off");
      break;
#endif
    case RC_DEFOBUFLIMIT:
      if (ParseNum(act, &defobuflimit) == 0 && msgok)
	Msg(0, "Default limit set to %d", defobuflimit);
      if (display && *rc_name)
	D_obufmax = defobuflimit;
      break;
    case RC_OBUFLIMIT:
      if (*args == 0)
	Msg(0, "Limit is %d, current buffer size is %d", D_obufmax, D_obuflen);
      else if (ParseNum(act, &D_obufmax) == 0 && msgok)
	Msg(0, "Limit set to %d", D_obufmax);
      break;
    case RC_DUMPTERMCAP:
      WriteFile(DUMP_TERMCAP);
      break;
    case RC_HARDCOPY:
      WriteFile(DUMP_HARDCOPY);
      break;
    case RC_LOG:
      n = fore->w_logfp ? 1 : 0;
      ParseSwitch(act, &n);
      LogToggle(n);
      break;
#ifdef BSDJOBS
    case RC_SUSPEND:
      Detach(D_STOP);
      break;
#endif
    case RC_NEXT:
      if (MoreWindows())
	SwitchWindow(NextWindow());
      break;
    case RC_PREV:
      if (MoreWindows())
	SwitchWindow(PreviousWindow());
      break;
    case RC_KILL:
      {
	char *name;

	n = fore->w_number;
#ifdef PSEUDOS
	if (fore->w_pwin)
	  {
	    FreePseudowin(fore);
#ifdef NETHACK
	    if (nethackflag)
	      Msg(0, "You have a sad feeling for a moment...");
	    else
#endif
	    Msg(0, "Filter removed.");
	    break;
	  }
#endif
	name = SaveStr(fore->w_title);
	KillWindow(fore);
#ifdef NETHACK
	if (nethackflag)
	  Msg(0, "You destroy poor window %d (%s).", n, name);
	else
#endif
	Msg(0, "Window %d (%s) killed.", n, name);
	if (name)
	  free(name);
	break;
      }
    case RC_QUIT:
      Finit(0);
      /* NOTREACHED */
    case RC_DETACH:
      Detach(D_DETACH);
      break;
#ifdef POW_DETACH
    case RC_POW_DETACH:
      if (key >= 0)
	{
	  static char buf[2];

	  buf[0] = key;
	  Input(buf, 1, INP_RAW, pow_detach_fn, NULL);
	}
      else
        Detach(D_POWER); /* detach and kill Attacher's parent */
      break;
#endif
    case RC_DEBUG:
#ifdef DEBUG
      if (!*args)
        {
	  if (dfp)
	    Msg(0, "debugging info is written to %s/", DEBUGDIR);
	  else
	    Msg(0, "debugging is currently off. Use 'debug on' to enable.");
	  break;
	}
      if (dfp)
        {
	  debug("debug: closing debug file.\n");
	  fflush(dfp);
	  fclose(dfp);
	  dfp = NULL;
	}
      if (strcmp("off", *args))
        opendebug(0, 1);
#else
      Msg(0, "Sorry, screen was compiled without -DDEBUG option.");
#endif
      break;
    case RC_ZOMBIE:
      {
        char ch2 = 0;

	if (!(s = *args))
	  {
	    ZombieKey_destroy = 0;
	    break;
	  }
	if (!(s = ParseChar(s, &ch)) || *s)
	  {
	    if (!s || !(s = ParseChar(s, &ch2)) || *s)
	      {
		Msg(0, "%s:zombie: one or two characters expected.", rc_name);
		break;
	      }
	  }
	ZombieKey_destroy = ch;
	ZombieKey_resurrect = ch2;
	break;
      }
    case RC_WALL:
      for (n = 0, s = *args; args[n]; n++)
        {
	  /* glue the vector together again. Brute force method. */
	  while (*s)
	    s++;
	  while (s < args[n+1])
	    *s++ = ' ';
	}
#ifdef MULTIUSER
      s = D_user->u_name;
#else
      s = D_usertty;
#endif
      display = NULL;	/* a message without display will cause a broadcast */
      Msg(0, "%s: %s", s, *args);
      break;
    case RC_AT:
      /* where this AT command comes from: */
#ifdef MULTIUSER
      s = SaveStr(D_user->u_name);
#else
      s = SaveStr(D_usertty);
#endif
      n = strlen(args[0]);
      if (n) n--;
      /*
       * the windows/displays loops are quite dangerous here, take extra
       * care not to trigger landmines. Things may appear/disappear while
       * we are walking along.
       */
      switch (args[0][n])
        {
	case '*':
	  {
	    struct display *nd;
	    struct user *u;

	    if (!n)
	      u = D_user;
	    else
	      for (u = users; u; u = u->u_next)
	        {
		  debug3("strncmp('%s', '%s', %d)\n", *args, u->u_name, n);
		  if (!strncmp(*args, u->u_name, n))
		    break;
	        }
	    debug1("at all displays of user %s\n", u->u_name);
	    for (display = displays; display; display = nd)
	      {
		nd = display->d_next;
		fore = D_fore;
	        if (D_user != u)
		  continue;
		debug1("AT display %s\n", D_usertty);
		DoCommand(args + 1);
		if (display)
		  Msg(0, "command from %s: %s %s", 
		      s, args[1], args[2] ? args[2] : "");
		display = NULL;
		fore = NULL;
	      }
	    free(s);
	    return;
	  }
	case '%':
	  {
	    struct display *nd;

	    debug1("at display matching '%s'\n", args[0]);
	    for (display = displays; display; display = nd)
	      {
	        nd = display->d_next;
		fore = D_fore;
	        if (strncmp(args[0], D_usertty, n) && 
		    (strncmp("/dev/", D_usertty, 5) || 
		     strncmp(args[0], D_usertty + 5, n)) &&
		    (strncmp("/dev/tty", D_usertty, 8) ||
		     strncmp(args[0], D_usertty + 8, n)))
		  continue;
		debug1("AT display %s\n", D_usertty);
		DoCommand(args + 1);
		if (display)
		  Msg(0, "command from %s: %s %s", 
		      s, args[1], args[2] ? args[2] : "");
		display = NULL;
		fore = NULL;
	      }
	    free(s);
	    return;
	  }
	case '#':
	  n--;
	  /* FALLTHROUGH */
	default:
	  {
	    struct win *nw;
	    int ch;

	    n++; 
	    ch = args[0][n];
	    args[0][n] = '\0';
	    if (!*args[0] || (i = WindowByNumber(args[0])) < 0)
	      {
	        args[0][n] = ch;      /* must restore string in case of bind */
	        /* try looping over titles */
		for (fore = windows; fore; fore = nw)
		  {
		    nw = fore->w_next;
		    if (strncmp(args[0], fore->w_title, n))
		      continue;
		    debug2("AT window %d(%s)\n", fore->w_number, fore->w_title);
		    /*
		     * consider this a bug or a feature: 
		     * while looping through windows, we have fore AND
		     * display context. This will confuse users who try to 
		     * set up loops inside of loops, but often allows to do 
		     * what you mean, even when you adress your context wrong.
		     */
		    i = 0;
		    if (fore->w_display)
		      display = fore->w_display;
		    DoCommand(args + 1);	/* may destroy our display */
		    if ((fore->w_display))
		      {
		        display = fore->w_display;
		        Msg(0, "command from %s: %s %s", 
			    s, args[1], args[2] ? args[2] : "");
		      }
		  }
		display = NULL;
		fore = NULL;
		if (i < 0)
		  Msg(0, "%s: at '%s': no such window.\n", rc_name, args[0]);
		free(s);
		return;
	      }
	    else if (i < MAXWIN && (fore = wtab[i]))
	      {
	        args[0][n] = ch;      /* must restore string in case of bind */
	        debug2("AT window %d (%s)\n", fore->w_number, fore->w_title);
		if (fore->w_display)
		  display = fore->w_display;
		DoCommand(args + 1);
		if ((display = fore->w_display))
		  Msg(0, "command from %s: %s %s", 
		      s, args[1], args[2] ? args[2] : "");
		display = NULL;
		fore = NULL;
		free(s);
		return;
	      }
	  }
	}
      Msg(0, "%s: at [identifier][%%|*|#] command [args]", rc_name);
      free(s);
      break;
#ifdef COPY_PASTE
    case RC_READREG:
      /* 
       * Without arguments we prompt for a destination register.
       * It will receive the copybuffer contents.
       * This is not done by RC_PASTE, as we prompt for source (not dest) 
       * there.
       */
      if ((s = *args) == NULL)
	{
	  Input("Copy to register:", 1, INP_RAW, copy_reg_fn, NULL);
	  break;
	}
      if (((s = ParseChar(s, &ch)) == NULL) || *s)
	{
	  Msg(0, "%s: copyreg: character, ^x, or (octal) \\032 expected.",
	      rc_name);
	  break;
	}
      /* 
       * With two arguments we *really* read register contents from file
       */
      if (args[1])
        {
	  if ((s = ReadFile(args[1], &n)))
	    {
	      struct plop *pp = plop_tab + (int)(unsigned char)ch;

	      if (pp->buf)
		free(pp->buf);
	      pp->buf = s;
	      pp->len = n;
	    }
	}
      else
        /*
	 * with one argument we copy the copybuffer into a specified register
	 * This could be done with RC_PASTE too, but is here to be consistent
	 * with the zero argument call.
	 */
        copy_reg_fn(&ch, 0, NULL);
      break;
#endif
    case RC_REGISTER:
      if ((s = ParseChar(*args, &ch)) == NULL || *s)
	Msg(0, "%s: register: character, ^x, or (octal) \\032 expected.",
	    rc_name);
      else
	{
	  struct plop *plp = plop_tab + (int)(unsigned char)ch;

	  if (plp->buf)
	    free(plp->buf);
	  plp->buf = SaveStr(args[1]);
	  plp->len = strlen(plp->buf);
	}
      break;
    case RC_PROCESS:
      if ((s = *args) == NULL)
	{
	  Input("Process register:", 1, INP_RAW, process_fn, NULL);
	  break;
	}
      if ((s = ParseChar(s, &ch)) == NULL || *s)
	{
	  Msg(0, "%s: process: character, ^x, or (octal) \\032 expected.",
	      rc_name);
	  break;
	}
      process_fn(&ch, 0, NULL);
      break;
    case RC_STUFF:
      s = *args;
      n = strlen(s);
      while(n)
        Process(&s, &n);
      break;
    case RC_REDISPLAY:
      Activate(-1);
      break;
    case RC_WINDOWS:
      ShowWindows();
      break;
    case RC_VERSION:
      Msg(0, "screen %s", version);
      break;
    case RC_TIME:
      ShowTime();
      break;
    case RC_INFO:
      ShowInfo();
      break;
    case RC_COMMAND:
      if (!D_ESCseen)
	{
	  D_ESCseen = 1;
	  break;
	}
      D_ESCseen = 0;
      /* FALLTHROUGH */
    case RC_OTHER:
      if (MoreWindows())
	SwitchWindow(D_other ? D_other->w_number : NextWindow());
      break;
    case RC_META:
      if (D_user->u_Esc < 0)
        break;
      ch = D_user->u_Esc;
      s = &ch;
      n = 1;
      Process(&s, &n);
      break;
    case RC_XON:
      ch = Ctrl('q');
      s = &ch;
      n = 1;
      Process(&s, &n);
      break;
    case RC_XOFF:
      ch = Ctrl('s');
      s = &ch;
      n = 1;
      Process(&s, &n);
      break;
    case RC_POW_BREAK:
    case RC_BREAK:
      n = 0;
      if (*args && ParseNum(act, &n))
	break;
      SendBreak(fore, n, nr == RC_POW_BREAK);
      break;
#ifdef LOCK
    case RC_LOCKSCREEN:
      Detach(D_LOCK);
      break;
#endif
    case RC_WIDTH:
      if (*args)
	{
	  if (ParseNum(act, &n))
	    break;
	}
      else
	{
	  if (D_width == Z0width)
	    n = Z1width;
	  else if (D_width == Z1width)
	    n = Z0width;
	  else if (D_width > (Z0width + Z1width) / 2)
	    n = Z0width;
	  else
	    n = Z1width;
	}
      if (n <= 0)
        {
	  Msg(0, "Illegal width");
	  break;
	}
      if (n == D_width)
	break;
      if (ResizeDisplay(n, D_height) == 0)
	{
	  DoResize(D_width, D_height);
	  Activate(D_fore ? D_fore->w_norefresh : 0);
	}
      else
	Msg(0, "Your termcap does not specify how to change the terminal's width to %d.", n);
      break;
    case RC_HEIGHT:
      if (*args)
	{
	  if (ParseNum(act, &n))
	    break;
	}
      else
	{
#define H0height 42
#define H1height 24
	  if (D_height == H0height)
	    n = H1height;
	  else if (D_height == H1height)
	    n = H0height;
	  else if (D_height > (H0height + H1height) / 2)
	    n = H0height;
	  else
	    n = H1height;
	}
      if (n <= 0)
        {
	  Msg(0, "Illegal height");
	  break;
	}
      if (n == D_height)
	break;
      if (ResizeDisplay(D_width, n) == 0)
	{
	  DoResize(D_width, D_height);
	  Activate(D_fore ? D_fore->w_norefresh : 0);
	}
      else
	Msg(0, "Your termcap does not specify how to change the terminal's height to %d.", n);
      break;
    case RC_AKA:
    case RC_TITLE:
      if (*args == 0)
	InputAKA();
      else
	ChangeAKA(fore, *args, 20);
      break;
    case RC_COLON:
      Input(":", 100, INP_COOKED, Colonfin, NULL);
      if (*args && **args)
	{
	  s = *args;
	  n = strlen(s);
	  Process(&s, &n);
	}
      break;
    case RC_LASTMSG:
      if (D_status_lastmsg)
	Msg(0, "%s", D_status_lastmsg);
      break;
    case RC_SCREEN:
      DoScreen("key", args);
      break;
    case RC_WRAP:
      if (ParseSwitch(act, &fore->w_wrap) == 0 && msgok)
        Msg(0, "%cwrap", fore->w_wrap ? '+' : '-');
      break;
    case RC_FLOW:
      if (*args)
	{
	  if (args[0][0] == 'a')
	    {
	      fore->w_flow = (fore->w_flow & FLOW_AUTO) ? FLOW_AUTOFLAG |FLOW_AUTO|FLOW_NOW : FLOW_AUTOFLAG;
	    }
	  else
	    {
	      if (ParseOnOff(act, &n))
		break;
	      fore->w_flow = (fore->w_flow & FLOW_AUTO) | n;
	    }
	}
      else
	{
	  if (fore->w_flow & FLOW_AUTOFLAG)
	    fore->w_flow = (fore->w_flow & FLOW_AUTO) | FLOW_NOW;
	  else if (fore->w_flow & FLOW_NOW)
	    fore->w_flow &= ~FLOW_NOW;
	  else
	    fore->w_flow = fore->w_flow ? FLOW_AUTOFLAG|FLOW_AUTO|FLOW_NOW : FLOW_AUTOFLAG;
	}
      SetFlow(fore->w_flow & FLOW_NOW);
      if (msgok)
	Msg(0, "%cflow%s", (fore->w_flow & FLOW_NOW) ? '+' : '-',
	    (fore->w_flow & FLOW_AUTOFLAG) ? "(auto)" : "");
      break;
    case RC_DEFWRITELOCK:
      if (args[0][0] == 'a')
	nwin_default.wlock = WLOCK_AUTO;
      else
	{
	  if (ParseOnOff(act, &n))
	    break;
	  nwin_default.wlock = n ? WLOCK_ON : WLOCK_OFF;
	}
      break;
    case RC_WRITELOCK:
#ifdef MULTIUSER
      if (*args)
	{
	  if (args[0][0] == 'a')
	    {
	      fore->w_wlock = WLOCK_AUTO;
	    }
	  else
	    {
	      if (ParseOnOff(act, &n))
		break;
	      fore->w_wlock = n ? WLOCK_ON : WLOCK_OFF;
	    }
	  /* 
	   * user may have permission to change the writelock setting, 
	   * but he may never aquire the lock himself without write permission
	   */
	  if (!AclCheckPermWin(D_user, ACL_WRITE, fore))
	    fore->w_wlockuser = D_user;
	}
#endif
      Msg(0, "writelock %s", (fore->w_wlock == WLOCK_AUTO) ? "auto" :
	  ((fore->w_wlock == WLOCK_OFF) ? "off" : "on"));
      break;
    case RC_CLEAR:
      if (fore->w_state == LIT)
	WriteString(fore, "\033[H\033[J", 6);
      break;
    case RC_RESET:
      fore->w_state = LIT;
      WriteString(fore, "\033c", 2);
      break;
    case RC_MONITOR:
      n = fore->w_monitor == MON_ON;
      if (ParseSwitch(act, &n))
	break;
      if (n)
	{
	  fore->w_monitor = MON_ON;
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "You feel like someone is watching you...");
	  else
#endif
	    Msg(0, "Window %d (%s) is now being monitored for all activity.", 
		fore->w_number, fore->w_title);
	}
      else
	{
	  fore->w_monitor = MON_OFF;
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "You no longer sense the watcher's presence.");
	  else
#endif
	    Msg(0, "Window %d (%s) is no longer being monitored for activity.", 
		fore->w_number, fore->w_title);
	}
      break;
    case RC_DISPLAYS:
      display_displays();
      break;
    case RC_HELP:
      display_help();
      break;
    case RC_LICENSE:
      display_copyright();
      break;
#ifdef COPY_PASTE
    case RC_COPY:
      if (D_layfn != &WinLf)
	{
	  Msg(0, "Must be on a window layer");
	  break;
	}
      MarkRoutine();
      break;
    case RC_HISTORY:
      {
        static char *pasteargs[] = {".", 0};
	if (D_layfn != &WinLf)
	  {
	    Msg(0, "Must be on a window layer");
	    break;
	  }
	if (GetHistory() == 0)
	  break;
	if (D_user->u_copybuffer == NULL)
	  break;
	args = pasteargs;
      }
      /*FALLTHROUGH*/
    case RC_PASTE:
      {
        char *ss, *dbuf, dch;
        int l = 0;

	/*
	 * without args we prompt for one(!) register to be pasted in the window
	 */
	if ((s = *args) == NULL)
	  {
	    Input("Paste from register:", 1, INP_RAW, ins_reg_fn, NULL);
	    break;
	  }
	/*	
	 * with two arguments we paste into a destination register
	 * (no window needed here).
	 */
	if (args[1] && ((s = ParseChar(args[1], &dch)) == NULL || *s))
	  {
	    Msg(0, "%s: paste destination: character, ^x, or (octal) \\032 expected.",
		rc_name);
	    break;
	  }
	/*
	 * measure length of needed buffer 
	 */
        for (ss = s = *args; (ch = *ss); ss++)
          {
	    if (ch == '.')
	      {
	      	if (display)
		  l += D_user->u_copylen;
	      }
	    else
              l += plop_tab[(int)(unsigned char)ch].len;
          }
        if (l == 0)
	  {
#ifdef NETHACK
	    if (nethackflag)
	      Msg(0, "Nothing happens.");
	    else
#endif
	    Msg(0, "empty buffer");
	    break;
	  }
	/*
	 * shortcut: 
	 * if there is only one source and the destination is a window, then
	 * pass a pointer rather than duplicating the buffer.
	 */
        if (s[1] == 0 && args[1] == 0)
          {
	    if (fore == 0)
	      break;
	    fore->w_pasteptr = 0;
	    fore->w_pastelen = 0;
	    if (fore->w_pastebuf)
	      free(fore->w_pastebuf);
	    fore->w_pastebuf = 0;	/* this flags we only have a pointer */
            if (*s == '.')
	      fore->w_pasteptr = D_user->u_copybuffer;
            else
	      fore->w_pasteptr = plop_tab[(int)(unsigned char)*s].buf;
	    fore->w_pastelen = l;
	    break;
          }
	/*
	 * if no shortcut, we construct a buffer
	 */
        if ((dbuf = (char *)malloc(l)) == 0)
          {
	    Msg(0, strnomem);
	    break;
          }
        l = 0;
	/*
	 * concatenate all sources into our own buffer, copy buffer is
	 * special and is skipped if no display exists.
	 */
        for (ss = s; (ch = *ss); ss++)
          {
	    if (ch == '.')
	      {
	        if (display == 0)
		  continue;
		bcopy(D_user->u_copybuffer, dbuf + l, D_user->u_copylen);
                l += D_user->u_copylen;
              }
	    else
	      {
		bcopy(plop_tab[(int)(unsigned char)ch].buf, dbuf + l, plop_tab[(int)(unsigned char)ch].len);
                l += plop_tab[(int)(unsigned char)ch].len;
              }
          }
	/*
	 * when called with one argument we paste our buffer into the window 
	 */
	if (args[1] == 0)
	  {
	    if (fore == 0)
	      {
		free(dbuf); /* no window? zap our buffer */
		break;
	      }
	    if (fore->w_pastebuf)
	      free(fore->w_pastebuf);
	    fore->w_pastebuf = dbuf;
	    fore->w_pasteptr = fore->w_pastebuf;
	    fore->w_pastelen = l;
	  }
	else
	  {
	    /*
	     * we have two arguments, the second is already in dch.
	     * use this as destination rather than the window.
	     */
	    if (dch == '.')
	      {
		if (display == 0)
		  {
		    free(dbuf);
		    break;
		  }
	        if (D_user->u_copybuffer != NULL)
	          UserFreeCopyBuffer(D_user);
		D_user->u_copybuffer = dbuf;
		D_user->u_copylen = l;
	      }
	    else
	      {
		struct plop *pp = plop_tab + (int)(unsigned char)dch;

		if (pp->buf)
		  free(pp->buf);
		pp->buf = dbuf;
		pp->len = l;
	      }
	  }
        break;
      }
    case RC_WRITEBUF:
      if (D_user->u_copybuffer == NULL)
	{
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "Nothing happens.");
	  else
#endif
	  Msg(0, "empty buffer");
	  break;
	}
      WriteFile(DUMP_EXCHANGE);
      break;
    case RC_READBUF:
      if ((s = ReadFile(BufferFile, &n)))
	{
	  if (D_user->u_copybuffer)
	    UserFreeCopyBuffer(D_user);
	  D_user->u_copylen = n;
	  D_user->u_copybuffer = s;
	}
      break;
    case RC_REMOVEBUF:
      KillBuffers();
      break;
#endif				/* COPY_PASTE */
    case RC_ESCAPE:
      if (ParseEscape(display ? D_user : users, *args))
	{
	  Msg(0, "%s: two characters required after escape.", rc_name);
	  break;
	}
      /* Change defescape if only one user. This is because we only
       * have one ktab.
       */
      if (users->u_next)
        break;
      /* FALLTHROUGH */
    case RC_DEFESCAPE:
      if (ParseEscape((struct user *)0, *args))
	{
	  Msg(0, "%s: two characters required after defescape.", rc_name);
	  break;
	}
#ifdef MAPKEYS
      CheckEscape();
#endif
      break;
    case RC_CHDIR:
      s = *args ? *args : home;
      if (chdir(s) == -1)
	Msg(errno, "%s", s);
      break;
    case RC_SHELL:
      if (ParseSaveStr(act, &ShellProg) == 0)
        ShellArgs[0] = ShellProg;
      break;
    case RC_HARDCOPYDIR:
      (void)ParseSaveStr(act, &hardcopydir);
      break;
    case RC_LOGFILE:
      if (*args)
	if (ParseSaveStr(act, &screenlogfile) || !msgok)
	  break;
      Msg(0, "logfile is '%s'", screenlogfile);
      break;
    case RC_SHELLTITLE:
    case RC_SHELLAKA:
      (void)ParseSaveStr(act, &nwin_default.aka);
      break;
    case RC_SLEEP:
    case RC_TERMCAP:
    case RC_TERMINFO:
    case RC_TERMCAPINFO:
      break;			/* Already handled */
    case RC_TERM:
      s = NULL;
      if (ParseSaveStr(act, &s))
	break;
      if (strlen(s) >= 20)
	{
	  Msg(0,"%s: term: argument too long ( < 20)", rc_name);
	  free(s);
	  break;
	}
      strcpy(screenterm, s);
      free(s);
      debug1("screenterm set to %s\n", screenterm);
      MakeTermcap(display == 0);
      debug("new termcap made\n");
      break;
    case RC_ECHO:
      if (msgok)
	{
	  /*
	   * D_user typed ^A:echo... well, echo isn't FinishRc's job,
	   * but as he wanted to test us, we show good will
	   */
	  if (*args && (args[1] == 0 || (strcmp(args[1], "-n") == 0 && args[2] == 0)))
	    Msg(0, "%s", args[1] ? args[1] : *args);
	  else
 	    Msg(0, "%s: 'echo [-n] \"string\"' expected.", rc_name);
	}
      break;
    case RC_BELL:
    case RC_BELL_MSG:
      if (*args == 0)
        {
	  char buf[256];
          AddXChars(buf, sizeof(buf), BellString);
	  Msg(0, "bell_msg is '%s'", buf);
	  break;
        }
      (void)ParseSaveStr(act, &BellString);
      break;
#ifdef COPY_PASTE
    case RC_BUFFERFILE:
      if (*args == 0)
	BufferFile = SaveStr(DEFAULT_BUFFERFILE);
      else if (ParseSaveStr(act, &BufferFile))
        break;
      if (msgok)
        Msg(0, "Bufferfile is now '%s'\n", BufferFile);
      break;
#endif
    case RC_ACTIVITY:
      (void)ParseSaveStr(act, &ActivityString);
      break;
#ifdef POW_DETACH
    case RC_POW_DETACH_MSG:
      if (*args == 0)
        {
	  char buf[256];
          AddXChars(buf, sizeof(buf), PowDetachString);
	  Msg(0, "pow_detach_msg is '%s'", buf);
	  break;
        }
      (void)ParseSaveStr(act, &PowDetachString);
      break;
#endif
#if defined(UTMPOK) && defined(LOGOUTOK)
    case RC_LOGIN:
      n = fore->w_slot != (slot_t)-1;
      if (ParseSwitch(act, &n) == 0)
        SlotToggle(n);
      break;
    case RC_DEFLOGIN:
      (void)ParseOnOff(act, &nwin_default.lflag);
      break;
#endif
    case RC_DEFFLOW:
      if (args[0] && args[1] && args[1][0] == 'i')
	{
	  iflag = 1;
	  if ((intrc == VDISABLE) && (origintrc != VDISABLE))
	    {
#if defined(TERMIO) || defined(POSIX)
	      intrc = D_NewMode.tio.c_cc[VINTR] = origintrc;
	      D_NewMode.tio.c_lflag |= ISIG;
#else /* TERMIO || POSIX */
	      intrc = D_NewMode.m_tchars.t_intrc = origintrc;
#endif /* TERMIO || POSIX */

	      if (display)
		SetTTY(D_userfd, &D_NewMode);
	    }
	}
      if (args[0] && args[0][0] == 'a')
	nwin_default.flowflag = FLOW_AUTOFLAG;
      else
	(void)ParseOnOff(act, &nwin_default.flowflag);
      break;
    case RC_DEFWRAP:
      (void)ParseOnOff(act, &nwin_default.wrap);
      break;
    case RC_DEFC1:
      (void)ParseOnOff(act, &nwin_default.c1);
      break;
    case RC_DEFGR:
      (void)ParseOnOff(act, &nwin_default.gr);
      break;
    case RC_DEFMONITOR:
      if (ParseOnOff(act, &n) == 0)
        nwin_default.monitor = (n == 0) ? MON_OFF : MON_ON;
      break;
    case RC_HARDSTATUS:
      RemoveStatus();
      (void)ParseSwitch(act, &use_hardstatus);
      break;
    case RC_CONSOLE:
      n = (console_window != 0);
      if (ParseSwitch(act, &n))
        break;
      if (TtyGrabConsole(fore->w_ptyfd, n, rc_name))
	break;
      if (n == 0)
	  Msg(0, "%s: releasing console %s", rc_name, HostName);
      else if (console_window)
	  Msg(0, "%s: stealing console %s from window %d (%s)", rc_name, 
	      HostName, console_window->w_number, console_window->w_title);
      else
	  Msg(0, "%s: grabbing console %s", rc_name, HostName);
      console_window = n ? fore : 0;
      break;
    case RC_ALLPARTIAL:
      if (ParseOnOff(act, &all_norefresh))
	break;
      if (!all_norefresh && fore)
	Activate(-1);
      if (msgok)
        Msg(0, all_norefresh ? "No refresh on window change!\n" :
			       "Window specific refresh\n");
      break;
    case RC_PARTIAL:
      (void)ParseSwitch(act, &n);
      fore->w_norefresh = n;
      break;
    case RC_VBELL:
      if (ParseSwitch(act, &visual_bell) || !msgok)
        break;
      if (visual_bell == 0)
	{
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "Suddenly you can't see your bell!");
	  else
#endif
	  Msg(0, "switched to audible bell.");
	}
      else
	{
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "Your bell is no longer invisible.");
	  else
#endif
	  Msg(0, "switched to visual bell.");
	}
      break;
    case RC_VBELLWAIT:
      if (ParseNum(act, &VBellWait) == 0 && msgok)
        Msg(0, "vbellwait set to %d seconds", VBellWait);
      break;
    case RC_MSGWAIT:
      if (ParseNum(act, &MsgWait) == 0 && msgok)
        Msg(0, "msgwait set to %d seconds", MsgWait);
      break;
    case RC_MSGMINWAIT:
      if (ParseNum(act, &MsgMinWait) == 0 && msgok)
        Msg(0, "msgminwait set to %d seconds", MsgMinWait);
      break;
    case RC_SILENCEWAIT:
      if ((ParseNum(act, &SilenceWait) == 0) && msgok)
        {
	  if (SilenceWait < 1)
	    SilenceWait = 1;
	  for (p = windows; p; p = p->w_next)
	    if (p->w_tstamp.seconds)
	      p->w_tstamp.seconds = SilenceWait;
	  Msg(0, "silencewait set to %d seconds", SilenceWait);
	}
      break;
    case RC_NUMBER:
      if (*args == 0)
        Msg(0, "This is window %d (%s).\n", fore->w_number, fore->w_title);
      else
        {
	  int old = fore->w_number;

	  if (ParseNum(act, &n) || n >= MAXWIN)
	    break;
	  p = wtab[n];
	  wtab[n] = fore;
	  fore->w_number = n;
	  wtab[old] = p;
	  if (p)
	    p->w_number = old;
#ifdef MULTIUSER
	  AclWinSwap(old, n);
#endif
  /* yucc */
	  if (fore->w_hstatus)
	    {
	      display = fore->w_display;
	      if (display)
	        RefreshStatus();
	    }
	}
      break;
    case RC_SILENCE:
      n = fore->w_tstamp.seconds != 0;
      i = SilenceWait;
      if (args[0] && 
          (args[0][0] == '-' || (args[0][0] >= '0' && args[0][0] <= '9')))
        {
	  if (ParseNum(act, &i))
	    break;
	  n = i;
	}
      else if (ParseSwitch(act, &n))
        break;
      if (n)
        {
#ifdef MULTIUSER
	  if (display)	/* we tell only this user */
	    ACLBYTE(fore->w_lio_notify, D_user->u_id) |= ACLBIT(D_user->u_id);
	  else
	    for (i = 0; i < maxusercount; i++)
	      ACLBYTE(fore->w_mon_notify, i) |= ACLBIT(i);
	  if (!fore->w_tstamp.seconds)
#endif
	  fore->w_tstamp.lastio = time(0);
	  fore->w_tstamp.seconds = i;
	  if (!msgok)
	    break;
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "You feel like someone is waiting for %d sec. silence...",
	        fore->w_tstamp.seconds);
	  else
#endif
	    Msg(0, "Window %d (%s) is now being monitored for %d sec. silence.",
	    	fore->w_number, fore->w_title, fore->w_tstamp.seconds);
	}
      else
        {
#ifdef MULTIUSER
	  if (display) /* we remove only this user */
	    ACLBYTE(fore->w_lio_notify, D_user->u_id) 
	      &= ~ACLBIT(D_user->u_id);
	  else
	    for (i = 0; i < maxusercount; i++)
	      ACLBYTE(fore->w_lio_notify, i) &= ~ACLBIT(i);
	  for (i = maxusercount - 1; i >= 0; i--)
	    if (ACLBYTE(fore->w_lio_notify, i))
	      break;
	  if (i < 0)
#endif
	    {
	      fore->w_tstamp.lastio = (time_t)0;
	      fore->w_tstamp.seconds = 0;
	    }
	  if (!msgok)
	    break;
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "You no longer sense the watcher's silence.");
	  else
#endif
	    Msg(0, "Window %d (%s) is no longer being monitored for silence.", 
		fore->w_number, fore->w_title);
	}
      break;
#ifdef COPY_PASTE
    case RC_DEFSCROLLBACK:
      (void)ParseNum(act, &nwin_default.histheight);
      break;
    case RC_SCROLLBACK:
      (void)ParseNum(act, &n);
      ChangeWindowSize(fore, fore->w_width, fore->w_height, n);
      if (msgok)
	Msg(0, "scrollback set to %d", fore->w_histheight);
      break;
#endif
    case RC_SESSIONNAME:
      if (*args == 0)
	Msg(0, "This session is named '%s'\n", SockName);
      else
	{
	  char buf[MAXPATHLEN];

	  s = NULL;
	  if (ParseSaveStr(act, &s))
	    break;
	  if (!*s || strlen(s) + (SockName - SockPath) > MAXPATHLEN - 13)
	    {
	      Msg(0, "%s: bad session name '%s'\n", rc_name, s);
	      free(s);
	      break;
	    }
	  strncpy(buf, SockPath, SockName - SockPath);
	  sprintf(buf + (SockName - SockPath), "%d.%s", (int)getpid(), s); 
	  free(s);
	  if ((access(buf, F_OK) == 0) || (errno != ENOENT))
	    {
	      Msg(0, "%s: inappropriate path: '%s'.", rc_name, buf);
	      break;
	    }
	  if (rename(SockPath, buf))
	    {
	      Msg(errno, "%s: failed to rename(%s, %s)", rc_name, SockPath, buf);
	      break;
	    }
	  debug2("rename(%s, %s) done\n", SockPath, buf);
	  strcpy(SockPath, buf);
	  MakeNewEnv();
	}
      break;
    case RC_SETENV:
      if (!args[0] || !args[1])
        {
	  debug1("RC_SETENV arguments missing: %s\n", args[0] ? args[0] : "");
          InputSetenv(args[0]);
	}
      else
#ifndef USESETENV
	{
	  char *buf;
	  int l;

	  if ((buf = (char *)malloc((l = strlen(args[0])) + 
				     strlen(args[1]) + 2)) == NULL)
	    {
	      Msg(0, strnomem);
	      break;
	    }
	  strcpy(buf, args[0]);
	  buf[l] = '=';
	  strcpy(buf + l + 1, args[1]);
	  putenv(buf);
# ifdef NEEDPUTENV
	  /*
	   * we use our own putenv(), knowing that it does a malloc()
	   * the string space, we can free our buf now. 
	   */
	  free(buf);
# else /* NEEDSETENV */
	  /*
	   * For all sysv-ish systems that link a standard putenv() 
	   * the string-space buf is added to the environment and must not
	   * be freed, or modified.
	   * We are sorry to say that memory is lost here, when setting 
	   * the same variable again and again.
	   */
# endif /* NEEDSETENV */
	}
#else /* USESETENV */
# if defined(linux) || defined(__convex__) || (BSD >= 199103)
      setenv(args[0], args[1], 0);
# else
      setenv(args[0], args[1]);
# endif /* linux || convex || BSD >= 199103 */
#endif /* USESETENV */
      MakeNewEnv();
      break;
    case RC_UNSETENV:
      unsetenv(*args);
      MakeNewEnv();
      break;
    case RC_SLOWPASTE:
      if (ParseNum(act, &slowpaste) == 0 && msgok)
	Msg(0, "slowpaste set to %d milliseconds", slowpaste);
      break;
#ifdef COPY_PASTE
    case RC_MARKKEYS:
      s = NULL;
      if (ParseSaveStr(act, &s))
        break;
      if (CompileKeys(s, mark_key_tab))
	{
	  Msg(0, "%s: markkeys: syntax error.", rc_name);
	  free(s);
	  break;
	}
      debug1("markkeys %s\n", *args);
      free(s);
      break;
    case RC_PASTEFONT:
      if (ParseSwitch(act, &pastefont) == 0 && msgok)
        Msg(0, "Will %spaste font settings", pastefont ? "" : "not ");
      break;
    case RC_CRLF:
      (void)ParseSwitch(act, &join_with_cr);
      break;
#endif
#ifdef NETHACK
    case RC_NETHACK:
      (void)ParseOnOff(act, &nethackflag);
      break;
#endif
    case RC_HARDCOPY_APPEND:
      (void)ParseOnOff(act, &hardcopy_append);
      break;
    case RC_VBELL_MSG:
      if (*args == 0)
        {
	  char buf[256];
          AddXChars(buf, sizeof(buf), VisualBellString);
	  Msg(0, "vbell_msg is '%s'", buf);
	  break;
        }
      (void)ParseSaveStr(act, &VisualBellString);
      debug1(" new vbellstr '%s'\n", VisualBellString);
      break;
    case RC_DEFMODE:
      if (ParseBase(act, *args, &n, 8, "oct"))
        break;
      if (n < 0 || n > 0777)
	{
	  Msg(0, "%s: mode: Invalid tty mode %o", rc_name, n);
          break;
	}
      TtyMode = n;
      if (msgok)
	Msg(0, "Ttymode set to %03o", TtyMode);
      break;
    case RC_AUTODETACH:
      (void)ParseOnOff(act, &auto_detach);
      break;
    case RC_STARTUP_MESSAGE:
      (void)ParseOnOff(act, &default_startup);
      break;
#ifdef PASSWORD
    case RC_PASSWORD:
      CheckPassword = 1;
      if (*args)
	{
	  strncpy(Password, *args, sizeof(Password) - 1);
	  if (!strcmp(Password, "none"))
	    CheckPassword = 0;
	}
      else
	{
	  if (display == 0)
	    {
	      debug("prompting for password on no display???\n");
	      break;
	    }
	  Input("New screen password:", sizeof(Password) - 1, INP_NOECHO,
	        pass1, NULL);
	}
      break;
#endif				/* PASSWORD */
    case RC_BIND:
      if ((s = ParseChar(*args, &ch)) == NULL || *s)
	{
	  Msg(0, "%s: bind: character, ^x, or (octal) \\032 expected.",
	      rc_name);
	  break;
	}
      n = (unsigned char)ch;
      ClearAction(&ktab[n]);
      if (args[1])
	{
	  if ((i = FindCommnr(args[1])) == RC_ILLEGAL)
	    {
	      Msg(0, "%s: bind: unknown command '%s'", rc_name, args[1]);
	      break;
	    }
	  if (CheckArgNum(i, args + 2))
	    break;
	  ktab[n].nr = i;
	  if (args[2])
	    ktab[n].args = SaveArgs(args + 2);
	}
      break;
#ifdef MAPKEYS
    case RC_BINDKEY:
	{
	  struct action *newact;
          int newnr, fl = 0, kf = 0, af = 0, df = 0, mf = 0;
	  struct display *odisp = display;
	  int used = 0;

	  for (; *args && **args == '-'; args++)
	    {
	      if (strcmp(*args, "-t") == 0)
		fl = KMAP_NOTIMEOUT;
	      else if (strcmp(*args, "-k") == 0)
		kf = 1;
	      else if (strcmp(*args, "-a") == 0)
		af = 1;
	      else if (strcmp(*args, "-d") == 0)
		df = 1;
	      else if (strcmp(*args, "-m") == 0)
		mf = 1;
	      else if (strcmp(*args, "--") == 0)
		{
		  args++;
		  break;
		}
	      else
		{
	          Msg(0, "%s: bindkey: invalid option %s", rc_name, *args);
		  return;
		}
	    }
	  if (df && mf)
	    {
	      Msg(0, "%s: bindkey: -d does not work with -m", rc_name);
	      break;
	    }
	  if (*args == 0)
	    {
	      if (mf)
		display_bindkey("Edit mode", mmtab);
	      else if (df)
		display_bindkey("Default", dmtab);
	      else
		display_bindkey("User", umtab);
	      break;
	    }
	  if (kf == 0)
	    {
	      if (af)
		{
		  Msg(0, "%s: bindkey: -a only works with -k", rc_name);
		  break;
		}
	      for (i = 0; i < KMAP_EXT; i++)
		if (kmap_extras[i] == 0)
		  {
		    if (args[1])
		      break;
		  }
		else
		  if (strcmp(kmap_extras[i], *args) == 0)
		      break;
	      if (i == KMAP_EXT)
		{
		  Msg(0, args[1] ? "%s: bindkey: no more room for keybinding" : "%s: bindkey: keybinding not found", rc_name);
		  break;
		}
	      if (df == 0 && dmtab[i + KMAP_KEYS + KMAP_AKEYS].nr != RC_ILLEGAL)
		used = 1;
	      if (mf == 0 && mmtab[i + KMAP_KEYS + KMAP_AKEYS].nr != RC_ILLEGAL)
		used = 1;
	      if ((df || mf) && umtab[i + KMAP_KEYS + KMAP_AKEYS].nr != RC_ILLEGAL)
		used = 1;
	      i += KMAP_KEYS + KMAP_AKEYS;
	    }
	  else
	    {
	      for (i = T_CAPS; i < T_OCAPS; i++)
		if (strcmp(term[i].tcname, *args) == 0)
		  break;
	      if (i == T_OCAPS)
		{
		  Msg(0, "%s: bindkey: unknown key '%s'", rc_name, *args);
		  break;
		}
	      if (af && i >= T_CURSOR && i < T_OCAPS)
	        i -=  T_CURSOR - KMAP_KEYS;
	      else
	        i -=  T_CAPS;
	    }
	  newact = df ? &dmtab[i] : mf ? &mmtab[i] : &umtab[i];
	  ClearAction(newact);
	  if (args[1])
	    {
	      if ((newnr = FindCommnr(args[1])) == RC_ILLEGAL)
		{
		  Msg(0, "%s: bindkey: unknown command '%s'", rc_name, args[1]);
		  break;
		}
	      if (CheckArgNum(newnr, args + 2))
		break;
	      newact->nr = newnr;
	      if (args[2])
		newact->args = SaveArgs(args + 2);
	      if (kf == 0 && args[1])
		{
		  if (kmap_extras[i - (KMAP_KEYS+KMAP_AKEYS)])
		    free(kmap_extras[i - (KMAP_KEYS+KMAP_AKEYS)]);
	          kmap_extras[i - (KMAP_KEYS+KMAP_AKEYS)] = SaveStr(*args);
		  kmap_extras_fl[i - (KMAP_KEYS+KMAP_AKEYS)] = fl;
		}
	    }
	  for (display = displays; display; display = display->d_next)
	    remap(i, args[1] ? 1 : 0);
	  if (kf == 0 && !args[1])
	    {
	      i -= KMAP_KEYS + KMAP_AKEYS;
	      if (!used && kmap_extras[i])
		{
		  free(kmap_extras[i]);
		  kmap_extras[i] = 0;
		  kmap_extras_fl[i] = 0;
		}
	    }
	  display = odisp;
	}
      break;
    case RC_MAPTIMEOUT:
      if (*args)
	{
          if (ParseNum(act, &n))
	    break;
	  if (n < 0 || n >= 1000)
	    {
	      Msg(0, "%s: maptimeout: illegal time %d", rc_name, n);
	      break;
	    }
	  maptimeout = n * 1000;
	}
      if (*args == 0 || msgok)
        Msg(0, "maptimeout is %dms", maptimeout/1000);
      break;
    case RC_MAPNOTNEXT:
      D_dontmap = 1;
      break;
    case RC_MAPDEFAULT:
      D_mapdefault = 1;
      break;
#endif
#ifdef MULTIUSER
    case RC_ACLCHG:
    case RC_ACLADD:
      for (n = 0; args[n]; n++)
        ;
      UsersAcl(NULL, n, args);
      break;
    case RC_ACLDEL:
      if (UserDel(args[0], NULL))
	break;
      if (msgok)
	Msg(0, "%s removed from acl database", args[0]);
      break;
    case RC_ACLGRP:
       {
	  break;
	}
    case RC_MULTIUSER:
      if (ParseOnOff(act, &n))
	break;
      multi = n ? "" : 0;
      chsock();
      if (msgok)
	Msg(0, "Multiuser mode %s", multi ? "enabled" : "disabled");
      break;
#endif /* MULTIUSER */
#ifdef PSEUDOS
    case RC_EXEC:
      winexec(args);
      break;
#endif
#ifdef MULTI
    case RC_CLONE:
      execclone(args);
      break;
#endif
    case RC_GR:
      if (ParseSwitch(act, &fore->w_gr) == 0 && msgok)
        Msg(0, "Will %suse GR", fore->w_gr ? "" : "not ");
      break;
    case RC_C1:
      if (ParseSwitch(act, &fore->w_c1) == 0 && msgok)
        Msg(0, "Will %suse C1", fore->w_c1 ? "" : "not ");
      break;
#ifdef KANJI
    case RC_KANJI:
      for (i = 0; i < 2; i++)
	{
	  if (args[i] == 0)
	    break;
	  if (strcmp(args[i], "jis") == 0 || strcmp(args[i], "off") == 0)
	    n = 0;
	  else if (strcmp(args[i], "euc") == 0)
	    n = EUC;
	  else if (strcmp(args[i], "sjis") == 0)
	    n = SJIS;
	  else
	    {
	      Msg(0, "kanji: illegal argument (%s)", args[i]);
	      break;
	    }
	  if (i == 0)
	    fore->w_kanji = n;
	  else
	    D_kanji = n;
	}
      if (fore)
        ResetCharsets(fore);
      break;
    case RC_DEFKANJI:
      if (strcmp(*args, "jis") == 0 || strcmp(*args, "off") == 0)
	n = 0;
      else if (strcmp(*args, "euc") == 0)
	n = EUC;
      else if (strcmp(*args, "sjis") == 0)
	n = SJIS;
      else
	{
	  Msg(0, "defkanji: illegal argument (%s)", *args);
	  break;
	}
      nwin_default.kanji = n;
      break;
#endif
    case RC_PRINTCMD:
      if (*args)
	{
	  if (printcmd)
	    free(printcmd);
	  printcmd = 0;
	  if (**args)
	    printcmd = SaveStr(*args);
	}
      if (*args == 0 || msgok)
	if (printcmd)
	  Msg(0, "using '%s' as print command", printcmd);
	else
	  Msg(0, "using termcap entries for printing");
      break;
    case RC_DIGRAPH:
      Input("Enter digraph: ", 10, INP_EVERY, digraph_fn, NULL);
      if (*args && **args)
	{
	  s = *args;
	  n = strlen(s);
	  Process(&s, &n);
	}
      break;
    case RC_DEFHSTATUS:
      if (*args == 0)
        {
	  char buf[256];
          *buf = 0;
	  if (nwin_default.hstatus)
            AddXChars(buf, sizeof(buf), nwin_default.hstatus);
	  Msg(0, "default hstatus is '%s'", buf);
	  break;
        }
      (void)ParseSaveStr(act, &nwin_default.hstatus);
      if (*nwin_default.hstatus == 0)
	{
	  free(nwin_default.hstatus);
	  nwin_default.hstatus = 0;
	}
      break;
    case RC_DEFCHARSET:
    case RC_CHARSET:
      if (*args == 0)
        {
	  char buf[256];
          *buf = 0;
	  if (nwin_default.charset)
            AddXChars(buf, sizeof(buf), nwin_default.charset);
	  Msg(0, "default charset is '%s'", buf);
	  break;
        }
      n = strlen(*args);
      if (n == 0 || n > 6)
	{
	  Msg(0, "%s: %s: string has illegal size.", rc_name, comms[nr].name);
	  break;
	}
      if (n > 4 && (
        ((args[0][4] < '0' || args[0][4] > '3') && args[0][4] != '.') ||
        ((args[0][5] < '0' || args[0][5] > '3') && args[0][5] && args[0][5] != '.')))
	{
	  Msg(0, "%s: %s: illegal mapping number.", rc_name, comms[nr].name);
	  break;
	}
      if (nr == RC_CHARSET)
	{
	  SetCharsets(fore, *args);
	  break;
	}
      if (nwin_default.charset)
	free(nwin_default.charset);
      nwin_default.charset = SaveStr(*args);
      break;
    case RC_SORENDITION:
      i = mchar_so.attr;
      if (*args && **args)
        {
	  if (ParseBase(act, *args, &i, 16, "hex"))
	    break;
	  if (i < 0 || i >= (1 << NATTR))
	    {
	      Msg(0, "sorendition: bad standout attributes");
	      break;
	    }
        }
#ifdef COLOR
      n = mchar_so.color;
      if (*args && args[1])
	{
	  if (ParseBase(act, args[1], &n, 16, "hex"))
	    break;
	  if (n < 0 || n > 0x99 || (n & 15) > 9)
	    {
	      Msg(0, "sorendition: bad standout color");
	      break;
	    }
	  n = 0x99 - n;
	}
      mchar_so.attr = i;
      mchar_so.color = n;
      Msg(0, "Standout attributes 0x%02x  color 0x%02x", (unsigned char)mchar_so.attr, 0x99 - (unsigned char)mchar_so.color);
#else
      mchar_so.attr = i;
      Msg(0, "Standout attributes 0x%02x", (unsigned char)mchar_so.attr);
#endif
      break;
    default:
      break;
    }
}

void
DoCommand(argv) 
char **argv;
{
  struct action act;

  if ((act.nr = FindCommnr(*argv)) == RC_ILLEGAL)  
    {
      Msg(0, "%s: unknown command '%s'", rc_name, *argv);
      return;
    }
  act.args = argv + 1;
  DoAction(&act, -1);
}

static char **
SaveArgs(args)
char **args;
{
  register char **ap, **pp;
  register int argc = 0;

  while (args[argc])
    argc++;
  if ((pp = ap = (char **) malloc((unsigned) (argc + 1) * sizeof(char **))) == 0)
    Panic(0, strnomem);
  while (argc--)
    *pp++ = SaveStr(*args++);
  *pp = 0;
  return ap;
}

/*
 * buf is split into argument vector args.
 * leading whitespace is removed.
 * @!| abbreviations are expanded.
 * the end of buffer is recognized by '\0' or an un-escaped '#'.
 * " and ' are interpreted.
 *
 * argc is returned.
 */
int 
Parse(buf, args)
char *buf, **args;
{
  register char *p = buf, **ap = args;
  register int delim, argc;

  argc = 0;
  for (;;)
    {
      while (*p && (*p == ' ' || *p == '\t'))
	++p;
      if (argc == 0)
	{
	  /* 
	   * Expand hardcoded shortcuts.
	   * This should not be done here, cause multiple commands per
	   * line or prefixed commands won't be recognized.
	   * But as spaces between shortcut character and arguments
	   * can be ommited this expansion affects tokenisation and
	   * should be done here. Hmmm. jw.
	   */
	  switch (*p)
	    {
	    case '@':
	      *ap++ = "at";
	      /*
	       * If comments were removed before this shortcut expanded,
	       * we wouldn't need this hack.
	       */
	      if (p[1] == '#')
	        *p = '\\';
	      while (*(++p) == ' ' || *p == '\t')
	        ;
	      argc++;
	      break;
#ifdef PSEUDOS
	    case '!':
	    case '|':
	      *ap++ = "exec";
	      if (*p == '!')
		p++;
	      while (*p == ' ' || *p == '\t')
		p++;
	      argc++;
	      break;
#endif
	    }
        }
      if (*p == '\0' || *p == '#')
	{
	  *p = '\0';
	  args[argc] = 0;
	  return argc;
	}
      if (*p == '\\' && p[1] == '#')
        p++;
      if (++argc >= MAXARGS)
	{
	  Msg(0, "%s: too many tokens.", rc_name);
	  return 0;
	}
      delim = 0;
      if (*p == '"' || *p == '\'')
	delim = *p++;
      *ap++ = p;
      while (*p && !(delim ? *p == delim : (*p == ' ' || *p == '\t')))
	++p;
      if (*p == '\0')
	{
	  if (delim)
	    {
	      Msg(0, "%s: Missing quote.", rc_name);
	      return 0;
	    }
	}
      else
        *p++ = '\0';
    }
}

int 
ParseEscape(u, p)
struct user *u;
char *p;
{
  unsigned char buf[2];
  int e, me;

#ifdef MAPKEYS
  if (*p == 0)
    e = me = -1;
  else
#endif
    {
      if ((p = ParseChar(p, (char *)buf)) == NULL ||
	  (p = ParseChar(p, (char *)buf+1)) == NULL || *p)
	return -1;
      e = buf[0];
      me = buf[1];
    }
  if (u)
    {
      u->u_Esc = e;
      u->u_MetaEsc = me;
    }
  else
    {
      if (users)
	{
	  if (DefaultEsc >= 0)
	    ClearAction(&ktab[DefaultEsc]);
	  if (DefaultMetaEsc >= 0)
	    ClearAction(&ktab[DefaultMetaEsc]);
	}
      DefaultEsc = e;
      DefaultMetaEsc = me;
      if (users)
	{
	  if (DefaultEsc >= 0)
	    {
	      ClearAction(&ktab[DefaultEsc]);
	      ktab[DefaultEsc].nr = RC_OTHER;
	    }
	  if (DefaultMetaEsc >= 0)
	    {
	      ClearAction(&ktab[DefaultMetaEsc]);
	      ktab[DefaultMetaEsc].nr = RC_META;
	    }
	}
    }
  return 0;
}

static int
ParseSwitch(act, var)
struct action *act;
int *var;
{
  if (*act->args == 0)
    {
      *var ^= 1;
      return 0;
    }
  return ParseOnOff(act, var);
}

static int
ParseOnOff(act, var)
struct action *act;
int *var;
{
  register int num = -1;
  char **args = act->args;

  if (args[1] == 0)
    {
      if (strcmp(args[0], "on") == 0)
	num = 1;
      else if (strcmp(args[0], "off") == 0)
	num = 0;
    }
  if (num < 0)
    {
      Msg(0, "%s: %s: invalid argument. Give 'on' or 'off'", rc_name, comms[act->nr].name);
      return -1;
    }
  *var = num;
  return 0;
}

static int
ParseSaveStr(act, var)
struct action *act;
char **var;
{
  char **args = act->args;
  if (*args == 0 || args[1])
    {
      Msg(0, "%s: %s: one argument required.", rc_name, comms[act->nr].name);
      return -1;
    }
  if (*var)
    free(*var);
  *var = SaveStr(*args);
  return 0;
}

static int
ParseNum(act, var)
struct action *act;
int *var;
{
  int i;
  char *p, **args = act->args;

  p = *args;
  if (p == 0 || *p == 0 || args[1])
    {
      Msg(0, "%s: %s: invalid argument. Give one argument.",
          rc_name, comms[act->nr].name);
      return -1;
    }
  i = 0; 
  while (*p)
    {
      if (*p >= '0' && *p <= '9')
	i = 10 * i + (*p - '0');
      else
	{
	  Msg(0, "%s: %s: invalid argument. Give numeric argument.",
	      rc_name, comms[act->nr].name);
	  return -1;
	}    
      p++;
    }
  debug1("ParseNum got %d\n", i);
  *var = i;
  return 0;
}

static struct win *
WindowByName(s)
char *s;
{
  struct win *p;

  for (p = windows; p; p = p->w_next)
    if (!strcmp(p->w_title, s))
      return p;
  for (p = windows; p; p = p->w_next)
    if (!strncmp(p->w_title, s, strlen(s)))
      return p;
  return 0;
}

static int
WindowByNumber(str)
char *str;
{
  int i;
  char *s;

  for (i = 0, s = str; *s; s++)
    {
      if (*s < '0' || *s > '9')
        break;
      i = i * 10 + (*s - '0');
    }
  return *s ? -1 : i;
}

/* 
 * Get window number from Name or Number string.
 * Numbers are tried first, then names, a prefix match suffices.
 * Be careful when assigning numeric strings as WindowTitles.
 */
int
WindowByNoN(str)
char *str;
{
  int i;
  struct win *p;
  
  if ((i = WindowByNumber(str)) < 0 || i >= MAXWIN)
    {
      if ((p = WindowByName(str)))
	return p->w_number;
      return -1;
    }
  return i;
}

static int
ParseWinNum(act, var)
struct action *act;
int *var;
{
  char **args = act->args;
  int i = 0;

  if (*args == 0 || args[1])
    {
      Msg(0, "%s: %s: one argument required.", rc_name, comms[act->nr].name);
      return -1;
    }
  
  i = WindowByNoN(*args);
  if (i < 0)
    {
      Msg(0, "%s: %s: invalid argument. Give window number or name.",
          rc_name, comms[act->nr].name);
      return -1;
    }
  debug1("ParseWinNum got %d\n", i);
  *var = i;
  return 0;
}


static int
ParseBase(act, p, var, base, bname)
struct action *act;
char *p;
int *var;
int base;
char *bname;
{
  int i = 0;
  int c;

  if (*p == 0)
    {
      Msg(0, "%s: %s: empty argument.", rc_name, comms[act->nr].name);
      return -1;
    }
  while ((c = *p++))
    {
      if (c >= 'a' && c <= 'z')
	c -= 'a' - 'A';
      if (c >= 'A' && c <= 'Z')
	c -= 'A' - ('0' + 10);
      c -= '0';
      if (c < 0 || c >= base)
	{
	  Msg(0, "%s: %s: argument is not %s.", rc_name, comms[act->nr].name, bname);
	  return -1;
	}    
      i = base * i + c;
    }
  debug1("ParseBase got %d\n", i);
  *var = i;
  return 0;
}

/*
 * Interprets ^?, ^@ and other ^-control-char notation.
 * Interprets \ddd octal notation
 * 
 * The result is placed in *cp, p is advanced behind the parsed expression and 
 * returned. 
 */
static char *
ParseChar(p, cp)
char *p, *cp;
{
  if (*p == 0)
    return 0;
  if (*p == '^')
    {
      if (*++p == '?')
        *cp = '\177';
      else if (*p >= '@')
        *cp = Ctrl(*p);
      else
        return 0;
      ++p;
    }
  else if (*p == '\\' && *++p <= '7' && *p >= '0')
    {
      *cp = 0;
      do
        *cp = *cp * 8 + *p - '0';
      while (*++p <= '7' && *p >= '0');
    }
  else
    *cp = *p++;
  return p;
}


static int
IsNum(s, base)
register char *s;
register int base;
{
  for (base += '0'; *s; ++s)
    if (*s < '0' || *s > base)
      return 0;
  return 1;
}

int
IsNumColon(s, base, p, psize)
int base, psize;
char *s, *p;
{
  char *q;
  if ((q = rindex(s, ':')) != NULL)
    {
      strncpy(p, q + 1, psize - 1);
      p[psize - 1] = '\0';
      *q = '\0';
    }
  else
    *p = '\0';
  return IsNum(s, base);
}

static void
SwitchWindow(n)
int n;
{
  struct win *p;

  debug1("SwitchWindow %d\n", n);
  if (display == 0)
    return;
  if (n < 0 || n >= MAXWIN || (p = wtab[n]) == 0)
    {
      ShowWindows();
      return;
    }
  if (p == D_fore)
    {
      Msg(0, "This IS window %d (%s).", n, p->w_title);
      return;
    }
  if (p->w_display)
    {
      Msg(0, "Window %d (%s) is on another display (%s@%s).", n, p->w_title,
          p->w_display->d_user->u_name, p->w_display->d_usertty);
      return;
    }
  SetForeWindow(p);
  Activate(fore->w_norefresh);  
}  

/* 
 * returns 0, if the lock really has been released 
 */
int
ReleaseAutoWritelock(dis, w)
struct display *dis;
struct win *w;
{
  /* release auto writelock when user has no other display here */
  if (w->w_wlock == WLOCK_AUTO && w->w_wlockuser == D_user)
    {
      struct display *d;

      for (d = displays; d; d = d->d_next)
	if ((d != display) && (d->d_fore == w))
	  break;
      debug3("%s %s autolock on win %d\n", 
	     D_user->u_name, d?"keeps":"releases", w->w_number);
      if (!d)
        {
	  w->w_wlockuser = NULL;
	  return 0;
	}
    }
  return 1;
}

void
SetForeWindow(wi)
struct win *wi;
{
  struct win *p, **pp;
  struct layer *l;
  /*
   * If we come from another window, make it inactive.
   */
  if (display)
    {
      fore = D_fore;
      if (fore)
	{
	  ReleaseAutoWritelock(display, fore);
	  /* deactivate old window. */
	  if (fore->w_tstamp.seconds)
	    fore->w_tstamp.lastio = Now;
	  D_other = fore;
	  fore->w_active = 0;
	  fore->w_display = 0;
	}
      else
	{
	  /* put all the display layers on the window. */
	  for (l = D_lay; l; l = l->l_next)
	    if (l->l_next == &BlankLayer)
	      {
		l->l_next = wi->w_lay;
		wi->w_lay = D_lay;
		for (l = D_lay; l != wi->w_lay; l = l->l_next)
		  l->l_block |= wi->w_lay->l_block;
		break;
	      }
	}
      D_fore = wi;
      if (D_other == wi)
	D_other = 0;
      D_lay = wi->w_lay;
      D_layfn = D_lay->l_layfn;
      if ((wi->w_wlock == WLOCK_AUTO) && 
#ifdef MULTIUSER
          !AclCheckPermWin(D_user, ACL_WRITE, wi) &&
#endif
          !wi->w_wlockuser)
        {
	  debug2("%s obtained auto writelock for window %d\n",
	  	 D_user->u_name, wi->w_number);
          wi->w_wlockuser = D_user;
        }
    }
  fore = wi;
  fore->w_display = display;
  if (!fore->w_lay)
    fore->w_active = 1;
  /*
   * Place the window at the head of the most-recently-used list.
   */
  for (pp = &windows; (p = *pp); pp = &p->w_next)
    if (p == wi)
      break;
  ASSERT(p);
  *pp = p->w_next;
  p->w_next = windows;
  windows = p;
}

static int
NextWindow()
{
  register struct win **pp;
  int n = fore ? fore->w_number : 0;

  for (pp = wtab + n + 1; pp != wtab + n; pp++)
    {
      if (pp == wtab + MAXWIN)
	pp = wtab;
      if (*pp)
	break;
    }
  return pp - wtab;
}

static int
PreviousWindow()
{
  register struct win **pp;
  int n = fore ? fore->w_number : MAXWIN - 1;

  for (pp = wtab + n - 1; pp != wtab + n; pp--)
    {
      if (pp < wtab)
	pp = wtab + MAXWIN - 1;
      if (*pp)
	break;
    }
  return pp - wtab;
}

static int
MoreWindows()
{
  if (windows && windows->w_next)
    return 1;
  if (fore == 0)
    {
      Msg(0, "No window available");
      return 0;
    }
#ifdef NETHACK
  if (nethackflag)
    Msg(0, "You cannot escape from window %d!", fore->w_number);
  else
#endif
  Msg(0, "No other window.");
  return 0;
}

void
KillWindow(wi)
struct win *wi;
{
  struct win **pp, *p;

  display = wi->w_display;
  if (display)
    {
      if (wi == D_fore)
	{
	  RemoveStatus();
	  if (D_lay != &wi->w_winlay)
	    ExitOverlayPage();
	  D_fore = 0;
	  D_lay = &BlankLayer;
	  D_layfn = BlankLayer.l_layfn;
	}
    }

  for (pp = &windows; (p = *pp); pp = &p->w_next)
    if (p == wi)
      break;
  ASSERT(p);
  *pp = p->w_next;
  /*
   * Remove window from linked list.
   */
  wi->w_inlen = 0;
  wtab[wi->w_number] = 0;
  FreeWindow(wi);
  /*
   * If the foreground window disappeared check the head of the linked list
   * of windows for the most recently used window. If no window is alive at
   * all, exit.
   */
  if (display && D_fore)
    return;
  if (windows == 0)
    Finit(0);
  SwitchWindow(windows->w_number);
}

static void
LogToggle(on)
int on;
{
  char buf[1024];

  if ((fore->w_logfp != 0) == on)
    {
      if (display && !*rc_name)
	Msg(0, "You are %s logging.", on ? "already" : "not");
      return;
    }
  strncpy(buf, MakeWinMsg(screenlogfile, fore, '%'), 1023);
  buf[1023] = 0;
  if (fore->w_logfp != NULL)
    {
#ifdef NETHACK
      if (nethackflag)
	Msg(0, "You put away your scroll of logging named \"%s\".", buf);
      else
#endif
      Msg(0, "Logfile \"%s\" closed.", buf);
      fclose(fore->w_logfp);
      fore->w_logfp = NULL;
      return;
    }
  if ((fore->w_logfp = secfopen(buf, "a")) == NULL)
    {
#ifdef NETHACK
      if (nethackflag)
	Msg(0, "You don't seem to have a scroll of logging named \"%s\".", buf);
      else
#endif
      Msg(errno, "Error opening logfile \"%s\"", buf);
      return;
    }
#ifdef NETHACK
  if (nethackflag)
    Msg(0, "You %s your scroll of logging named \"%s\".",
	ftell(fore->w_logfp) ? "add to" : "start writing on", buf);
  else
#endif
  Msg(0, "%s logfile \"%s\"", ftell(fore->w_logfp) ? "Appending to" : "Creating", buf);
}

void
ShowWindows()
{
  char buf[1024];
  register char *s, *ss;
  register struct win **pp, *p;
  register char *cmd;

  ASSERT(display);
  s = ss = buf;
  for (pp = wtab; pp < wtab + MAXWIN; pp++)
    {
      if ((p = *pp) == 0)
	continue;

      cmd = p->w_title;
      if (s - buf + strlen(cmd) > sizeof(buf) - 24)
	break;
      if (s > buf)
	{
	  *s++ = ' ';
	  *s++ = ' ';
	}
      sprintf(s, "%d", p->w_number);
      s += strlen(s);
      if (p == fore)
	{
	  ss = s;
	  *s++ = '*';
	}
      else if (p == D_other)
	*s++ = '-';
      if (p->w_display && p->w_display != display)
	*s++ = '&';
      if (p->w_monitor == MON_DONE || p->w_monitor == MON_MSG)
	*s++ = '@';
      if (p->w_bell == BELL_DONE || p->w_bell == BELL_MSG)
	*s++ = '!';
#ifdef UTMPOK
      if (p->w_slot != (slot_t) 0 && p->w_slot != (slot_t) -1)
	*s++ = '$';
#endif
      if (p->w_logfp != NULL)
	{
	  strcpy(s, "(L)");
	  s += 3;
	}
      if (p->w_ptyfd < 0)
        *s++ = 'Z';
      *s++ = ' ';
      strcpy(s, cmd);
      s += strlen(s);
      if (p == fore)
	{
	  /* 
	   * this is usually done by Activate(), but when looking
	   * on your current window, you may get annoyed, as there is still
	   * that temporal '!' and '@' displayed.
	   * So we remove that after displaying it once.
	   */
	  p->w_bell = BELL_OFF;
	  if (p->w_monitor != MON_OFF)
	    p->w_monitor = MON_ON;
	}
    }
  *s++ = ' ';
  *s = '\0';
  if (ss - buf > D_width / 2)
    {
      ss -= D_width / 2;
      if (s - ss < D_width)
	{
	  ss = s - D_width;
	  if (ss < buf)
	    ss = buf;
	}
    }
  else
    ss = buf;
  Msg(0, "%s", ss);
}


static void
ShowTime()
{
  char buf[512];
  struct tm *tp;
  time_t now;

  (void) time(&now);
  tp = localtime(&now);
  sprintf(buf, "%2d:%02d:%02d %s", tp->tm_hour, tp->tm_min, tp->tm_sec,
	  HostName);
#ifdef LOADAV
  AddLoadav(buf + strlen(buf));
#endif /* LOADAV */
  Msg(0, "%s", buf);
}

static void
ShowInfo()
{
  char buf[512], *p;
  register struct win *wp = fore;
  register int i;

  if (wp == 0)
    {
      Msg(0, "(%d,%d)/(%d,%d) no window", D_x + 1, D_y + 1, D_width, D_height);
      return;
    }
#ifdef COPY_PASTE
  sprintf(buf, "(%d,%d)/(%d,%d)+%d %c%sflow %cins %corg %cwrap %capp %clog %cmon %cr",
#else
  sprintf(buf, "(%d,%d)/(%d,%d) %c%sflow %cins %corg %cwrap %capp %clog %cmon %cr",
#endif
	  wp->w_x + 1, wp->w_y + 1, wp->w_width, wp->w_height,
#ifdef COPY_PASTE
	  wp->w_histheight,
#endif
	  (wp->w_flow & FLOW_NOW) ? '+' : '-',
	  (wp->w_flow & FLOW_AUTOFLAG) ? "" : ((wp->w_flow & FLOW_AUTO) ? "(+)" : "(-)"),
	  wp->w_insert ? '+' : '-', wp->w_origin ? '+' : '-',
	  wp->w_wrap ? '+' : '-', wp->w_keypad ? '+' : '-',
	  (wp->w_logfp != NULL) ? '+' : '-',
	  (wp->w_monitor != MON_OFF) ? '+' : '-',
	  wp->w_norefresh ? '-' : '+');
  if (D_CC0 || (D_CS0 && *D_CS0))
    {
      p = buf + strlen(buf);
      if (wp->w_gr)
        sprintf(p++, " G%c%c [", wp->w_Charset + '0', wp->w_CharsetR + '0');
      else
        sprintf(p, " G%c [", wp->w_Charset + '0');
      p += 5;
      for (i = 0; i < 4; i++)
	{
	  if (wp->w_charsets[i] == ASCII)
	    *p++ = 'B';
	  else if (wp->w_charsets[i] >= ' ')
	    *p++ = wp->w_charsets[i];
	  else
	    {
	      *p++ = '^';
	      *p++ = wp->w_charsets[i] ^ 0x40;
	    }
	}
      *p++ = ']';
#ifdef KANJI
      strcpy(p, wp->w_kanji == EUC ? " euc" : wp->w_kanji == SJIS ? " sjis" : "");
      p += strlen(p);
#endif
      *p = 0;
    }
  Msg(0, "%s", buf);
}


static void
AKAfin(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  ASSERT(display);
  if (len && fore)
    ChangeAKA(fore, buf, 20);
}

static void
InputAKA()
{
  Input("Set window's title to: ", 20, INP_COOKED, AKAfin, NULL);
}

static void
Colonfin(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  if (len)
    RcLine(buf);
}

static void
SelectFin(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  int n;

  if (!len || !display)
    return;
  if ((n = WindowByNoN(buf)) < 0)
    return;
  SwitchWindow(n);
}
    
static void
InputSelect()
{
  Input("Switch to window: ", 20, INP_COOKED, SelectFin, NULL);
}

static char setenv_var[31];


static void
SetenvFin1(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  if (!len || !display)
    return;
  InputSetenv(buf);
}
  
static void
SetenvFin2(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  struct action act;
  char *args[3];

  if (!len || !display)
    return;
  act.nr = RC_SETENV;
  args[0] = setenv_var;
  args[1] = buf;
  args[2] = NULL;
  act.args = args;
  debug2("SetenvFin2: setenv '%s' '%s'\n", setenv_var, buf);
  DoAction(&act, -1);
}

static void
InputSetenv(arg)
char *arg;
{
  static char setenv_buf[50 + sizeof(setenv_var)];	/* need to be static here, cannot be freed */

  if (arg)
    {
      strncpy(setenv_var, arg, sizeof(setenv_var) - 1);
      sprintf(setenv_buf, "Enter value for %s: ", setenv_var);
      Input(setenv_buf, 30, INP_COOKED, SetenvFin2, NULL);
    }
  else
    Input("Setenv: Enter variable name: ", 30, INP_COOKED, SetenvFin1, NULL);
}

/*
 * the following options are understood by this parser:
 * -f, -f0, -f1, -fy, -fa
 * -t title, -T terminal-type, -h height-of-scrollback, 
 * -ln, -l0, -ly, -l1, -l
 * -a, -M, -L
 */
void
DoScreen(fn, av)
char *fn, **av;
{
  struct NewWindow nwin;
  register int num;
  char buf[20];
  char termbuf[25];

  nwin = nwin_undef;
  termbuf[0] = '\0';
  while (av && *av && av[0][0] == '-')
    {
      if (av[0][1] == '-')
	{
	  av++;
	  break;
	}
      switch (av[0][1])
	{
	case 'f':
	  switch (av[0][2])
	    {
	    case 'n':
	    case '0':
	      nwin.flowflag = FLOW_NOW * 0;
	      break;
	    case 'y':
	    case '1':
	    case '\0':
	      nwin.flowflag = FLOW_NOW * 1;
	      break;
	    case 'a':
	      nwin.flowflag = FLOW_AUTOFLAG;
	      break;
	    default:
	      break;
	    }
	  break;
	case 'k':
	case 't':
	  if (av[0][2])
	    nwin.aka = &av[0][2];
	  else if (*++av)
	    nwin.aka = *av;
	  else
	    --av;
	  break;
	case 'T':
	  if (av[0][2])
	    nwin.term = &av[0][2];
	  else if (*++av)
	    nwin.term = *av;
	  else
	    --av;
	  break;
	case 'h':
	  if (av[0][2])
	    nwin.histheight = atoi(av[0] + 2);
	  else if (*++av)
	    nwin.histheight = atoi(*av);
	  else 
	    --av;
	  break;
#ifdef LOGOUTOK
	case 'l':
	  switch (av[0][2])
	    {
	    case 'n':
	    case '0':
	      nwin.lflag = 0;
	      break;
	    case 'y':
	    case '1':
	    case '\0':
	      nwin.lflag = 1;
	      break;
	    default:
	      break;
	    }
	  break;
#endif
	case 'a':
	  nwin.aflag = 1;
	  break;
	case 'M':
	  nwin.monitor = MON_ON;
	  break;
	default:
	  Msg(0, "%s: screen: invalid option -%c.", fn, av[0][1]);
	  break;
	}
      ++av;
    }
  num = 0;
  if (av && *av && IsNumColon(*av, 10, buf, sizeof(buf)))
    {
      if (*buf != '\0')
	nwin.aka = buf;
      num = atoi(*av);
      if (num < 0 || num > MAXWIN - 1)
	{
	  Msg(0, "%s: illegal screen number %d.", fn, num);
	  num = 0;
	}
      nwin.StartAt = num;
      ++av;
    }
  if (av && *av)
    {
      nwin.args = av;
      if (!nwin.aka)
        nwin.aka = Filename(*av);
    }
  MakeWindow(&nwin);
}

#ifdef COPY_PASTE
/*
 * CompileKeys must be called before Markroutine is first used.
 * to initialise the keys with defaults, call CompileKeys(NULL, mark_key_tab);
 *
 * s is an ascii string in a termcap-like syntax. It looks like
 *   "j=u:k=d:l=r:h=l: =.:" and so on...
 * this example rebinds the cursormovement to the keys u (up), d (down),
 * l (left), r (right). placing a mark will now be done with ".".
 */
int
CompileKeys(s, array)
char *s;
unsigned char *array;
{
  int i;
  unsigned char key, value;

  if (!s || !*s)
    {
      for (i = 0; i < 256; i++)
        array[i] = i;
      return 0;
    }
  debug1("CompileKeys: '%s'\n", s);
  while (*s)
    {
      s = ParseChar(s, (char *) &key);
      if (!s || *s != '=')
	return -1;
      do 
	{
          s = ParseChar(++s, (char *) &value);
	  if (!s)
	    return -1;
	  array[value] = key;
	}
      while (*s == '=');
      if (!*s) 
	break;
      if (*s++ != ':')
	return -1;
    }
  return 0;
}
#endif /* COPY_PASTE */

/*
 *  Asynchronous input functions
 */

#if defined(POW_DETACH)
static void
pow_detach_fn(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  if (len)
    {
      *buf = 0;
      return;
    }
  if (ktab[(int)(unsigned char)*buf].nr != RC_POW_DETACH)
    {
      if (display)
        write(D_userfd, "\007", 1);
#ifdef NETHACK
      if (nethackflag)
	 Msg(0, "The blast of disintegration whizzes by you!");
#endif
    }
  else
    Detach(D_POWER);
}
#endif /* POW_DETACH */

#ifdef COPY_PASTE
static void
copy_reg_fn(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  struct plop *pp = plop_tab + (int)(unsigned char)*buf;

  if (len)
    {
      *buf = 0;
      return;
    }
  if (pp->buf)
    free(pp->buf);
  if ((pp->buf = (char *)malloc(D_user->u_copylen)) == NULL)
    {
      Msg(0, strnomem);
      return;
    }
  bcopy(D_user->u_copybuffer, pp->buf, D_user->u_copylen);
  pp->len = D_user->u_copylen;
  Msg(0, "Copied %d characters into register %c", D_user->u_copylen, *buf);
}

static void
ins_reg_fn(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  struct plop *pp = plop_tab + (int)(unsigned char)*buf;


  if (!fore)
    return;	/* Input() should not call us w/o fore, but you never know... */
  if (*buf == '.')
    Msg(0, "ins_reg_fn: Warning: pasting real register '.'!");
  if (len)
    {
      *buf = 0;
      return;
    }
  if (pp->buf)
    {
      if (fore->w_pastebuf)
        free(fore->w_pastebuf);
      fore->w_pastebuf = 0;
      fore->w_pasteptr = pp->buf;
      fore->w_pastelen = pp->len;
      return;
    }
#ifdef NETHACK
  if (nethackflag)
    Msg(0, "Nothing happens.");
  else
#endif
  Msg(0, "Empty register.");
}
#endif /* COPY_PASTE */

static void
process_fn(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  struct plop *pp = plop_tab + (int)(unsigned char)*buf;

  if (len)
    {
      *buf = 0;
      return;
    }
  if (pp->buf)
    {
      ProcessInput(pp->buf, pp->len);
      return;
    }
#ifdef NETHACK
  if (nethackflag)
    Msg(0, "Nothing happens.");
  else
#endif
  Msg(0, "Empty register.");
}


#ifdef PASSWORD

/* ARGSUSED */
static void
pass1(buf, len, data)
char *buf;
int len;
char *data;
{
  strncpy(Password, buf, sizeof(Password) - 1);
  Input("Retype new password:", sizeof(Password) - 1, INP_NOECHO, pass2, NULL);
}

/* ARGSUSED */
static void
pass2(buf, len, data)
char *buf;
int len;
char *data;
{
  int st;
  char salt[2];

  if (buf == 0 || strcmp(Password, buf))
    {
#ifdef NETHACK
      if (nethackflag)
	Msg(0, "[ Passwords don't match - your armor crumbles away ]");
      else
#endif /* NETHACK */
        Msg(0, "[ Passwords don't match - checking turned off ]");
      CheckPassword = 0;
    }
  if (Password[0] == '\0')
    {
      Msg(0, "[ No password - no secure ]");
      CheckPassword = 0;
    }
  for (st = 0; st < 2; st++)
    salt[st] = 'A' + (int)((time(0) >> 6 * st) % 26);
  strncpy(Password, crypt(Password, salt), sizeof(Password) - 1);
  if (CheckPassword)
    {
#ifdef COPY_PASTE
      if (D_user->u_copybuffer)
	UserFreeCopyBuffer(D_user);
      D_user->u_copylen = strlen(Password);
      if ((D_user->u_copybuffer = (char *) malloc(D_user->u_copylen + 1)) == NULL)
	{
	  Msg(0, strnomem);
          D_user->u_copylen = 0;
	}
      else
	{
	  strcpy(D_user->u_copybuffer, Password);
	  Msg(0, "[ Password moved into copybuffer ]");
	}
#else				/* COPY_PASTE */
      Msg(0, "[ Crypted password is \"%s\" ]", Password);
#endif				/* COPY_PASTE */
    }
  if (buf)
    bzero(buf, strlen(buf));
}
#endif /* PASSWORD */

static void
digraph_fn(buf, len, data)
char *buf;
int len;
char *data;	/* dummy */
{
  int ch, i, x;

  ch = buf[len];
  if (ch)
    {
      if (ch < ' ' || ch == '\177')
	return;
      if (len && *buf == '0')
	{
	  if (ch < '0' || ch > '7')
	    {
	      buf[len] = '\034';	/* ^] is ignored by Input() */
	      return;
	    }
	  if (len == 3)
	    buf[len] = '\n';
	  return;
	}
      if (len == 1)
        buf[len] = '\n';
      return;
    }
  buf[len] = buf[len + 1];	/* gross */
  len++;
  if (len < 2)
    return;
  if (buf[0] == '0')
    {
      x = 0;
      for (i = 1; i < len; i++)
	{
	  if (buf[i] < '0' || buf[i] > '7')
	    break;
	  x = x * 8 | (buf[i] - '0');
	}
    }
  else
    {
      for (i = 0; i < sizeof(digraphs)/sizeof(*digraphs); i++)
	if ((digraphs[i][0] == (unsigned char)buf[0] && digraphs[i][1] == (unsigned char)buf[1]) ||
	    (digraphs[i][0] == (unsigned char)buf[1] && digraphs[i][1] == (unsigned char)buf[0]))
	  break;
      if (i == sizeof(digraphs)/sizeof(*digraphs))
	{
	  Msg(0, "Unknown digraph");
	  return;
	}
      x = digraphs[i][2];
    }
  i = 1;
  *buf = x;
  while(i)
    Process(&buf, &i);
}

#ifdef MAPKEYS
static int
StuffKey(i)
int i;
{
  struct action *act;

  debug1("StuffKey #%d", i);
#ifdef DEBUG
  if (i < KMAP_KEYS)
    debug1(" - %s", term[i + T_CAPS].tcname);
#endif
  if (i >= T_CURSOR - T_CAPS && i < T_KEYPAD - T_CAPS && D_cursorkeys)
    i += T_OCAPS - T_CURSOR;
  else if (i >= T_KEYPAD - T_CAPS && i < T_OCAPS - T_CAPS && D_keypad)
    i += T_OCAPS - T_CURSOR;
  debug1(" - action %d\n", i);
  fore = D_fore;
  act = 0;
#ifdef COPY_PASTE
  if (InMark() || InInput())
    act = &mmtab[i];
#endif
  if ((!act || act->nr == RC_ILLEGAL) && !D_mapdefault)
    act = &umtab[i];
  D_mapdefault = 0;
  if (!act || act->nr == RC_ILLEGAL)
    act = &dmtab[i];
  if (act == 0 || act->nr == RC_ILLEGAL)
    return -1;
  DoAction(act, 0);
  return 0;
}
#endif

