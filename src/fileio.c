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
#ifndef sgi
# include <sys/file.h>
#endif /* sgi */
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>

#include "config.h" 
#include "screen.h"
#include "extern.h"

#ifdef NETHACK
extern nethackflag;
#endif

extern struct display *display;
extern struct win *fore;
extern int real_uid, eff_uid;
extern int real_gid, eff_gid;
extern char *extra_incap, *extra_outcap;
extern char *home, *RcFileName;
extern char SockPath[], *SockNamePtr;
#ifdef COPY_PASTE
extern char *BufferFile;
#endif
extern int hardcopy_append;
extern char *hardcopydir;

static char *CatExtra __P((char *, char *));


static FILE *fp = NULL;
char *rc_name;

static char *
CatExtra(str1, str2)
register char *str1, *str2;
{
  register char *cp;
  register int len1, len2, add_colon;

  len1 = strlen(str1);
  if (len1 == 0)
    return(str2);
  add_colon = (str1[len1 - 1] != ':');
  if (str2)
    {
      len2 = strlen(str2);
      if ((cp = realloc(str2, (unsigned) len1 + len2 + add_colon + 1)) == NULL)
	Panic(0, strnomem);
      bcopy(cp, cp + len1 + add_colon, len2 + 1);
    }
  else
    {
      if (len1 == 0)
	return 0;
      if ((cp = malloc((unsigned) len1 + add_colon + 1)) == NULL)
	Panic(0, strnomem);
      cp[len1 + add_colon] = '\0'; 
    }
  bcopy(str1, cp, len1);
  if (add_colon)
    cp[len1] = ':';

  return cp;
}

static char *
findrcfile(rcfile)
char *rcfile;
{
  static char buf[256];
  char *rc, *p;

  if (rcfile)
    {
      rc = SaveStr(rcfile);
      debug1("findrcfile: you specified '%s'\n", rcfile);
    }
  else
    {
      debug("findrcfile: you specified nothing...\n");
      if ((p = getenv("ISCREENRC")) != NULL && *p != '\0')
	{
	  debug1("  ... but $ISCREENRC has: '%s'\n", p);
	  rc = SaveStr(p);
	}
      else if ((p = getenv("SCREENRC")) != NULL && *p != '\0')
	{
	  debug1("  ... but $SCREENRC has: '%s'\n", p);
	  rc = SaveStr(p);
	}
      else
	{
	  debug("  ...nothing in $SCREENRC, defaulting $HOME/.screenrc\n");
	  if (strlen(home) > 244)
	    Panic(0, "Rc: home too large");
	  sprintf(buf, "%s/.iscreenrc", home);
          if (access(buf, R_OK))
	    sprintf(buf, "%s/.screenrc", home);
	  rc = SaveStr(buf);
	}
    }
  return rc;
}

/*
 * this will be called twice:
 * 1) rcfilename = "/etc/screenrc"
 * 2) rcfilename = RcFileName
 */
void
StartRc(rcfilename)
char *rcfilename;
{
  register int argc, len;
  register char *p, *cp;
  char buf[256];
  char *args[MAXARGS];

  rc_name = findrcfile(rcfilename);

  if ((fp = secfopen(rc_name, "r")) == NULL)
    {
      if (RcFileName && strcmp(RcFileName, rc_name) == 0)
	{
          /*
           * User explicitly gave us that name,
           * this is the only case, where we get angry, if we can't read
           * the file.
           */
	  debug3("StartRc: '%s','%s', '%s'\n", RcFileName, rc_name, rcfilename);
          Panic(0, "Unable to open \"%s\".", rc_name);
	  /* NOTREACHED */
	}
      debug1("StartRc: '%s' no good. ignored\n", rc_name);
      Free(rc_name);
      rc_name = "";
      return;
    }
  while (fgets(buf, sizeof buf, fp) != NULL)
    {
      if ((p = rindex(buf, '\n')) != NULL)
	*p = '\0';
      if ((argc = Parse(buf, args)) == 0)
	continue;
      if (strcmp(args[0], "echo") == 0)
	{
	  if (!display)
	    continue;
	  if (argc < 2 || (argc == 3 && strcmp(args[1], "-n")) || argc > 3)
	    {
	      Msg(0, "%s: 'echo [-n] \"string\"' expected.", rc_name);
	      continue;
	    }
	  AddStr(args[argc - 1]);
	  if (argc != 3)
	    {
	      AddStr("\r\n");
	      Flush();
	    }
	}
      else if (strcmp(args[0], "sleep") == 0)
	{
	  if (!display)
	    continue;
	  debug("sleeeeeeep\n");
	  if (argc != 2)
	    {
	      Msg(0, "%s: sleep: one numeric argument expected.", rc_name);
	      continue;
	    }
	  DisplaySleep(atoi(args[1]));
	}
#ifdef TERMINFO
      else if (strcmp(args[0], "terminfo") == 0)
#else
      else if (strcmp(args[0], "termcap") == 0)
#endif
	{
	  if (!display)
	    continue;
	  if (argc < 3 || argc > 4)
	    {
	      Msg(0, "%s: %s: incorrect number of arguments.", rc_name, args[0]);
	      continue;
	    }
	  for (p = args[1]; p && *p; p = cp)
	    {
	      if ((cp = index(p, '|')) != 0)
		*cp++ = '\0';
	      len = strlen(p);
	      if (p[len - 1] == '*')
		{
		  if (!(len - 1) || !strncmp(p, d_termname, len - 1))
		    break;
		}
	      else if (!strcmp(p, d_termname))
		break;
	    }
	  if (!(p && *p))
	    continue;
	  extra_incap = CatExtra(args[2], extra_incap);
	  if (argc == 4)
	    extra_outcap = CatExtra(args[3], extra_outcap);
	}
    }
  fclose(fp);
  Free(rc_name);
  rc_name = "";
}

