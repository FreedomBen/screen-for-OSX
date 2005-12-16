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

#include "config.h"
#include "screen.h"	/* includes acls.h */
#include "extern.h"


/************************************************************************
 * user managing code, this does not really belong into the acl stuff   *
 ************************************************************************/

extern struct comm comms[];
extern struct win *windows, *wtab[];
extern char NullStr[];
extern struct display *display, *displays;
struct user *users;

#ifdef MULTIUSER
int maxusercount = 0;	/* used in process.c: RC_MONITOR, RC_SILENCE */

/* record given user ids here */
static AclBits userbits;

/*
 * rights a new unknown user will have on windows and cmds. 
 * These are changed by a "umask ?-..." command: 
 */
static char default_w_bit[ACL_BITS_PER_WIN] = 
{ 
  0,	/* EXEC */
  0, 	/* WRITE */
  0 	/* READ */
};

static char default_c_bit[ACL_BITS_PER_CMD] = 
{ 
  0	/* EXEC */
};

/* rights of all users per newly created window */
/*
 * are now stored per user (umask)
 * static AclBits default_w_userbits[ACL_BITS_PER_WIN];
 * static AclBits default_c_userbits[ACL_BITS_PER_CMD];
 */

static int GrowBitfield __P((AclBits *, int, int, int));



static int
GrowBitfield(bfp, len, delta, defaultbit)
AclBits *bfp;
int len, delta, defaultbit;
{
  AclBits n, o = *bfp;
  int i;

  if (!(n = (AclBits)calloc(1, (unsigned long)(&ACLBYTE((char *)0, len + delta + 1)))))
    return -1;
  for (i = 0; i < (len + delta); i++)
    {
      if (((i <  len) && (ACLBIT(i) & ACLBYTE(o, i))) ||
          ((i >= len) && (defaultbit)))
	ACLBYTE(n, i) |= ACLBIT(i);
    }
  if (len)
    free((char *)o);
  *bfp = n;
  return 0;
}

#endif /* MULTIUSER */

/* 
 * Returns an nonzero Address. Its contents is either a User-ptr, 
 * or NULL which may be replaced by a User-ptr to create the entry.
 */
struct user **
FindUserPtr(name)
char *name;
{
  struct user **u;

  for (u = &users; *u; u = &(*u)->u_next)
    if (!strcmp((*u)->u_name, name))
      break;
#ifdef MULTIUSER
  debug3("FindUserPtr %s %sfound, id %d\n", name, (*u)?"":"not ", 
         (*u)?(*u)->u_id:-1);
#else /* MULTIUSER */
  debug2("FindUserPtr %s %sfound\n", name, (*u)?"":"not ");
#endif /* MULTIUSER */
  return u;
}

int DefaultEsc = -1;		/* initialised by screen.c:main() */
int DefaultMetaEsc = -1;

/*
 * Add a new user. His password may be NULL or "" if none. His name must not 
 * be "none", as this represents the NULL-pointer when dealing with groups.
 * He has default rights, determined by umask.
 */
int
UserAdd(name, pass, up)
char *name, *pass;
struct user **up;
{
#ifdef MULTIUSER
  int j;
#endif

