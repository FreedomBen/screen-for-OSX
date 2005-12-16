/* Copyright (c) 1991 Juergen Weigert (jnweiger@immd4.uni-erlangen.de)
 *                    Michael Schroeder (mlschroe@immd4.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
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
 *
 * putenv -- screen brings it's own putenv/getenv to systems like
 *           pyramid, ultrix3.0. 
 *
 * thanks to Tim MacKenzie
 */
#ifndef lint
  static char rcs_id[] = "$Id$ FAU";
#endif

/*
 *  putenv  --  put value into environment
 *
 *  Usage:  i = putenv (string)
 *    int i;
 *    char  *string;
 *
 *  where string is of the form <name>=<value>.
 *  If "value" is 0, then "name" will be deleted from the environment.
 *  Putenv returns 0 normally, -1 on error (not enough core for malloc).
 *
 *  Putenv may need to add a new name into the environment, or to
 *  associate a value longer than the current value with a particular
 *  name.  So, to make life simpler, putenv() copies your entire
 *  environment into the heap (i.e. malloc()) from the stack
 *  (i.e. where it resides when your process is initiated) the first
 *  time you call it.
 *
 *  HISTORY
 *  3-Sep-91 Michael Schroeder (mlschroe). Modified to behave as
 *    as putenv.
 * 16-Aug-91 Tim MacKenzie (tym) at Monash University. Modified for
 *    use in screen (iScreen) (ignores final int parameter)
 * 14-Oct-85 Michael Mauldin (mlm) at Carnegie-Mellon University
 *      Ripped out of CMU lib for Rob-O-Matic portability
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *    Created for VAX.  Too bad Bell Labs didn't provide this.  It's
 *    unfortunate that you have to copy the whole environment onto the
 *    heap, but the bookkeeping-and-not-so-much-copying approach turns
 *    out to be much hairier.  So, I decided to do the simple thing,
 *    copying the entire environment onto the heap the first time you
 *    call putenv(), then doing realloc() uniformly later on.
 */
#if defined(NEEDSETENV)

#include "config.h"

#define EXTRASIZE 5        /* increment to add to env. size */

char *index(), *malloc(), *realloc();
int   strlen();

static int  envsize = -1;    /* current size of environment */
extern char **environ;        /* the global which is your env. */

static int  findenv();        /* look for a name in the env. */
static int  newenv();        /* copy env. from stack to heap */
static int  moreenv();        /* incr. size of env. */

int unsetenv(name)
char *name;
{
  register int i;
  
  i = findenv(name);
  if (i<0)
    return;			/* Already here */
  
  free(environ[i]);
  if (envsize > 0)
    envsize--;
  for (; environ[i]; i++)
    environ[i] = environ[i+1];
}

int putenv(string)
char *string;
{ 
  register int  i, j;
  register char *p;
  
  if (envsize < 0)
    {				/* first time putenv called */
      if (newenv() < 0)		/* copy env. to heap */
	return (-1);
    }
  
  i = findenv(string);		/* look for name in environment */

  if (i < 0)
    {			/* name must be added */
      for (i = 0; environ[i]; i++);
      if (i >= (envsize - 1))
	{			/* need new slot */
	  if (moreenv() < 0)
	    return (-1);
	}
      p = malloc(strlen(string) + 1);
      if (p == 0)		/* not enough core */
	return (-1);
      environ[i + 1] = 0;	/* new end of env. */
    }
  else
    {			/* name already in env. */
      p = realloc(environ[i], strlen(string) + 1);
      if (p == 0)
	return (-1);
    }
  sprintf(p, "%s", string); /* copy into env. */
  environ[i] = p;
  
  return (0);
}

static int  findenv(name)
char *name;
{
  register char *namechar, *envchar;
  register int  i, found;
  
  found = 0;
  for (i = 0; environ[i] && !found; i++)
    { 
      envchar = environ[i];
      namechar = name;
      while (*namechar && *namechar != '=' && (*namechar == *envchar))
        { 
	  namechar++;
	  envchar++;
        }
      found = ((*namechar == '\0' || *namechar == '=') && *envchar == '=');
    }
  return (found ? i - 1 : -1);
}

static int newenv()
{ 
  register char **env, *elem;
  register int i, esize;

  for (i = 0; environ[i]; i++)
    ;
  esize = i + EXTRASIZE + 1;
  env = (char **)malloc(esize * sizeof (elem));
  if (env == 0)
    return (-1);

  for (i = 0; environ[i]; i++)
    { 
      elem = malloc(strlen(environ[i]) + 1);
      if (elem == 0)
	return (-1);
      env[i] = elem;
      strcpy(elem, environ[i]);
    }
   
  env[i] = 0;
  environ = env;
  envsize = esize;
  return (0);
}

static int moreenv()
{ 
  register int  esize;
  register char **env;
  
  esize = envsize + EXTRASIZE;
  env = (char **)realloc(environ, esize * sizeof (*env));
  if (env == 0)
    return (-1);
  environ = env;
  envsize = esize;
  return (0);
}

#endif /* NEEDSETENV */