void
FinishRc(rcfilename)
char *rcfilename;
{
  char buf[256];

  rc_name = findrcfile(rcfilename);

  if ((fp = secfopen(rc_name, "r")) == NULL)
    {
      if (RcFileName && strcmp(RcFileName, rc_name) == 0)
	{
    	  /*
 	   * User explicitly gave us that name, 
	   * this is the only case, where we get angry, if we can't read
	   * the file.
	   */
  	  debug3("FinishRc:'%s','%s','%s'\n", RcFileName, rc_name, rcfilename);
          Panic(0, "Unable to open \"%s\".", rc_name);
	  /* NOTREACHED */
	}
      debug1("FinishRc: '%s' no good. ignored\n", rc_name);
      Free(rc_name);
      rc_name = "";
      return;
    }

  debug("finishrc is going...\n");
  while (fgets(buf, sizeof buf, fp) != NULL)
    {
      RcLine(buf);
    }
  (void) fclose(fp);
  Free(rc_name);
  rc_name = "";
}

/*
 *	"$HOST blafoo"   	-> "localhost blafoo"
 *	"${HOST}blafoo"	  	-> "localhostblafoo"
 *	"\$HOST blafoo" 	-> "$HOST blafoo"
 *	"\\$HOST blafoo"	-> "\localhost blafoo"
 *	"'$HOST ${HOST}'"	-> "'$HOST ${HOST}'" 
 *	"'\$HOST'"       	-> "'\$HOST'"
 *	"\'$HOST' $HOST"   	-> "'localhost' $HOST"
 *
 *	"$:termcapname:"	-> "termcapvalue"
 *	"$:terminfoname:"	-> "termcapvalue"
 *
 *	"\101"			-> "A"
 */
char *
expand_vars(ss)
char *ss;
{
  static char ebuf[2048];
  register int esize = 2047, vtype, quofl = 0;
  register char *e = ebuf;
  register char *s = ss;
  register char *v;
  char xbuf[11];

  while (*s && *s != '\0' && *s != '\n' && esize > 0)
    {
      if (*s == '\'')
	quofl ^= 1;
      if (*s == '$' && !quofl)
	{
	  char *p, c;

	  p = ++s;
	  switch (*s)
	    {
	    case '{':
	      p = ++s;
	      while (*p != '}')
	        if (*p++ == '\0')
	          return ss;
	      vtype = 0;		/* env var */
	      break;
	    case ':':
	      p = ++s;
	      while (*p != ':')
		if (*p++ == '\0')
		  return ss;
	      vtype = 1;		/* termcap string */
	      break;
	    default:
	      while (*p != ' ' && *p != '\0' && *p != '\n')
		p++;
	      vtype = 0;		/* env var */
	    }
	  c = *p;
	  debug1("exp: c='%c'\n", c);
	  *p = '\0';
	  if (vtype == 0)
	    {
	      v = xbuf;
	      if (strcmp(s, "TERM") == 0)
		v = display ? d_termname : "unknown";
	      else if (strcmp(s, "COLUMNS") == 0)
		sprintf(xbuf, "%d", display ? d_width : -1);
	      else if (strcmp(s, "LINES") == 0)
		sprintf(xbuf, "%d", display ? d_height : -1);
	      else
		v = getenv(s);
	    }
	  else
	    v = gettermcapstring(s);
	  if (v)
	    {
	      debug2("exp: $'%s'='%s'\n", s, v);
	      while (*v && esize-- > 0)
	        *e++ = *v++;
	    }
	  else 
	    debug1("exp: '%s' not env\n", s);  /* '{'-: */
	  if ((*p = c) == '}' || c == ':')
	    p++;
	  s = p;
	}
      else
	{
	  /*
	   * \$, \\$, \\, \\\, \012 are reduced here, 
	   * d_other sequences starting whith \ are passed through.
	   */
	  if (s[0] == '\\' && !quofl)
	    {
	      if (s[1] >= '0' && s[1] <= '7')
		{
		  int i;

		  s++;
		  i = *s - '0';
		  s++;
		  if (*s >= '0' && *s <= '7')
		    {
		      i = i * 8 + *s - '0';
		      s++;
		      if (*s >= '0' && *s <= '7')
			{
			  i = i * 8 + *s - '0';
			  s++;
			}
		    }
		  debug2("expandvars: octal coded character %o (%d)\n", i, i);
		  *e++ = i;
		}
	      else
		{
		  if (s[1] == '$' || 
		      (s[1] == '\\' && s[2] == '$') ||
		      s[1] == '\'' || 
		      (s[1] == '\\' && s[2] == '\''))
		    s++;
		}
	    }
	  *e++ = *s++;
	  esize--;
	}
    }
  if (esize <= 0)
    Msg(0, "expand_vars: buffer overflow\n");
  *e = '\0';
  debug1("expand_var returns '%s'\n", ebuf);
  return ebuf;
}