  if (!up)
    up = FindUserPtr(name);
  if (*up)
    {
      if (pass)
        (*up)->u_password = SaveStr(pass);
      return 1;		/* he is already there */
    }
  if (strcmp("none", name))	/* "none" is a reserved word */
    *up = (struct user *)calloc(1, sizeof(struct user));
  if (!*up)
    return -1;		/* he still does not exist */
#ifdef COPY_PASTE
  (*up)->u_copybuffer = NULL;
  (*up)->u_copylen = 0;
#endif
  (*up)->u_Esc = DefaultEsc;
  (*up)->u_MetaEsc = DefaultMetaEsc;
  strncpy((*up)->u_name, name, 20);
  (*up)->u_password = NULL;
  if (pass)
    (*up)->u_password = SaveStr(pass);
  if (!(*up)->u_password)
    (*up)->u_password = NullStr;

#ifdef MULTIUSER
  (*up)->u_group = NULL;
  /* now find an unused index */
  for ((*up)->u_id = 0; (*up)->u_id < maxusercount; (*up)->u_id++)
    if (!(ACLBIT((*up)->u_id) & ACLBYTE(userbits, (*up)->u_id)))
      break;
  debug2("UserAdd %s id %d\n", name, (*up)->u_id);
  if ((*up)->u_id == maxusercount)
    {
      int j;
      struct win *w;
      struct user *u;

      debug2("growing all bitfields %d += %d\n", maxusercount, USER_CHUNK);
      /* the bitfields are full, grow a chunk */
      /* first, the used_uid_indicator: */
      if (GrowBitfield(&userbits, maxusercount, USER_CHUNK, 0))
        {
	  free((char *)*up); *up = NULL; return -1;
	}
      /* second, default command bits  */
      /* (only if we generate commands dynamically) */
/*
      for (j = 0; j < ACL_BITS_PER_CMD; j++)
	if (GrowBitfield(&default_c_userbits[j], maxusercount, USER_CHUNK, 
	    default_c_bit[j]))
	  {
	    free((char *)*up); *up = NULL; return -1;
	  }
*/
      /* third, the bits for each commands */
      for (j = 0; j <= RC_LAST; j++)
        {
	  int i;
	  
	  for (i = 0; i < ACL_BITS_PER_CMD; i++)
	    if (GrowBitfield(&comms[j].userbits[i], maxusercount, USER_CHUNK,
	        default_c_bit[i]))
	      {
	        free((char *)*up); *up = NULL; return -1;
	      }
        }
      /* fourth, default window creation bits per user */
      for (u = users; u != *up; u = u->u_next)
        {
	  for (j = 0; j < ACL_BITS_PER_WIN; j++)
	    {
	      if (GrowBitfield(&u->u_umask_w_bits[j], maxusercount, USER_CHUNK,
		  default_w_bit[j]))
		{
		  free((char *)*up); *up = NULL; return -1;
		}
	    }
	}

      /* fifth, the bits for each window */
      /* keep these in sync with NewWindowAcl() */
      for (w = windows; w; w = w->w_next)
	{
	  /* five a: the access control list */
	  for (j = 0; j < ACL_BITS_PER_WIN; j++)
	    if (GrowBitfield(&w->w_userbits[j], maxusercount, USER_CHUNK,
		default_w_bit[j]))
	      {
		free((char *)*up); *up = NULL; return -1;
	      }
	  /* five b: the activity notify list */
	  /* five c: the silence notify list */
	  if (GrowBitfield(&w->w_mon_notify, maxusercount, USER_CHUNK, 0) ||
	      GrowBitfield(&w->w_lio_notify, maxusercount, USER_CHUNK, 0))
	    {  
	      free((char *)*up); *up = NULL; return -1; 
	    }
	}
      maxusercount += USER_CHUNK;
    }

  /* mark the user-entry as "in-use" */
  ACLBYTE(userbits, (*up)->u_id) |= ACLBIT((*up)->u_id);    

  /* user id 0 is the session creator, he has all rights */
  if ((*up)->u_id == 0)
    AclSetPerm(NULL, *up, "+a", "#?");
  
  /* user nobody has a fixed set of rights: */
  if (!strcmp((*up)->u_name, "nobody"))
    {
      AclSetPerm(NULL, *up, "-rwx", "#?");
      AclSetPerm(NULL, *up, "+x", "su");
      AclSetPerm(NULL, *up, "+x", "detach");
      AclSetPerm(NULL, *up, "+x", "displays");
      AclSetPerm(NULL, *up, "+x", "version");
    }

