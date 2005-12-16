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
#include <fcntl.h>
#ifdef ultrix
# include <sys/fixpoint.h>
#endif /* ultrix */

#include "config.h"
#include "screen.h"

#include "extern.h"

#ifdef LOADAV

static int GetLoadav __P((void));

static LOADAV_TYPE loadav[LOADAV_NUM];
static int loadok;



/***************************************************************/

#if defined(linux) && !defined(LOADAV_DONE)
#define LOADAV_DONE
/*
 * This is the easy way. It relies in /proc being mounted.
 * For the big and ugly way refer to previous screen version.
 */
void
InitLoadav()
{
  loadok = 1;
}

static int
GetLoadav()
{
  FILE *fp;
  double d[3];
  int i;

  if ((fp = secfopen("/proc/loadavg", "r")) == NULL)
    return 0;
  fscanf(fp, "%lf %lf %lf\n", d, d + 1, d + 2);
  fclose(fp);
  for (i = 0; i < (LOADAV_NUM > 3 ? 3 : LOADAV_NUM); i++)
    loadav[i] = d[i];
  return i;
}
#endif /* linux */

/***************************************************************/

#if defined(LOADAV_GETLOADAVG) && !defined(LOADAV_DONE)
#define LOADAV_DONE
void
InitLoadav()
{
  loadok = 1;
}

static int
GetLoadav()
{
  return getloadavg(loadav, LOADAV_NUM);
}
#endif

/***************************************************************/

#if defined(apollo) && !defined(LOADAV_DONE)
#define LOADAV_DONE
void
InitLoadav()
{
  loadok = 1;
}

static int
GetLoadav()
{
  proc1_$get_loadav(loadav);
  return LOADAV_NUM;
}
#endif

/***************************************************************/

#if defined(NeXT) && !defined(LOADAV_DONE)
#define LOADAV_DONE

#include <mach/mach.h>

static processor_set_t default_set;

void
InitLoadav()
{
  kern_return_t error;

  error = processor_set_default(host_self(), &default_set);
  if (error != KERN_SUCCESS)
    mach_error("Error calling processor_set_default", error);
  else
    loadok = 1;
}

static int
GetLoadav()
{
  unsigned int info_count;
  struct processor_set_basic_info info;
  host_t host;

  info_count = PROCESSOR_SET_BASIC_INFO_COUNT;
  if (processor_set_info(default_set, PROCESSOR_SET_BASIC_INFO, &host, (processor_set_info_t)&info, &info_count) != KERN_SUCCESS)
    return 0;
  loadav[0] = (float)info.load_average / LOAD_SCALE;
  return 1;
}
#endif

/***************************************************************/

#if !defined(LOADAV_DONE)
/*
 * The old fashion way: open kernel and read avenrun
 *
 * Header File includes
 */

# ifdef NLIST_STRUCT
#  include <nlist.h>
# else
#  include <a.out.h>
# endif
# ifndef NLIST_DECLARED
extern int nlist __P((char *, struct nlist *));
# endif

#ifdef __sgi
# if _MIPS_SZLONG == 64
#  define nlist nlist64
# endif
#endif

static struct nlist nl[2];
static int kmemf;

void
InitLoadav()
{
  debug("Init Kmem...\n");
  if ((kmemf = open("/dev/kmem", O_RDONLY)) == -1)
    return;
# if !defined(_AUX_SOURCE) && !defined(AUX)
#  ifdef NLIST_NAME_UNION
  nl[0].n_un.n_name = LOADAV_AVENRUN;
#  else
  nl[0].n_name = LOADAV_AVENRUN;
#  endif
# else
  strncpy(nl[0].n_name, LOADAV_AVENRUN, sizeof(nl[0].n_name));
# endif
  debug2("Searching in %s for %s\n", LOADAV_UNIX, nl[0].n_name);
  nlist(LOADAV_UNIX, nl);
# ifndef _IBMR2
  if (nl[0].n_value == 0)
# else
  if (nl[0].n_value == 0 || lseek(kmemf, (off_t) nl[0].n_value, 0) == (off_t)-1 || read(kmemf, (char *)&nl[0].n_value, sizeof(nl[0].n_value)) != sizeof(nl[0].n_value))
# endif
    {
      close(kmemf);
      return;
    }
# ifdef sgi
  nl[0].n_value &= (unsigned long)-1 >> 1;	/* clear upper bit */
# endif /* sgi */
  debug1("AvenrunSym found (0x%lx)!!\n", nl[0].n_value);
  loadok = 1;
}

static int
GetLoadav()
{
  if (lseek(kmemf, (off_t) nl[0].n_value, 0) == (off_t)-1)
    return 0;
  if (read(kmemf, (char *) loadav, sizeof(loadav)) != sizeof(loadav))
    return 0;
  return LOADAV_NUM;
}
#endif

/***************************************************************/

#ifndef FIX_TO_DBL
#define FIX_TO_DBL(l) ((double)(l) /  LOADAV_SCALE)
#endif

void
AddLoadav(p)
char *p;
{
  int i, j;
  if (loadok == 0)
    return;
  j = GetLoadav();
  for (i = 0; i < j; i++)
    {
      sprintf(p, " %2.2f", FIX_TO_DBL(loadav[i]));
      p += strlen(p);
    }
}

#endif /* LOADAV */