void
RcLine(ubuf)
char *ubuf;
{
  char *args[MAXARGS], *buf;

  buf = expand_vars(ubuf); 
  if (Parse(buf, args) <= 0)
    return;
  DoCommand(args);
}

void
WriteFile(dump)
int dump;
{
  /* dump==0:	create .termcap,
   * dump==1:	hardcopy,
   * #ifdef COPY_PASTE
   * dump==2:	BUFFERFILE
   * #endif COPY_PASTE 
   */
  register int i, j, k;
  register char *p;
  register FILE *f;
  char fn[1024];
  char *mode = "w";

  switch (dump)
    {
    case DUMP_TERMCAP:
      i = SockNamePtr - SockPath;
      strncpy(fn, SockPath, i);
      strcpy(fn + i, ".termcap");
      break;
    case DUMP_HARDCOPY:
      if (hardcopydir)
	sprintf(fn, "%s/hardcopy.%d", hardcopydir, fore->w_number);
      else
        sprintf(fn, "hardcopy.%d", fore->w_number);
      if (hardcopy_append && !access(fn, W_OK))
	mode = "a";
      break;
#ifdef COPY_PASTE
    case DUMP_EXCHANGE:
      sprintf(fn, "%s", BufferFile);
      umask(0);
      break;
#endif
    }

  debug2("WriteFile(%d) %s\n", dump, fn);
  if (UserContext() > 0)
    {
      debug("Writefile: usercontext\n");
      if ((f = fopen(fn, mode)) == NULL)
	{
	  debug2("WriteFile: fopen(%s,\"%s\") failed\n", fn, mode);
	  UserReturn(0);
	}
      else
	{
	  switch (dump)
	    {
	    case DUMP_HARDCOPY:
	      if (*mode == 'a')
		{
		  putc('>', f);
		  for (j = d_width - 2; j > 0; j--)
		    putc('=', f);
		  fputs("<\n", f);
		}
	      for (i = 0; i < d_height; i++)
		{
		  p = fore->w_image[i];
		  for (k = d_width - 1; k >= 0 && p[k] == ' '; k--)
		    ;
		  for (j = 0; j <= k; j++)
		    putc(p[j], f);
		  putc('\n', f);
		}
	      break;
	    case DUMP_TERMCAP:
	      if ((p = index(MakeTermcap(fore->w_aflag), '=')) != NULL)
		{
		  fputs(++p, f);
		  putc('\n', f);
		}
	      break;
#ifdef COPY_PASTE
	    case DUMP_EXCHANGE:
	      p = d_user->u_copybuffer;
	      for (i = 0; i < d_user->u_copylen; i++)
		putc(*p++, f);
	      break;
#endif
	    }
	  (void) fclose(f);
	  UserReturn(1);
	}
    }
  if (UserStatus() <= 0)
    Msg(0, "Cannot open \"%s\"", fn);
  else
    {
      switch (dump)
	{
	case DUMP_TERMCAP:
	  Msg(0, "Termcap entry written to \"%s\".", fn);
	  break;
	case DUMP_HARDCOPY:
	  Msg(0, "Screen image %s to \"%s\".",
	      (*mode == 'a') ? "appended" : "written", fn);
	  break;
#ifdef COPY_PASTE
	case DUMP_EXCHANGE:
	  Msg(0, "Copybuffer written to \"%s\".", fn);
#endif
	}
    }
}

#ifdef COPY_PASTE