  /* 
   * Create his umask:
   * Give default_w_bit's for all users, 
   * but allow himself everything on "his" windows.
   */
  for (j = 0; j < ACL_BITS_PER_WIN; j++)
    {
      if (GrowBitfield(&(*up)->u_umask_w_bits[j], 0, maxusercount, 
          default_w_bit[j]))
        {
          free((char *)*up); *up = NULL; return -1;
        }
      ACLBYTE((*up)->u_umask_w_bits[j], (*up)->u_id) |= ACLBIT((*up)->u_id);
    }
#else /* MULTIUSER */
  debug1("UserAdd %s\n", name);
#endif /* MULTIUSER */
  return 0;
}

/* 
 * Remove a user from the list. 
 * Destroy all his permissions and completely detach him from the session.
 */
int 
UserDel(name, up)
char *name;
struct user **up;
{
  struct user *u;
#ifdef MULTIUSER
  int i;
#endif
  struct display *old, *next;

  if (!up)
    up = FindUserPtr(name);
  if (!(u = *up))
    return -1;			/* he who does not exist cannot be removed */
  old = display;
  for (display = displays; display; display = next)
    {
      next = display->d_next;	/* read the next ptr now, Detach may zap it. */
      if (D_user != u)
	continue;
      if (display == old)
	old = NULL;
      Detach(D_REMOTE);
    }
  display = old;
  *up = u->u_next;

#ifdef MULTIUSER
  for (up = &users; *up; up = &(*up)->u_next)
    {
      /* unlink all group references to this user */
      struct usergroup **g = &(*up)->u_group;

      while (*g)
        {
	  if ((*g)->u == u)
	    {
	      struct usergroup *next = (*g)->next;

	      free((char *)(*g));
	      *g = next;
	    }
	  else
	    g = &(*g)->next;
	}
    }
  ACLBYTE(userbits, u->u_id) &= ~ACLBIT(u->u_id);
  /* restore the bits in his slot to default: */
  AclSetPerm(NULL, u, default_w_bit[ACL_READ] ? "+r" : "-r", "#");
  AclSetPerm(NULL, u, default_w_bit[ACL_WRITE]? "+w" : "-w", "#");
  AclSetPerm(NULL, u, default_w_bit[ACL_EXEC] ? "+x" : "-x", "#");
  AclSetPerm(NULL, u, default_c_bit[ACL_EXEC] ? "+x" : "-x", "?");
  for (i = 0; i < ACL_BITS_PER_WIN; i++)
    free((char *)u->u_umask_w_bits[i]);
#endif /* MULTIUSER */
  debug1("FREEING user structure for %s\n", u->u_name);
#ifdef COPY_PASTE
  UserFreeCopyBuffer(u);
#endif
  free((char *)u);
  if (!users)
    {
      debug("Last user deleted. Feierabend.\n");
      Finit(0);	/* Destroying whole session. Noone could ever attach again. */
    }
  return 0;
}


#ifdef COPY_PASTE

/*
 * returns 0 if the copy buffer was really deleted.
 * Also removes any references into the users copybuffer
 */
int
UserFreeCopyBuffer(u)
struct user *u;
{
  struct win *w;

  if (!u->u_copybuffer)
    return 1;
  for (w = windows; w; w = w->w_next)
    {
      if (w->w_pasteptr >= u->u_copybuffer &&
          w->w_pasteptr - u->u_copybuffer < u->u_copylen)
	{
	  if (w->w_pastebuf)
            free((char *)w->w_pastebuf);
          w->w_pastebuf = 0;
	  w->w_pasteptr = 0;
	  w->w_pastelen = 0;
	}
    }
  free((char *)u->u_copybuffer);
  u->u_copylen = 0;
  u->u_copybuffer = NULL;
  return 0;
}
#endif	/* COPY_PASTE */

/************************************************************************
 *                     end of user managing code                        *
 ************************************************************************/


#ifdef MULTIUSER

/* This gives the users default rights to the new window w created by u */
int
NewWindowAcl(w, u)
struct win *w;
struct user *u;
{
  int i, j;

  debug2("NewWindowAcl %s's umask_w_bits for window %d\n", 
         u ? u->u_name : "everybody", w->w_number);

