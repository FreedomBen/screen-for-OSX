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
#ifndef sun
#include <sys/ioctl.h>
#endif

#include "config.h"

#ifdef SVR4
# include <sys/stropts.h>
#endif

#include "screen.h"
#include "extern.h"

extern struct display *displays, *display;
extern struct win *windows, *fore, *wtab[], *console_window;
extern char *ShellArgs[];
extern char *ShellProg;
extern char screenterm[];
extern char HostName[];
extern int TtyMode;
extern struct LayFuncs WinLf;
extern int real_uid, real_gid, eff_uid, eff_gid;
extern char Termcap[];
extern char **NewEnv;

#if defined(TIOCSWINSZ) || defined(TIOCGWINSZ)
extern struct winsize glwz;
#endif

#ifdef _IBMR2
extern int aixhack;
#endif


static int OpenDevice __P((char *, int, int *, char **));
static int ForkWindow __P((char **, char *, char *, char *, struct win *));
static void execvpe __P((char *, char **, char **));


char DefaultShell[] = "/bin/sh";
static char DefaultPath[] = ":/usr/ucb:/bin:/usr/bin";


struct NewWindow nwin_undef   = 
{
  -1, (char *)0, (char **)0, (char *)0, (char *)0, -1, -1, 
  -1, -1, -1, -1, -1, -1, -1, -1, (char *)0, (char *)0
};

struct NewWindow nwin_default = 
{ 
  0, (char *)0, ShellArgs, (char *)0, screenterm, 0, 1*FLOW_NOW, 
  LOGINDEFAULT, DEFAULTHISTHEIGHT, MON_OFF, WLOCK_AUTO,
  1, 1, 0, 0, (char *)0, (char *)0
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
  res->wlock = new->wlock != nwin_undef.wlock ? new->wlock : def->wlock;
  res->wrap = new->wrap != nwin_undef.wrap ? new->wrap : def->wrap;
  res->c1 = new->c1 != nwin_undef.c1 ? new->c1 : def->c1;
  res->gr = new->gr != nwin_undef.gr ? new->gr : def->gr;
#ifdef KANJI
  res->kanji = new->kanji != nwin_undef.kanji ? new->kanji : def->kanji;
#endif
  res->hstatus = new->hstatus != nwin_undef.hstatus ? new->hstatus : def->hstatus;
  res->charset = new->charset != nwin_undef.charset ? new->charset : def->charset;
}

int
MakeWindow(newwin)
struct NewWindow *newwin;
{
  register struct win **pp, *p;
  register int n, i;
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
  debug1("NWin: wlock   %d\n", nwin.wlock);
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
  bzero((char *) p, (int) sizeof(struct win)); /* looks like a calloc above */

  /* save the command line so that zombies can be resurrected */
  for (i = 0; nwin.args[i] && i < MAXARGS - 1; i++)
    p->w_cmdargs[i] = SaveStr(nwin.args[i]);
  p->w_cmdargs[i] = 0;

#ifdef MULTIUSER
  if (NewWindowAcl(p))
    {
      free((char *)p);
      close(f);
      Msg(0, strnomem);
      return -1;
    }
#endif
  p->w_winlay.l_next = 0;
  p->w_winlay.l_layfn = &WinLf;
  p->w_winlay.l_data = (char *)p;
  p->w_lay = &p->w_winlay;
  p->w_display = display;
  p->w_pdisplay = 0;
#ifdef MULTIUSER
  if (display && !AclCheckPermWin(D_user, ACL_WRITE, p))
#else
  if (display)
#endif
    p->w_wlockuser = D_user;
  p->w_number = n;
  p->w_ptyfd = f;
  p->w_aflag = nwin.aflag;
  p->w_flow = nwin.flowflag | ((nwin.flowflag & FLOW_AUTOFLAG) ? (FLOW_AUTO|FLOW_NOW) : FLOW_AUTO);
  if (!nwin.aka)
    nwin.aka = Filename(nwin.args[0]);
  strncpy(p->w_akabuf, nwin.aka, MAXSTR - 1);
  if ((nwin.aka = rindex(p->w_akabuf, '|')) != NULL)
    {
      p->w_autoaka = 0;
      *nwin.aka++ = 0;
      p->w_title = nwin.aka;
      p->w_akachange = nwin.aka + strlen(nwin.aka);
    }
  else
    p->w_title = p->w_akachange = p->w_akabuf;
  if (nwin.hstatus)
    p->w_hstatus = SaveStr(nwin.hstatus);
  p->w_monitor = nwin.monitor;
  p->w_norefresh = 0;
  strncpy(p->w_tty, TtyName, MAXSTR - 1);

#ifndef COPY_PASTE
  nwin.histheight = 0;
#endif
  if (ChangeWindowSize(p, display ? D_defwidth : 80, display ? D_defheight : 24, nwin.histheight))
    {
      FreeWindow(p);
      return -1;
    }
#ifdef KANJI
  p->w_kanji = nwin.kanji;
#endif
  ResetWindow(p);	/* sets w_wrap, w_c1, w_gr */
  if (nwin.charset)
    SetCharsets(p, nwin.charset);

