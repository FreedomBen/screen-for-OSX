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
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#ifndef sun
#include <sys/ioctl.h>
#endif
#ifdef SVR4	/* for solaris 2.1 */
# include <sys/stropts.h>
#endif

#include "config.h"
#include "screen.h"
#include "extern.h"

extern struct display *displays, *display;
extern struct win *windows, *fore, *wtab[], *console_window;
extern char *ShellArgs[];
extern char screenterm[];
extern char HostName[];
extern int default_monitor, TtyMode;
extern struct LayFuncs WinLf;
extern int real_uid, real_gid;
extern char Termcap[];
extern char **NewEnv;

#if defined(TIOCSWINSZ) || defined(TIOCGWINSZ)
extern struct winsize glwz;
#endif


static int OpenDevice __P((char *, int, int *, char **));
static int ForkWindow __P((char **, char *, char *, char *, struct win *));
static void execvpe __P((char *, char **, char **));


char DefaultShell[] = "/bin/sh";
static char DefaultPath[] = ":/usr/ucb:/bin:/usr/bin";


struct NewWindow nwin_undef   = 
{
  -1, (char *)0, (char **)0, (char *)0, (char *)0, -1, -1, 
  -1, -1, -1, (struct pseudowin *)0
};

struct NewWindow nwin_default = 
{ 
  0, 0, ShellArgs, 0, screenterm, 0, 1*FLOW_NOW, 
  LOGINDEFAULT, DEFAULTHISTHEIGHT, -1, (struct pseudowin *)0 
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
  res->monitor = new->monitor != nwin_undef.monitor ? new->monitor : def->monitor;
  res->pwin = new->pwin;
}

int
MakeWindow(newwin)
struct NewWindow *newwin;
{
  register struct win **pp, *p;
  register int n;
  int f = -1;
  struct NewWindow nwin;
  int ttyflag;
  char *TtyName;

  debug1("NewWindow: StartAt %d\n", newwin->StartAt);
  debug1("NewWindow: aka     %s\n", newwin->aka?newwin->aka:"NULL");
  debug1("NewWindow: dir     %s\n", newwin->dir?newwin->dir:"NULL");
  debug1("NewWindow: term    %s\n", newwin->term?newwin->term:"NULL");
  nwin_compose(&nwin_default, newwin, &nwin);
  debug1("NWin: aka     %s\n", nwin.aka ? nwin.aka : "NULL");
  pp = wtab + nwin.StartAt;

  do
    {
      if (*pp == 0)
	break;
      if (++pp == wtab + MAXWIN)
	pp = wtab;
    }
  while (pp != wtab + nwin.StartAt);
  if (*pp)
    {
      Msg(0, "No more windows.");
      return -1;
    }

#if defined(USRLIMIT) && defined(UTMPOK)
  /*
   * Count current number of users, if logging windows in.
   */
  if (nwin.lflag && CountUsers() >= USRLIMIT)
    {
      Msg(0, "User limit reached.  Window will not be logged in.");
      nwin.lflag = 0;
    }
#endif
  n = pp - wtab;
  debug1("Makewin creating %d\n", n);

  if ((f = OpenDevice(nwin.args[0], nwin.lflag, &ttyflag, &TtyName)) < 0)
    return -1;

  if ((p = (struct win *) malloc(sizeof(struct win))) == 0)
    {
      close(f);
      Msg(0, strnomem);
      return -1;
    }
  bzero((char *) p, (int) sizeof(struct win));
  p->w_dupto = -1;
  p->w_winlay.l_next = 0;
  p->w_winlay.l_layfn = &WinLf;
  p->w_winlay.l_data = (char *)p;
  p->w_lay = &p->w_winlay;
  p->w_display = display;
  p->w_number = n;
  p->w_ptyfd = f;
  p->w_aflag = nwin.aflag;
  p->w_flow = nwin.flowflag | ((nwin.flowflag & FLOW_AUTOFLAG) ? (FLOW_AUTO|FLOW_NOW) : FLOW_AUTO);
  if (!nwin.aka)
    nwin.aka = Filename(nwin.args[0]);
  strncpy(p->w_cmd, nwin.aka, MAXSTR - 1);
  if ((nwin.aka = rindex(p->w_cmd, '|')) != NULL)
    {
      *nwin.aka++ = '\0';
      nwin.aka += strlen(nwin.aka);
      p->w_akapos = nwin.aka - p->w_cmd;
      p->w_autoaka = 0;
    }
  else
    p->w_akapos = 0;
  if ((p->w_monitor = nwin.monitor) == -1)
    p->w_monitor = default_monitor;
  p->w_norefresh = 0;
  strncpy(p->w_tty, TtyName, MAXSTR - 1);