  /* keep these in sync with UserAdd part five. */
  if (GrowBitfield(&w->w_mon_notify, 0, maxusercount, 0) ||
      GrowBitfield(&w->w_lio_notify, 0, maxusercount, 0))
    return -1;
  for (j = 0; j < ACL_BITS_PER_WIN; j++)
    {
      /* we start with len 0 for the new bitfield size and add maxusercount */
      if (GrowBitfield(&w->w_userbits[j], 0, maxusercount, 0))
	{
	  while (--j >= 0)
	    free((char *)w->w_userbits[j]);
	  return -1;
	}
      for (i = 0; i < maxusercount; i++)
        if (u ? (ACLBIT(i) & ACLBYTE(u->u_umask_w_bits[j], i)) : 
	        default_w_bit[j])
	  ACLBYTE(w->w_userbits[j], i) |= ACLBIT(i);
    }
  return 0;
}

/* if mode starts with '-' we remove the users exec bit for cmd */
/*
 * NOTE: before you make this function look the same as 
 * AclSetPermWin, try to merge both functions. 
 */
static int
AclSetPermCmd(u, mode, cmd)
struct user *u;
char *mode;
struct comm *cmd;
{
  int neg = 0;
  char *m = mode;

  while (*m)
    {
      switch (*m++)
        {
	case '-': 
	  neg = 1;
	  continue;
        case '+':
	  neg = 0;
	  continue;
        case 'a':
        case 'e': 
        case 'x': 
/*	  debug3("AclSetPermCmd %s %s %s\n", u->u_name, mode, cmd->name); */
	  if (neg)
	    ACLBYTE(cmd->userbits[ACL_EXEC], u->u_id) &= ~ACLBIT(u->u_id);
	  else
	    ACLBYTE(cmd->userbits[ACL_EXEC], u->u_id) |= ACLBIT(u->u_id);
	  break;
        case 'r':
	case 'w':
	  break;
        default:
	  return -1;
	}
    }
  return 0;
}

/* mode strings of the form +rwx -w+rx r -wx are parsed and evaluated */
/*
 * aclchg nerd -w+w 2
 * releases a writelock on window 2 held by user nerd.
 * Letter n allows network access on a window.
 * uu should be NULL, except if you want to change his umask.
 */
static int
AclSetPermWin(uu, u, mode, win)
struct user *u, *uu;
char *mode;
struct win *win;
{
  int neg = 0;
  int bit, bits;
  AclBits *bitarray;
  char *m = mode;

  if (uu)
    {
      debug3("AclSetPermWin %s UMASK %s %s\n", uu->u_name, u->u_name, mode);
      bitarray = uu->u_umask_w_bits;
    }
  else
    {
      ASSERT(win);
      bitarray = win->w_userbits;
      debug3("AclSetPermWin %s %s %d\n", u->u_name, mode, win->w_number);
    }