  if (ttyflag == TTY_FLAG_PLAIN)
    {
      p->w_t.flags |= TTY_FLAG_PLAIN;
      p->w_pid = 0;
    }
  else
    {
      debug("forking...\n");
#ifdef PSEUDOS
      p->w_pwin = NULL;
#endif
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
  if (display && D_fore)
    D_other = D_fore;
  *pp = p;
  p->w_next = windows;
  windows = p;
#ifdef UTMPOK
  p->w_slot = (slot_t) -1;
# ifdef LOGOUTOK
  debug1("MakeWindow will %slog in.\n", nwin.lflag?"":"not ");
  if (nwin.lflag)
# else /* LOGOUTOK */
  debug1("MakeWindow will log in, LOGOUTOK undefined in config.h%s.\n", 
         nwin.lflag?"":" (although lflag=0)");
# endif /* LOGOUTOK */
    {
      p->w_slot = (slot_t) 0;
      if (display)
        SetUtmp(p);
    }
#endif
  SetForeWindow(p);
  Activate(p->w_norefresh);
  return n;
}

/*
 * Resurrect a window from Zombie state. 
 * The command vector is therefore stored in the window structure.
 * Note: The terminaltype defaults to screenterm again, the current 
 * working directory is lost.
 */
int
RemakeWindow(p)
struct win *p;
{
  int ttyflag;
  char *TtyName;
  int lflag, f;

  lflag = nwin_default.lflag;
  if ((f = OpenDevice(p->w_cmdargs[0], lflag, &ttyflag, &TtyName)) < 0)
    return -1;

  strncpy(p->w_tty, *TtyName ? TtyName : p->w_title, MAXSTR - 1);
  p->w_ptyfd = f;

  p->w_t.flags &= ~TTY_FLAG_PLAIN;
  if (ttyflag == TTY_FLAG_PLAIN)
    {
      p->w_t.flags |= TTY_FLAG_PLAIN;	/* Just in case... */
      WriteString(p, p->w_cmdargs[0], strlen(p->w_cmdargs[0]));
      WriteString(p, ": ", 2);
      WriteString(p, p->w_title, strlen(p->w_title));
      WriteString(p, "\r\n", 2);
      p->w_pid = 0;
    }
  else 
    {
      for (f = 0; p->w_cmdargs[f]; f++)
	{
	  if (f)
	    WriteString(p, " ", 1);
	  WriteString(p, p->w_cmdargs[f], strlen(p->w_cmdargs[f]));
	}
      WriteString(p, "\r\n", 2);
      p->w_pid = ForkWindow(p->w_cmdargs, (char *)0, nwin_default.term, TtyName, p);
      if (p->w_pid < 0)
        return -1;
    }

#ifdef UTMPOK
  p->w_slot = (slot_t) -1;
  debug1("RemakeWindow will %slog in.\n", lflag ? "" : "not ");
# ifdef LOGOUTOK
  if (lflag)
# endif
    {
      p->w_slot = (slot_t) 0;
      if (display)
	SetUtmp(p);
    }
#endif
  return p->w_number;
}

void
FreeWindow(wp)
struct win *wp;
{
  struct display *d;
  int i;

#ifdef PSEUDOS
  if (wp->w_pwin)
    FreePseudowin(wp);
#endif
#ifdef UTMPOK
  RemoveUtmp(wp);
#endif
  if (wp->w_ptyfd >= 0)
    {
      (void) chmod(wp->w_tty, 0666);
      (void) chown(wp->w_tty, 0, 0);
      close(wp->w_ptyfd);
      wp->w_ptyfd = -1;
    }
  if (wp == console_window)
    console_window = 0;
  if (wp->w_logfp != NULL)
    fclose(wp->w_logfp);
  ChangeWindowSize(wp, 0, 0, 0);
  if (wp->w_hstatus)
    free(wp->w_hstatus);
  for (i = 0; wp->w_cmdargs[i]; i++)
    free(wp->w_cmdargs[i]);
  for (d = displays; d; d = d->d_next)
    if (d->d_other == wp)
      d->d_other = 0;
#ifdef MULTIUSER
  for (i = 0; i < ACL_BITS_PER_WIN; i++)
    free((char *)wp->w_userbits[i]);
#endif
  free((char *)wp);
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