  if (ChangeWindowSize(p, display ? d_defwidth : 80, display ? d_defheight : 24))
    {
      FreeWindow(p);
      return -1;
    }
#ifdef COPY_PASTE
  ChangeScrollback(p, nwin.histheight, p->w_width);
#endif
  ResetWindow(p);	/* sets p->w_wrap */

  if (ttyflag == TTY_FLAG_PLAIN)
    {
      p->w_t.flags |= TTY_FLAG_PLAIN;
      p->w_pid = 0;
    }
  else
    {
      debug("forking...\n");
      p->w_pwin = NULL;
      p->w_pid = ForkWindow(nwin.args, nwin.dir, nwin.term, TtyName, p);
      if (p->w_pid < 0)
	{
	  FreeWindow(p);
	  return -1;
	}
    }
  /*
   * Place the newly created window at the head of the most-recently-used list.
   */
  if (display && d_fore)
    d_other = d_fore;
  *pp = p;
  p->w_next = windows;
  windows = p;
#ifdef UTMPOK
  debug1("MakeWindow will %slog in.\n", nwin.lflag?"":"not ");
  if (nwin.lflag == 1)
    {
      if (display)
        SetUtmp(p);
      else
	p->w_slot = (slot_t) 0;
    }
  else
    p->w_slot = (slot_t) -1;
#endif
  SetForeWindow(p);
  Activate(p->w_norefresh);
  return n;
}

void
FreeWindow(wp)
struct win *wp;
{
  struct display *d;

  if (wp->w_pwin)
    FreePseudowin(wp);
#ifdef UTMPOK
  RemoveUtmp(wp);
#endif
  (void) chmod(wp->w_tty, 0666);
  (void) chown(wp->w_tty, 0, 0);
  close(wp->w_ptyfd);
  if (wp == console_window)
    console_window = 0;
  if (wp->w_logfp != NULL)
    fclose(wp->w_logfp);
  ChangeWindowSize(wp, 0, 0);
  for (d = displays; d; d = d->_d_next)
    if (d->_d_other == wp)
      d->_d_other = 0;
  free(wp);
}

static int
OpenDevice(arg, lflag, typep, namep)
char *arg;
int lflag;
int *typep;
char **namep;
{
  struct stat st;
  int f;

  if ((stat(arg, &st)) == 0 && (st.st_mode & S_IFCHR))
    {
      if (access(arg, R_OK | W_OK) == -1)
	{
	  Msg(errno, "Cannot access line '%s' for R/W", arg); 
	  return -1;
	}
      debug("OpenDevice: OpenTTY\n");
      if ((f = OpenTTY(arg)) < 0)
	return -1;
      *typep = TTY_FLAG_PLAIN;
      *namep = arg;
    }
  else
    {
      *typep = 0;    /* for now we hope it is a program */
      f = OpenPTY(namep);
      if (f == -1)
	{
	  Msg(0, "No more PTYs.");
	  return -1;
	}
#ifdef TIOCPKT
      {
	int flag = 1;

	if (ioctl(f, TIOCPKT, &flag))
	  {
	    Msg(errno, "TIOCPKT ioctl");
	    close(f);
	    return -1;
	  }
      }
#endif /* TIOCPKT */
    }
  (void) fcntl(f, F_SETFL, FNDELAY);
#ifdef PTYGROUP
  (void) chown(*namep, real_uid, PTYGROUP);
#else
  (void) chown(*namep, real_uid, real_gid);
#endif
#ifdef UTMPOK
  (void) chmod(*namep, lflag ? TtyMode : (TtyMode & ~022));
#else
  (void) chmod(*namep, TtyMode);
#endif
  return f;
}

