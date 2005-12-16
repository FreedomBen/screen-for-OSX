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
 *	Larry W. Virden (lvirden@cas.org)
 *	Howard Chu (hyc@hanauma.jpl.nasa.gov)
 *	Tim MacKenzie (tym@dibbler.cs.monash.edu.au)
 *	Markku Jarvinen (mta@{cc,cs,ee}.tut.fi)
 *	Marc Boucher (marc@CAM.ORG)
 *
 ****************************************************************
 */

#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <sys/types.h>
#include <signal.h>

#include "config.h"
#include "screen.h"
#include "extern.h"

char DefaultShell[] = "/bin/sh";
static char DefaultPath[] = ":/usr/ucb:/bin:/usr/bin";

void execvpe(prog, args, env)
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
  
  if ((w = display ? d_fore : windows) == NULL)
    return -1;
  if (w->w_pwin)
    {
      Msg(ENOSPC, "Only one pseudowin allowed per window.");
      return -1;
    }
  if (!(pwin = (struct pseudowin *)malloc(sizeof(struct pseudowin))))
    {
      Msg(EAGAIN, strnomem);
      return -1;
    }

  /* allow ^a:!!./ttytest as a short form for ^a:exec !.. ./ttytest */
  for (s = *av; *s == ' '; s++)
    ;
  for (p = s; *p == ':' || *p == '.' || *p == '!'; p++)
    ;
  while (*p && p > s && p[-1] == '.')
    p--;
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
  
  if ((l = OpenDevice(av[0], 0, &pwin->p_ptyfd, &t)) < 0)
    return l;
  strncpy(pwin->p_tty, t, MAXSTR - 1);
  if (l == TTY_FLAG_PLAIN)
    {
      pwin->p_pid = 0;
      Msg(0, "Cannot handle a TTY as a pseudo win.");
      close(pwin->p_ptyfd);
      return -1;
    }
  w->w_pwin = pwin;
  pwin->p_pid = ForkWindow(av, NULL, NULL, t, w);
  if ((r = pwin->p_pid) < 0)
    FreePseudowin(w);
  return r;
}