  if ((stat(arg, &st)) == 0 && S_ISCHR(st.st_mode))
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

	if (ioctl(f, TIOCPKT, (char *)&flag))
	  {
	    Msg(errno, "TIOCPKT ioctl");
	    close(f);
	    return -1;
	  }
      }
#endif /* TIOCPKT */
    }
  (void) fcntl(f, F_SETFL, FNBLOCK);
#ifdef linux
  /*
   * Tenebreux (zeus@ns.acadiacom.net) has Linux 1.3.70 where select gets
   * confused in the following condition:
   * Open a pty-master side, request a flush on it, then set packet mode.
   * and call select(). Select will return a possible read, where the 
   * one byte response to the flush can be found. Select will thereafter
   * return a possible read, which yields I/O error.
   *
   * If we request another flush *after* switching into packet mode, this
   * I/O error does not occur. We receive a single response byte although we 
   * send two flush requests now. Maybe we should not flush at all.
   *
   * 10.5.96 jw.
   */
  tcflush(f, TCIOFLUSH);
#endif
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
  int pid;
  char tebuf[25];
  char ebuf[10];
  char shellbuf[7 + MAXPATHLEN];
  char *proc;
#ifndef TIOCSWINSZ
  char libuf[20], cobuf[20];
#endif
  int newfd;
  int w = win->w_width;
  int h = win->w_height;
#ifdef PSEUDOS
  int i, pat, wfdused;
  struct pseudowin *pwin = win->w_pwin;