/*
 * Fields w_width, w_height, aflag, number (and w_tty)
 * are read from struct win *win. No fields written.
 * If pwin is nonzero, filedescriptors are distributed 
 * between win->w_tty and open(ttyn)
 *
 */
static int 
ForkWindow(args, dir, term, ttyn, win)
char **args, *dir, *term, *ttyn;
struct win *win;
{
  struct pseudowin *pwin = win->w_pwin;
  int pid;
  char tebuf[25];
  char ebuf[10];
#ifndef TIOCSWINSZ
  char libuf[20], cobuf[20];
#endif
  int i, newfd, pat, wfdused;
  int w = win->w_width;
  int h = win->w_height;

  switch (pid = fork())
    {
    case -1:
      Msg(errno, "fork");
      return -1;
    case 0:
      signal(SIGHUP, SIG_DFL);
      signal(SIGINT, SIG_DFL);
      signal(SIGQUIT, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
#ifdef BSDJOBS
      signal(SIGTTIN, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);
#endif
      if (setuid(real_uid) || setgid(real_gid))
	{
	  SendErrorMsg("Setuid/gid: %s", sys_errlist[errno]);
	  eexit(1);
	}
      if (dir && *dir && chdir(dir) == -1)
	{
	  SendErrorMsg("Cannot chdir to %s: %s", dir, sys_errlist[errno]);
	  eexit(1);
	}

      if (display)
	{
	  brktty(d_userfd);
	  freetty();
	}
      else
	brktty(-1);
#ifdef DEBUG
      if (dfp != stderr)
	fclose(dfp);
#endif
      closeallfiles(win->w_ptyfd);
#ifdef DEBUG
      if ((dfp = fopen("/tmp/debug/screen.child", "a")) == 0)
	dfp = stderr;
      else
	(void) chmod("/tmp/debug/screen.child", 0666);
      debug1("=== ForkWindow: pid %d\n", getpid());
#endif
      /* 
       * distribute filedescriptors between the ttys
       */
      pat = pwin ? pwin->fdpat : 
		   ((F_PFRONT<<(F_PSHIFT*2)) | (F_PFRONT<<F_PSHIFT) | F_PFRONT);
      newfd = -1;
      wfdused = 0;
      /* Close the three /dev/null descriptors */
      close(0);
      close(1);
      close(2);
      for(i = 0; i < 3; i++)
	{
	  if (pat & F_PFRONT << F_PSHIFT * i)
	    {
	      if (newfd < 0)
		{
		  if ((newfd = open(ttyn, O_RDWR)) < 0)
		    {
		      SendErrorMsg("Cannot open %s: %s",
				   ttyn, sys_errlist[errno]);
		      eexit(1);
		    }
		}
	      else
		dup(newfd);
	    }
	  else
	    {
	      dup(win->w_ptyfd);
	      wfdused = 1;
	    }
	}
      if (wfdused)
	{
	    /* 
	     * the pseudo window process should not be surprised with a 
	     * nonblocking filedescriptor. Poor Backend!
	     */
	    debug1("Clearing NDELAY on window-fd(%d)\n", win->w_ptyfd);
	    if (fcntl(win->w_ptyfd, F_SETFL, 0))
	      SendErrorMsg("Warning: ForkWindow clear NDELAY fcntl failed, %d", errno);
	}
      close(win->w_ptyfd);

      if (newfd >= 0)
	{
	  struct mode fakemode, *modep;
#ifdef SVR4
	  if (ioctl(newfd, I_PUSH, "ptem"))
	    {
	      SendErrorMsg("Cannot I_PUSH ptem %s %s", ttyn, sys_errlist[errno]);
	      eexit(1);
	    }
	  if (ioctl(newfd, I_PUSH, "ldterm"))
	    {
	      SendErrorMsg("Cannot I_PUSH ldterm %s %s", ttyn, sys_errlist[errno]);
	      eexit(1);
	    }
	  if (ioctl(newfd, I_PUSH, "ttcompat"))
	    {
	      SendErrorMsg("Cannot I_PUSH ttcompat %s %s", ttyn, sys_errlist[errno]);
	      eexit(1);
	    }
#endif
	  if (fgtty(newfd))
	    SendErrorMsg("fgtty: %s (%d)", sys_errlist[errno], errno);
	  if (display)
	    {
	      debug("ForkWindow: using display tty mode for new child.\n");
	      modep = &d_OldMode;
	    }
	  else
	    {
	      debug("No display - creating tty setting\n");
	      modep = &fakemode;
	      InitTTY(modep, 0);
#ifdef DEBUGGGGGGGGGGGGGG
	      DebugTTY(modep);
#endif
	    }
	  /* We only want echo if the users input goes to the pseudo
	   * and the pseudo's stdout is not send to the window.
	   */
	  if (pwin && (!(pat & F_UWP) || (pat & F_PBACK << F_PSHIFT)))
	    {
	      debug1("clearing echo on pseudywin fd (pat %x)\n", pat);
#if defined(POSIX) || defined(TERMIO)
	      modep->tio.c_lflag &= ~ECHO;
	      modep->tio.c_iflag &= ~ICRNL;
#else
	      modep->m_ttyb.sg_flags &= ~ECHO;
#endif
	    }
	  SetTTY(newfd, modep);
#ifdef TIOCSWINSZ
	  glwz.ws_col = w;
	  glwz.ws_row = h;
	  (void) ioctl(newfd, TIOCSWINSZ, &glwz);
#endif
	}
#ifndef TIOCSWINSZ
      sprintf(libuf, "LINES=%d", h);
      sprintf(cobuf, "COLUMNS=%d", w);
      NewEnv[4] = libuf;
      NewEnv[5] = cobuf;
#endif
      if (win->w_aflag)
	NewEnv[2] = MakeTermcap(1);
      else
	NewEnv[2] = Termcap;
      if (term && *term && strcmp(screenterm, term) &&
	  (strlen(term) < 20))
	{
	  char *s1, *s2, tl;

	  sprintf(tebuf, "TERM=%s", term);
	  debug2("Makewindow %d with %s\n", win->w_number, tebuf);
	  tl = strlen(term);
	  NewEnv[1] = tebuf;
	  if ((s1 = index(Termcap, '|')))
	    {
	      if ((s2 = index(++s1, '|')))
		{
		  if (strlen(Termcap) - (s2 - s1) + tl < 1024)
		    {
		      bcopy(s2, s1 + tl, strlen(s2) + 1);
		      bcopy(term, s1, tl);
		    }
		}
	    }
	}
      sprintf(ebuf, "WINDOW=%d", win->w_number);
      NewEnv[3] = ebuf;

      debug1("calling execvpe %s\n", *args);
      execvpe(*args, args, NewEnv);
      debug1("exec error: %d\n", errno);
      SendErrorMsg("Cannot exec %s: %s", *args, sys_errlist[errno]);
      exit(1);
    default:
      return pid;
    } /* end fork switch */
  /* NOTREACHED */
}

static void
execvpe(prog, args, env)
char *prog, **args, **env;
{
  register char *path = NULL, *p;
  char buf[1024];
  char *shargs[MAXARGS + 1];
  register int i, eaccess = 0;

  for (i = 0; i < 3; i++)
    if (!strncmp("../" + i, prog, 3 - i))
      path = "";
  if (!path && !(path = getenv("PATH")))
    path = DefaultPath;
  do
    {
      p = buf;
      while (*path && *path != ':')
	*p++ = *path++;
      if (p > buf)
	*p++ = '/';
      strcpy(p, prog);
      execve(buf, args, env);
      switch (errno)
	{
	case ENOEXEC:
	  shargs[0] = DefaultShell;
	  shargs[1] = buf;
	  for (i = 1; (shargs[i + 1] = args[i]) != NULL; ++i)
	    ;
	  execve(DefaultShell, shargs, env);
	  return;
	case EACCES:
	  eaccess = 1;
	  break;
	case ENOMEM:
	case E2BIG:
	case ETXTBSY:
	  return;
	}
    } while (*path++);
  if (eaccess)
    errno = EACCES;
}

int
winexec(av)
char **av;
{
  char **pp;
  char *p, *s, *t;
  int i, r = 0, l = 0;
  struct pseudowin *pwin;
  struct win *w;
  extern struct display *display;
  extern struct win *windows;
  
  if ((w = display ? fore : windows) == NULL)
    return -1;
  if (!*av || w->w_pwin)
    {
      Msg(0, "Filter running: %s", w->w_pwin ? w->w_pwin->p_cmd : "(none)");
      return -1;
    }
  if (!(pwin = (struct pseudowin *)malloc(sizeof(struct pseudowin))))
    {
      Msg(0, strnomem);
      return -1;
    }
  bzero((char *)pwin, (int)sizeof(*pwin));

  /* allow ^a:!!./ttytest as a short form for ^a:exec !.. ./ttytest */
  for (s = *av; *s == ' '; s++)
    ;
  for (p = s; *p == ':' || *p == '.' || *p == '!'; p++)
    ;
  if (*p != '|')
    while (*p && p > s && p[-1] == '.')
      p--;
  if (*p == '|')
    {
      l = F_UWP;
      p++;
    }
  if (*p) 
    av[0] = p;
  else
    av++;

  t = pwin->p_cmd;
  for (i = 0; i < 3; i++)
    {
      *t = (s < p) ? *s++ : '.';
      switch (*t++)
	{
	case '.':
	case '|':
	  l |= F_PFRONT << (i * F_PSHIFT);
	  break;
	case '!':
	  l |= F_PBACK << (i * F_PSHIFT);
	  break;
	case ':':
	  l |= F_PBOTH << (i * F_PSHIFT);
	  break;
	}
    }
  
  if (l & F_UWP)
    {
      *t++ = '|';
      if ((l & F_PMASK) == F_PFRONT)
	{
	  *pwin->p_cmd = '!';
	  l ^= F_PFRONT | F_PBACK;
	}
    }
  if (!(l & F_PBACK))
    l |= F_UWP;
  *t++ = ' ';
  pwin->fdpat = l;
  debug1("winexec: '%#x'\n", pwin->fdpat);
  
  l = MAXSTR - 4;
  for (pp = av; *pp; pp++)
    {
      p = *pp;
      while (*p && l-- > 0)
        *t++ = *p++;
      if (l <= 0)
	break;
      *t++ = ' ';
    }
  *--t = '\0';
  debug1("%s\n", pwin->p_cmd);
  
  if ((pwin->p_ptyfd = OpenDevice(av[0], 0, &l, &t)) < 0)
    {
      free(pwin);
      return -1;
    }
  strncpy(pwin->p_tty, t, MAXSTR - 1);
  w->w_pwin = pwin;
  if (l == TTY_FLAG_PLAIN)
    {
      FreePseudowin(w);
      Msg(0, "Cannot handle a TTY as a pseudo win.");
      return -1;
    }
#ifdef TIOCPKT
  {
    int flag = 0;

    if (ioctl(pwin->p_ptyfd, TIOCPKT, &flag))
      {
	Msg(errno, "TIOCPKT ioctl");
	FreePseudowin(w);
	return -1;
      }
  }
#endif /* TIOCPKT */
  pwin->p_pid = ForkWindow(av, NULL, NULL, t, w);
  if ((r = pwin->p_pid) < 0)
    FreePseudowin(w);
  return r;
}

void
FreePseudowin(w)
struct win *w;
{
  struct pseudowin *pwin = w->w_pwin;

  ASSERT(pwin);
  if (fcntl(w->w_ptyfd, F_SETFL, FNDELAY))
    Msg(errno, "Warning: FreePseudowin: NDELAY fcntl failed");
  (void) chmod(pwin->p_tty, 0666);
  (void) chown(pwin->p_tty, 0, 0);
  if (pwin->p_ptyfd >= 0)
    close(pwin->p_ptyfd);
  free(pwin);
  w->w_pwin = NULL;
}