  while (*m)
    {
      switch (*m++)
        {
	case '-': 
	  neg = 1;
	  continue;
        case '+':
	  neg = 0;
	  continue;
        case 'r': 
	  bits = (1 << ACL_READ);
	  break;
	case 'w':
	  bits = (1 << ACL_WRITE);
	  break;
        case 'x':
	  bits = (1 << ACL_EXEC);
	  break;
        case 'a':
	  bits = (1 << ACL_BITS_PER_WIN) - 1;
	  break;
	default:
	  return -1;
        }
      for (bit = 0; bit < ACL_BITS_PER_WIN; bit++)
	{  
	  if (!(bits & (1 << bit)))
	    continue;
	  if (neg)
	    ACLBYTE(bitarray[bit], u->u_id) &= ~ACLBIT(u->u_id);
	  else
	    ACLBYTE(bitarray[bit], u->u_id) |= ACLBIT(u->u_id);
	  if (!uu && (win->w_wlockuser == u) && neg && (bit == ACL_WRITE))
	    {
	      debug2("%s lost writelock on win %d\n", u->u_name, win->w_number);
	      win->w_wlockuser = NULL;
	      if (win->w_wlock == WLOCK_ON)
		win->w_wlock = WLOCK_AUTO;
	    }
	}
    }
  if (uu && u->u_name[0] == '?' && u->u_name[1] == '\0')
    {
      /* 
       * It is Mr. '?', the unknown user. He deserves special treatment as
       * he defines the defaults. Sorry, this is global, not per user.
       */
      if (win)
        {
	  debug1("AclSetPermWin: default_w_bits '%s'.\n", mode);
	  for (bit = 0; bit < ACL_BITS_PER_WIN; bit++)
	    default_w_bit[bit] = 
	      (ACLBYTE(bitarray[bit], u->u_id) & ACLBIT(u->u_id)) ? 1 : 0;
	}
      else
	{
	  /*
	   * Hack. I do not want to duplicate all the above code for
	   * AclSetPermCmd. This asumes that there are not more bits 
	   * per cmd than per win.
	   */
	  debug1("AclSetPermWin: default_c_bits '%s'.\n", mode);
	  for (bit = 0; bit < ACL_BITS_PER_CMD; bit++)
	    default_c_bit[bit] = 
	      (ACLBYTE(bitarray[bit], u->u_id) & ACLBIT(u->u_id)) ? 1 : 0;
	}
      UserDel(u->u_name, NULL);
    }
  return 0;
}

/* 
 * String is broken down into comand and window names, mode applies
 * A command name matches first, so do not use these as window names.
 * uu should be NULL, except if you want to change his umask.
 */
int
AclSetPerm(uu, u, mode, s)
struct user *uu, *u;
char *mode, *s;
{
  struct win *w;
  int i;
  char *p, ch;

  debug3("AclSetPerm(uu, user '%s', mode '%s', object '%s')\n",
         u->u_name, mode, s);
  while (*s)
    {
      switch (*s)
	{
	case '*':			/* all windows and all commands */
	  return AclSetPerm(uu, u, mode, "#?");
	case '#':
	  if (uu)			/* window umask or .. */
	    AclSetPermWin(uu, u, mode, (struct win *)1);
	  else				/* .. or all windows */
	    for (w = windows; w; w = w->w_next)
	      AclSetPermWin(NULL, u, mode, w);
	  s++;
	  break;
	case '?':
	  if (uu)			/* command umask or .. */
	    AclSetPermWin(uu, u, mode, (struct win *)0);
	  else				/* .. or all commands */
	    for (i = 0; i <= RC_LAST; i++)
	      AclSetPermCmd(u, mode, &comms[i]);
	  s++;
	  break;
	default:
	  for (p = s; *p && *p != ' ' && *p != '\t' && *p != ','; p++)
	    ;
	  if ((ch = *p))
	    *p++ = '\0';
	  if ((i = FindCommnr(s)) != RC_ILLEGAL)
	    AclSetPermCmd(u, mode, &comms[i]);
	  else if (((i = WindowByNoN(s)) >= 0) && wtab[i])
	    AclSetPermWin(NULL, u, mode, wtab[i]);
	  else
	    /* checking group name */
	    return -1;
	  if (ch)
	    p[-1] = ch;
	  s = p; 
	}    
    }
  return 0;
}

/* 
 * Generic ACL Manager:
 *
 * This handles acladd and aclchg identical.
 * With 2 or 4 parameters, the second parameter is a password.
 * With 3 or 4 parameters the last two parameters specify the permissions
 *   else user is added with full permissions.
 * With 1 parameter the users permissions are copied from user *argv.
 *   Unlike the other cases, u->u_name should not match *argv here.
 * uu should be NULL, except if you want to change his umask.
 */