#endif

  proc = *args;
  if (proc == 0)
    {
      args = ShellArgs;
      proc = *args;
    }
  switch (pid = fork())
    {
    case -1:
      Msg(errno, "fork");
      break;
    case 0:
      signal(SIGHUP, SIG_DFL);
      signal(SIGINT, SIG_DFL);
      signal(SIGQUIT, SIG_DFL);
      signal(SIGTERM, SIG_DFL);
#ifdef BSDJOBS
      signal(SIGTTIN, SIG_DFL);
      signal(SIGTTOU, SIG_DFL);
#endif
#ifdef SIGPIPE
      signal(SIGPIPE, SIG_DFL);
#endif
#ifdef SIGXFSZ
      signal(SIGXFSZ, SIG_DFL);
#endif

      displays = 0;	/* beware of Panic() */
      if (setuid(real_uid) || setgid(real_gid))
	{
	  SendErrorMsg("Setuid/gid: %s", strerror(errno));
	  exit(1);
	}
      eff_uid = real_uid;
      eff_gid = real_gid;
      if (dir && *dir && chdir(dir) == -1)
	{
	  SendErrorMsg("Cannot chdir to %s: %s", dir, strerror(errno));
	  exit(1);
	}

      if (display)
	{
	  brktty(D_userfd);
	  freetty();
	}
      else
	brktty(-1);
#ifdef DEBUG
      if (dfp && dfp != stderr)
	fclose(dfp);
#endif
#ifdef _IBMR2
      close(0);
      dup(aixhack);
      close(aixhack);
#endif
      closeallfiles(win->w_ptyfd);
#ifdef _IBMR2
      aixhack = dup(0);
#endif
#ifdef DEBUG
	{
	  char buf[256];
	  
	  sprintf(buf, "%s/screen.child", DEBUGDIR);
	  if ((dfp = fopen(buf, "a")) == 0)
	    dfp = stderr;
	  else
	    (void) chmod(buf, 0666);
	}
      debug1("=== ForkWindow: pid %d\n", getpid());
#endif
      /* Close the three /dev/null descriptors */
      close(0);
      close(1);
      close(2);
      newfd = -1;
      /* 
       * distribute filedescriptors between the ttys
       */
#ifdef PSEUDOS
      pat = pwin ? pwin->fdpat : 
		   ((F_PFRONT<<(F_PSHIFT*2)) | (F_PFRONT<<F_PSHIFT) | F_PFRONT);
      wfdused = 0;
      for(i = 0; i < 3; i++)
	{
	  if (pat & F_PFRONT << F_PSHIFT * i)
	    {
	      if (newfd < 0)
		{
		  if ((newfd = open(ttyn, O_RDWR)) < 0)
		    {
		      SendErrorMsg("Cannot open %s: %s", ttyn, strerror(errno));
		      exit(1);
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
	    debug1("Clearing NBLOCK on window-fd(%d)\n", win->w_ptyfd);
	    if (fcntl(win->w_ptyfd, F_SETFL, 0))
	      SendErrorMsg("Warning: ForkWindow clear NBLOCK fcntl failed, %d", errno);
	}
#else /* PSEUDOS */
      if ((newfd = open(ttyn, O_RDWR)) != 0)
	{
	  SendErrorMsg("Cannot open %s: %s", ttyn, strerror(errno));
	  exit(1);
	}
      dup(0);
      dup(0);
#endif /* PSEUDOS */
      close(win->w_ptyfd);
#ifdef _IBMR2
      close(aixhack);
#endif

      if (newfd >= 0)
	{
	  struct mode fakemode, *modep;
#if defined(SVR4) && !defined(sgi)
	  if (ioctl(newfd, I_PUSH, "ptem"))
	    {
	      SendErrorMsg("Cannot I_PUSH ptem %s %s", ttyn, strerror(errno));
	      exit(1);
	    }
	  if (ioctl(newfd, I_PUSH, "ldterm"))
	    {
	      SendErrorMsg("Cannot I_PUSH ldterm %s %s", ttyn, strerror(errno));
	      exit(1);
	    }
	  if (ioctl(newfd, I_PUSH, "ttcompat"))
	    {
	      SendErrorMsg("Cannot I_PUSH ttcompat %s %s", ttyn, strerror(errno));
	      exit(1);
	    }
#endif
	  if (fgtty(newfd))
	    SendErrorMsg("fgtty: %s (%d)", strerror(errno), errno);
	  if (display)
	    {
	      debug("ForkWindow: using display tty mode for new child.\n");
	      modep = &D_OldMode;
	    }
	  else
	    {
	      debug("No display - creating tty setting\n");
	      modep = &fakemode;
	      InitTTY(modep, 0);
#ifdef DEBUG
	      DebugTTY(modep);
#endif
	    }
	  /* We only want echo if the users input goes to the pseudo
	   * and the pseudo's stdout is not send to the window.
	   */
#ifdef PSEUDOS
	  if (pwin && (!(pat & F_UWP) || (pat & F_PBACK << F_PSHIFT)))
	    {
	      debug1("clearing echo on pseudywin fd (pat %x)\n", pat);
# if defined(POSIX) || defined(TERMIO)
	      modep->tio.c_lflag &= ~ECHO;
	      modep->tio.c_iflag &= ~ICRNL;
# else
	      modep->m_ttyb.sg_flags &= ~ECHO;
# endif
	    }
#endif
	  SetTTY(newfd, modep);
#ifdef TIOCSWINSZ
	  glwz.ws_col = w;
	  glwz.ws_row = h;
	  (void) ioctl(newfd, TIOCSWINSZ, (char *)&glwz);
#endif
	  /* Always turn off nonblocking mode */
	  (void)fcntl(newfd, F_SETFL, 0);
	}
#ifndef TIOCSWINSZ
      sprintf(libuf, "LINES=%d", h);
      sprintf(cobuf, "COLUMNS=%d", w);
      NewEnv[5] = libuf;
      NewEnv[6] = cobuf;
#endif
#ifdef MAPKEYS
      NewEnv[2] = MakeTermcap(display == 0 || win->w_aflag);
#else
      if (win->w_aflag)
	NewEnv[2] = MakeTermcap(1);
      else
	NewEnv[2] = Termcap;
#endif
      strcpy(shellbuf, "SHELL=");
      strncpy(shellbuf + 6, ShellProg, MAXPATHLEN);
      shellbuf[MAXPATHLEN + 6] = 0;
      NewEnv[4] = shellbuf;
      debug1("ForkWindow: NewEnv[4] = '%s'\n", shellbuf);
      if (term && *term && strcmp(screenterm, term) &&
	  (strlen(term) < 20))
	{
	  char *s1, *s2, tl;

	  sprintf(tebuf, "TERM=%s", term);
	  debug2("Makewindow %d with %s\n", win->w_number, tebuf);
	  tl = strlen(term);
	  NewEnv[1] = tebuf;
	  if ((s1 = index(NewEnv[2], '|')))
	    {
	      if ((s2 = index(++s1, '|')))
		{
		  if (strlen(NewEnv[2]) - (s2 - s1) + tl < 1024)
		    {
		      bcopy(s2, s1 + tl, strlen(s2) + 1);
		      bcopy(term, s1, tl);
		    }
		}
	    }
	}
      sprintf(ebuf, "WINDOW=%d", win->w_number);
      NewEnv[3] = ebuf;

      if (*proc == '-')
	proc++;
      if (!*proc)
	proc = DefaultShell;
      debug1("calling execvpe %s\n", proc);
      execvpe(proc, args, NewEnv);
      debug1("exec error: %d\n", errno);
      SendErrorMsg("Cannot exec '%s': %s", proc, strerror(errno));
      exit(1);
    default:
      break;
    }
#ifdef _IBMR2
  close(aixhack);
  aixhack = -1;
#endif
  return pid;
}

static void
execvpe(prog, args, env)
char *prog, **args, **env;
{
  register char *path = NULL, *p;
  char buf[1024];
  char *shargs[MAXARGS + 1];
  register int i, eaccess = 0;

  if (rindex(prog, '/'))
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

#ifdef PSEUDOS

int
winexec(av)
char **av;
{
  char **pp;
  char *p, *s, *t;
  int i, r = 0, l = 0;
  struct win *w;
  extern struct display *display;
  extern struct win *windows;
  struct pseudowin *pwin;
  
  if ((w = display ? fore : windows) == NULL)
    return -1;
  if (!*av || w->w_pwin)
    {
      Msg(0, "Filter running: %s", w->w_pwin ? w->w_pwin->p_cmd : "(none)");
      return -1;
    }
  if (w->w_ptyfd < 0)
    {
      Msg(0, "You feel dead inside.");
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
      free((char *)pwin);
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

    if (ioctl(pwin->p_ptyfd, TIOCPKT, (char *)&flag))
      {
	Msg(errno, "TIOCPKT ioctl");
	FreePseudowin(w);
	return -1;
      }
  }
#endif /* TIOCPKT */
  pwin->p_pid = ForkWindow(av, (char *)0, (char *)0, t, w);
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
  if (fcntl(w->w_ptyfd, F_SETFL, FNBLOCK))
    Msg(errno, "Warning: FreePseudowin: NBLOCK fcntl failed");
  (void) chmod(pwin->p_tty, 0666);
  (void) chown(pwin->p_tty, 0, 0);
  if (pwin->p_ptyfd >= 0)
    close(pwin->p_ptyfd);
  free((char *)pwin);
  w->w_pwin = NULL;
}

#endif /* PSEUDOS */


#ifdef MULTI

/*
 *  Clone routines. To be removed...
 */

static int CloneTermcap __P((struct display *));
extern char **environ;


int
execclone(av)
char **av;
{
  int f, sf;
  char specialbuf[6];
  struct display *old = display;
  char **avp, *namep;

  sf = OpenPTY(&namep);
  if (sf == -1)
    {
      Msg(0, "No more PTYs.");
      return -1;
    }
#ifdef _IBMR2
  close(aixhack);
  aixhack = -1;
#endif
  f = open(namep, O_RDWR);
  if (f == -1)
    {
      close(sf);
      Msg(errno, "Cannot open slave");
      return -1;
    }
  brktty(f);
  signal(SIGHUP, SIG_IGN);	/* No hangups, please */
  if (MakeDisplay(D_username, namep, D_termname, f, -1, &D_OldMode) == 0)
    {
      display = old;
      Msg(0, "Could not make display.");
      close(f);
      close(sf);
      return -1;
    }
  if (CloneTermcap(old))
    {
      FreeDisplay();
      display = old;
      close(sf);
      return -1;
    }

  SetMode(&D_OldMode, &D_NewMode);
  SetTTY(f, &D_NewMode);

  switch (fork())
    {
    case -1:
      FreeDisplay();
      display = old;
      Msg(errno, "fork");
      close(sf);
      return -1;
    case 0:
      D_usertty[0] = 0;		/* for SendErrorMsg */
      displays = 0;		/* beware of Panic() */
      if (setuid(real_uid) || setgid(real_gid))
	{
	  SendErrorMsg("Setuid/gid: %s", strerror(errno));
	  exit(1);
	}
      eff_uid = real_uid;
      eff_gid = real_gid;
      closeallfiles(sf);
      close(1);
      dup(sf);
      close(sf);
#ifdef DEBUG
	{
	  char buf[256];

	  sprintf(buf, "%s/screen.child", DEBUGDIR);
	  if ((dfp = fopen(buf, "a")) == 0)
	    dfp = stderr;
	  else
	    (void) chmod(buf, 0666);
	}
      debug1("=== Clone: pid %d\n", getpid());
#endif
      for (avp = av; *avp; avp++)
        {
          if (strcmp(*avp, "%p") == 0)
            *avp = namep;
          if (strcmp(*avp, "%X") == 0)
            *avp = specialbuf;
        }
      sprintf(specialbuf, "-SXX1");
      namep += strlen(namep);
      specialbuf[3] = *--namep;
      specialbuf[2] = *--namep;
#ifdef DEBUG
      debug("Calling:");
      for (avp = av; *avp; avp++)
        debug1(" %s", *avp);
      debug("\n");
#endif
      execvpe(*av, av, environ);
      SendErrorMsg("Cannot exec '%s': %s", *av, strerror(errno));
      exit(1);
    default:
      break;
    }
  close(sf);
  InitTerm(0);
  Activate(0);
  if (D_fore == 0)
    ShowWindows();
  return 0;
}

extern struct term term[];      /* terminal capabilities */

static int
CloneTermcap(old)
struct display *old;
{
  char *tp;
  int i, l;

  l = 0;
  for (i = 0; i < T_N; i++)
    if (term[i].type == T_STR && old->d_tcs[i].str)
      l += strlen(old->d_tcs[i].str) + 1;
  if ((D_tentry = (char *)malloc(l)) == 0)
    {
      Msg(0, strnomem);
      return -1;
    }

  tp = D_tentry;
  for (i = 0; i < T_N; i++)
    {
      switch(term[i].type)
        {
        case T_FLG:
          D_tcs[i].flg = old->d_tcs[i].flg;
          break;
        case T_NUM:
          D_tcs[i].num = old->d_tcs[i].num;
          break;
        case T_STR:
          D_tcs[i].str = old->d_tcs[i].str;
          if (D_tcs[i].str)
            {
              strcpy(tp, D_tcs[i].str);
              D_tcs[i].str = tp;
              tp += strlen(tp) + 1;
            }
	  break;
        default:
          Panic(0, "Illegal tc type in entry #%d", i);
        }
    }
  CheckScreenSize(0);
  for (i = 0; i < NATTR; i++)
    D_attrtab[i] = old->d_attrtab[i];
  for (i = 0; i < 256; i++)
    D_c0_tab[i] = old->d_c0_tab[i];
  D_UPcost = old->d_UPcost;
  D_DOcost = old->d_DOcost;
  D_NLcost = old->d_NLcost;
  D_LEcost = old->d_LEcost;
  D_NDcost = old->d_NDcost;
  D_CRcost = old->d_CRcost;
  D_IMcost = old->d_IMcost;
  D_EIcost = old->d_EIcost;
#ifdef AUTO_NUKE
  D_auto_nuke = old->d_auto_nuke;
#endif
  if (D_CXC)
    CreateTransTable(D_CXC);
  D_tcinited = 1;
  return 0;
}

#endif