void
ReadFile()
{
  int i, l, size;
  char fn[1024], c;
  struct stat stb;

  sprintf(fn, "%s", BufferFile);
  debug1("ReadFile(%s)\n", fn);
  if ((i = secopen(fn, O_RDONLY, 0)) < 0)
    {
      Msg(errno, "no %s -- no slurp", fn);
      return;
    }
  if (fstat(i, &stb))
    {
      Msg(errno, "no good %s -- no slurp", fn);
      close(i);
      return;
    }
  size = stb.st_size;
  if (d_user->u_copybuffer)
    UserFreeCopyBuffer(d_user);
  d_user->u_copylen = 0;
  if ((d_user->u_copybuffer = malloc(size)) == NULL)
    {
      close(i);
      Msg(0, strnomem);
      return;
    }
  errno = 0;
  if ((l = read(i, d_user->u_copybuffer, size)) != size)
    {
      d_user->u_copylen = (l > 0) ? l : 0;
#ifdef NETHACK
      if (nethackflag)
        Msg(errno, "You choke on your food: %d bytes from %s", 
	    d_user->u_copylen, fn);
      else
#endif
      Msg(errno, "Got only %d bytes from %s", d_user->u_copylen, fn);
      close(i);
      return;
    }
  d_user->u_copylen = l;
  if (read(i, &c, 1) > 0)
    Msg(0, "Slurped only %d characters into buffer - try again", d_user->u_copylen);
  else
    Msg(0, "Slurped %d characters into buffer", d_user->u_copylen);
  close(i);
  return;
}

void
KillBuffers()
{
  char fn[1024];
  sprintf(fn, "%s", BufferFile);
  errno = 0;
#ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
#else
  if (access(fn, W_OK) == -1)
    {
      Msg(errno, "%s not removed", fn);
      return;
    }
#endif
  unlink(fn);
  Msg(errno, "%s removed", fn);
#ifndef NOREUID
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
#endif
}
#endif	/* COPY_PASTE */


/*
 * (Almost) secure open and fopen...
 */

FILE *
secfopen(name, mode)
char *name;
char *mode;
{
  FILE *fi;
#ifdef NOREUID
  int flags, fd;
#endif

  debug2("secfopen(%s, %s)\n", name, mode);
#ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
  fi = fopen(name, mode);
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
  return fi;
#else
  if (eff_uid == real_uid)
    return(fopen(name, mode));
  if (mode[0] && mode[1] == '+')
    flags = O_RDWR;
  else
    flags = (mode[0] == 'r') ? O_RDONLY : O_WRONLY;
  if (mode[0] == 'w')
    flags |= O_CREAT | O_TRUNC;
  else if (mode[0] == 'a')
    flags |= O_CREAT | O_APPEND;
  else if (mode[0] != 'r')
    {
      errno = EINVAL;
      return(0);
    }
  if ((fd = secopen(name, flags, 0666)) < 0)
    return(0);
  if ((fi = fdopen(fd, mode)) == 0)
    {
      close(fd);
      return(0);
    }
  return(fi);
#endif
}


int
secopen(name, flags, mode)
char *name;
int flags;
int mode;
{
  int fd;
#ifdef NOREUID
  int q;
  struct stat stb;
#endif

  debug3("secopen(%s, 0x%x, 0%03o)\n", name, flags, mode);
#ifndef NOREUID
  setreuid(eff_uid, real_uid);
  setregid(eff_gid, real_gid);
  fd = open(name, flags, mode);
  setreuid(real_uid, eff_uid);
  setregid(real_gid, eff_gid);
  return fd;
#else
  if (eff_uid == real_uid)
    return(open(name, flags, mode));
  /* Truncation/creation is done in UserContext */
  if ((flags & O_TRUNC) || ((flags & O_CREAT) && access(name, F_OK)))
    {
      if (UserContext() > 0)
	{
          if ((fd = open(name, flags, mode)) >= 0)
	    {
	      close(fd);
	      UserReturn(0);
            }
	  if (errno == 0)
	    errno = EACCES;
	  UserReturn(errno);
	}
      if (q = UserStatus())
	{
	  if (q > 0)
	    errno = q;
          return(-1);
	}
    }
  if (access(name, F_OK))
    return(-1);
  if ((fd = open(name, flags & ~(O_TRUNC | O_CREAT), 0)) < 0)
    return(-1);
  debug("open successful\n");
  if (fstat(fd, &stb))
    {
      close(fd);
      return(-1);
    }
  debug("fstat successful\n");
  if (stb.st_uid != real_uid)
    {
      switch (flags & (O_RDONLY | O_WRONLY | O_RDWR))
        {
	case O_RDONLY:
	  q = 0004;
	  break;
	case O_WRONLY:
	  q = 0002;
	  break;
	default:
	  q = 0006;
	  break;
        }
      if ((stb.st_mode & q) != q)
	{
          debug("secopen: permission denied\n");
	  close(fd);
	  errno = EACCES;
	  return(-1);
	}
    }
  debug1("secopen ok - returning %d\n", fd);
  return(fd);
#endif
}