static int
UserAcl(uu, u, argc, argv)
struct user *uu, **u;
int argc;
char **argv;
{
  if ((*u && !strcmp((*u)->u_name, "nobody")) ||
      (argc > 1 && !strcmp(argv[0], "nobody")))
    return -1;			/* do not change nobody! */

  switch (argc)
    {
    case 1+1+2:
      debug2("UserAcl: user '%s', password '%s':", argv[0], argv[1]);
      return (UserAdd(argv[0], argv[1], u) < 0) || 
	      AclSetPerm(uu, *u, argv[2], argv[3]);
    case 1+2:
      debug1("UserAcl: user '%s', no password:", argv[0]);
      return (UserAdd(argv[0],    NULL, u) < 0) || 
	      AclSetPerm(uu, *u, argv[1], argv[2]);
    case 1+1:
      debug2("UserAcl: user '%s', password '%s'\n", argv[0], argv[1]);
      return UserAdd(argv[0], argv[1], u) < 0;
    case 1:
      debug1("UserAcl: user '%s', no password:", argv[0]);
      return (UserAdd(argv[0],    NULL, u) < 0) || 
	      AclSetPerm(uu, *u, "+a", "#?");
    default:
      return -1;
    }
}

static int
UserAclCopy(to_up, from_up)
struct user **to_up, **from_up;
{
  struct win *w;
  int i, j, to_id, from_id;

  if (!*to_up || !*from_up)
    return -1;
  debug2("UserAclCopy: from user '%s' to user '%s'\n", 
         (*from_up)->u_name, (*to_up)->u_name);
  if ((to_id = (*to_up)->u_id) == (from_id = (*from_up)->u_id))
    return -1;
  for (w = windows; w; w = w->w_next)
    {
      for (i = 0; i < ACL_BITS_PER_WIN; i++)
        {
	  if (ACLBYTE(w->w_userbits[i], from_id) & ACLBIT(from_id))
	    ACLBYTE(w->w_userbits[i], to_id) |= ACLBIT(to_id);
	  else
	    {
	      ACLBYTE(w->w_userbits[i], to_id) &= ~ACLBIT(to_id);
	      if ((w->w_wlockuser == *to_up) && (i == ACL_WRITE))
		{
		  debug2("%s lost wlock on win %d\n", 
		         (*to_up)->u_name, w->w_number);
		  w->w_wlockuser = NULL;
		  if (w->w_wlock == WLOCK_ON)
		    w->w_wlock = WLOCK_AUTO;
		}
	    }
	}
    }
  for (j = 0; j <= RC_LAST; j++)
    {
      for (i = 0; i < ACL_BITS_PER_CMD; i++)
        {
	  if (ACLBYTE(comms[j].userbits[i], from_id) & ACLBIT(from_id))
	    ACLBYTE(comms[j].userbits[i], to_id) |= ACLBIT(to_id);
	  else
	    ACLBYTE(comms[j].userbits[i], to_id) &= ~ACLBIT(to_id);
	}
    }

  return 0;
}

/*
 * Syntax:
 * 	user [password] [+rwx #?]
 * 	* [password] [+rwx #?]
 *      user1,user2,user3 [password] [+rwx #?]
 *	user1,user2,user3=user
 * uu should be NULL, except if you want to change his umask.
 */
int
UsersAcl(uu, argc, argv)
struct user *uu;
int argc;
char **argv;
{
  char *s;
  int r;
  struct user **cf_u = NULL;

  if (argc == 1)
    {
      char *p = NULL;

      s = argv[0]; 
      while (*s)
        if (*s++ == '=') p = s;
      if (p)
        {
          p[-1] = '\0';
          cf_u = FindUserPtr(p);
        }
    }

  if (argv[0][0] == '*' && argv[0][1] == '\0')
    {
      struct user **u;
  
      debug("all users acls.\n");
      for (u = &users; *u; u = &(*u)->u_next)
	if (strcmp("nobody", (*u)->u_name) && 
	    ((cf_u) ?
	     ((r = UserAclCopy(u, cf_u)) < 0) :
	     ((r = UserAcl(uu, u, argc, argv)) < 0)))
	  return -1;
      return 0;
    } 

  do
    {
      for (s = argv[0]; *s && *s!=' ' && *s!='\t' && *s!=',' && *s!='='; s++)
	;
      *s ? (*s++ = '\0') : (*s = '\0');
      debug2("UsersAcl(uu, \"%s\", argc=%d)\n", argv[0], argc);
      if ((cf_u) ?
	  ((r = UserAclCopy(FindUserPtr(argv[0]), cf_u)) < 0) :
	  ((r = UserAcl(uu, FindUserPtr(argv[0]), argc, argv)) < 0))
        return -1;
    } while (*(argv[0] = s));
  return 0;
}

#if 0
void
AclWinSwap(a, b)
int a, b;
{
  int a_bit = 0, b_bit = 0;
  AclGroupList **g;
  AclBits p;

  debug2("acl lists swapping windows %d and %d\n", a, b);
  
  for (g = &aclgrouproot; *g; g = &(*g)->next) 
    {
      p = (*g)->group->winbits;
      /* see if there was a bit for window a. zap it */
      if (a >= 0)
	if ((a_bit = ACLBIT(a) & ACLBYTE(p, a)))
	  ACLBYTE(p, a) &= ~ACLBIT(a);
      /* see if there was a bit for window b. zap it */
      if (b >= 0)
	if ((b_bit = ACLBIT(b) & ACLBYTE(p, b)))
	  ACLBYTE(p, b) &= ~ACLBIT(b);
      /* b may cause a set */
      if (b_bit && a >= 0)
        ACLBYTE(p, a) |= ACLBIT(a);
      /* a may cause b set */
      if (a_bit && b >= 0)
        ACLBYTE(p, b) |= ACLBIT(b);
    }
}
#else
void
AclWinSwap(a, b)
int a, b;
{
  debug2("AclWinSwap(%d, %d) DUMMY\n", a, b);
}
#endif

struct user *EffectiveAclUser = NULL;	/* hook for AT command permission */

int 
AclCheckPermWin(u, mode, w)
struct user *u;
int mode;
struct win *w;
{
  int ok;

  if (mode < 0 || mode >= ACL_BITS_PER_WIN)
    return -1;
  if (EffectiveAclUser)
    {
      debug1("AclCheckPermWin: WARNING user %s overridden!\n", u->u_name);
      u = EffectiveAclUser;
    }
  ok = ACLBYTE(w->w_userbits[mode], u->u_id) & ACLBIT(u->u_id);
  debug3("AclCheckPermWin(%s, %d, %d) = ", u->u_name, mode, w->w_number);

  if (!ok)
    {
      struct usergroup **g = &u->u_group;
      struct user *saved_eff = EffectiveAclUser;

      EffectiveAclUser = NULL;
      while (*g)
        {
	  if (!AclCheckPermWin((*g)->u, mode, w))
	    break;
	  g = &(*g)->next;
	}
      EffectiveAclUser = saved_eff;
      if (*g)
        ok = 1;
    }
  debug1("%d\n", !ok);
  return !ok;
}

int 
AclCheckPermCmd(u, mode, c)
struct user *u;
int mode;
struct comm *c;
{
  int ok;

  if (mode < 0 || mode >= ACL_BITS_PER_CMD)
    return -1;
  if (EffectiveAclUser)
    {
      debug1("AclCheckPermCmd: WARNING user %s overridden!\n", u->u_name);
      u = EffectiveAclUser;
    }
  ok = ACLBYTE(c->userbits[mode], u->u_id) & ACLBIT(u->u_id);
  debug3("AclCheckPermCmd(%s %d %s) = ", u->u_name, mode, c->name); 
  if (!ok)
    {
      struct usergroup **g = &u->u_group;
      struct user *saved_eff = EffectiveAclUser;

      EffectiveAclUser = NULL;
      while (*g)
        {
          if (!AclCheckPermCmd((*g)->u, mode, c))
            break;   
          g = &(*g)->next;
        }                          
      EffectiveAclUser = saved_eff;
      if (*g)
        ok = 1;  
    }
  debug1("%d\n", !ok);
  return !ok;
}

#endif /* MULTIUSER */
