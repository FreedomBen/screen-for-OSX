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
#include "screen.h"	/* includes acl.h */
#include "extern.h"


/************************************************************************
 * user managing code, this does not really belong into the acl stuff   *
 ************************************************************************/

extern struct comm comms[];
extern struct win *windows, *wtab[];
extern struct display *display, *displays;
struct user *users;

#ifdef MULTIUSER
/* record given user ids here */
static AclBits userbits;

/* rights a new unknown user will have on windows and cmds */
static char default_w_bit[ACL_BITS_PER_WIN] = 
{ 
  1,	/* EXEC */
  1, 	/* WRITE */
  1 	/* READ */
};

static char default_c_bit[ACL_BITS_PER_CMD] = 
{ 
  1	/* EXEC */
};

/* rights of all users per newly created window */
static AclBits default_w_userbits[ACL_BITS_PER_WIN];

/*
static AclBits default_c_userbits[ACL_BITS_PER_CMD];
*/

static int maxusercount = 0;

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
    free(o);
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
         (*u)?(*u)->id:-1);
#else
  debug2("FindUserPtr %s %sfound\n", name, (*u)?"":"not ");
#endif
  return u;
}

char DefaultEsc = Ctrl('a');
char DefaultMetaEsc = 'a';

/*
 * Add a new user. His password may be NULL or "" if none. 
 * He is in no groups and thus has no rights.
 */
int
UserAdd(name, pass, up)
char *name, *pass;
struct user **up;
{
  if (!up)
    up = FindUserPtr(name);
  if (*up)
    return 1;		/* he is already there */
  *up = (struct user *)calloc(1, sizeof(struct user));
  if (!*up)
    return -1;		/* he still does not exist */
  (*up)->u_copybuffer = NULL;
  (*up)->u_copylen = 0;
  (*up)->u_Esc = DefaultEsc;
  (*up)->u_MetaEsc = DefaultMetaEsc;
  strncpy((*up)->u_name, name, 20);
  if (pass)
    strncpy((*up)->u_password, pass, 20);

#ifdef MULTIUSER
  /* now find an unused index */
  for ((*up)->id = 0; (*up)->id < maxusercount; (*up)->id++)
    if (!(ACLBIT((*up)->id) & ACLBYTE(userbits, (*up)->id)))
      break;
  debug2("UserAdd %s id %d\n", name, (*up)->id);
  if ((*up)->id == maxusercount)
    {
      int i, j;
      struct win *w;

      debug2("growing all bitfields %d += %d\n", maxusercount, USER_CHUNK);
      /* the bitfields are full, grow a chunk */
      /* first, the used_uid_indicator: */
      if (GrowBitfield(&userbits, maxusercount, USER_CHUNK, 0))
        {
	  free(*up); *up = NULL; return -1;
	}
      /* second, default command bits  */
      /* (only if we generate commands dynamically) */
/*
      for (j = 0; j < ACL_BITS_PER_CMD; j++)
	if (GrowBitfield(&default_c_userbits[j], maxusercount, USER_CHUNK, 
	    default_c_bit[j]))
	  {
	    free(*up); *up = NULL; return -1;
	  }
*/
      /* third, the bits for each commands */
      for (i = 0; i <= RC_LAST; i++)
        for (j = 0; j < ACL_BITS_PER_CMD; j++)
	  if (GrowBitfield(&comms[i].userbits[j], maxusercount, USER_CHUNK,
	      default_c_bit[j]))
	    {
	      free(*up); *up = NULL; return -1;
	    }
      /* fourth, default window and bits */
      for (j = 0; j < ACL_BITS_PER_WIN; j++)
	if (GrowBitfield(&default_w_userbits[j], maxusercount, USER_CHUNK,
	    default_w_bit[j]))
	  {
	    free(*up); *up = NULL; return -1;
	  }
      /* fifth, the bits for each window */
      for (w = windows; w; w = w->w_next)
        for (j = 0; j < ACL_BITS_PER_WIN; j++)
	  if (GrowBitfield(&w->w_userbits[j], maxusercount, USER_CHUNK,
	      default_w_bit[j]))
	    {
	      free(*up); *up = NULL; return -1;
	    }
      maxusercount += USER_CHUNK;
    }
  ACLBYTE(userbits, (*up)->id) |= ACLBIT((*up)->id);    
#else /* MULTIUSER */
  debug1("UserAdd %s\n", name);
#endif /* MULTIUSER */
  return 0;
}

/* change user's password */
int 
UserSetPass(name, pass, up)
char *name, *pass;
struct user **up;
{
  if (!up)
    up = FindUserPtr(name);
  if (!*up)
    return UserAdd(name, pass, up);
  strncpy((*up)->u_password, pass ? pass : "", 20);
  (*up)->u_password[20] = '\0';
  return 0;
}

/* 
 * Remove a user from the list. 
 * Decrease reference count of all his groups
 * Free his grouplist.
 */
int 
UserDel(name, up)
char *name;
struct user **up;
{
  struct user *u;
  struct display *old, *next;

  if (!up)
    up = FindUserPtr(name);
  if (!(u = *up))
    return -1;			/* he who does not exist cannot be removed */
  old = display;
  for (display = displays; display; display = next)
    {
      next = display->_d_next;
      if (d_user != u)
	continue;
      if (display == old)
	old = 0;
      Detach(D_REMOTE);
    }
  display = old;
  *up = u->u_next;
#ifdef MULTIUSER
  ACLBYTE(userbits, u->id) &= ~ACLBIT(u->id);
  AclSetPerm(u, "-rwx", "#?");
#endif
  debug1("FREEING user structure for %s\n", u->u_name);
  UserFreeCopyBuffer(u);
  free(u);
  return 0;
}

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
            free(w->w_pastebuf);
          w->w_pastebuf = 0;
	  w->w_pasteptr = 0;
	  w->w_pastelen = 0;
	}
    }
  free(u->u_copybuffer);
  d_user->u_copylen = 0;
  u->u_copybuffer = NULL;
  return 0;
}

/************************************************************************
 *                     end of user managing code                        *
 ************************************************************************/


#ifdef MULTIUSER

extern char *multi;	/* username */

/* This gives the users default rights to the new window */
int
NewWindowAcl(w)
struct win *w;
{
  int i, j;

  debug1("NewWindowAcl default_w_userbits for window %d\n", w->w_number);
  for (j = 0; j < ACL_BITS_PER_WIN; j++)
    {
      /* we start with len 0 for the new bitfield size and add maxusercount */
      if (GrowBitfield(&w->w_userbits[j], 0, maxusercount, 0))
	{
	  while (--j >= 0)
	    free(w->w_userbits[j]);
	  return -1;
	}
      for (i = 0; i < maxusercount; i++)
        if (ACLBIT(i) & ACLBYTE(default_w_userbits[j], i))
	  ACLBYTE(w->w_userbits[j], i) |= ACLBIT(i);
    }
  return 0;
}

/* if mode starts with '-' we remove the users exec bit for cmd */
int
AclSetPermCmd(u, mode, cmd)
struct user *u;
char *mode;
struct comm *cmd;
{
  int neg = 0;

  if (!multi)
    return 0;
  debug3("AclSetPermCmd %s %s %s\n", u->u_name, mode, cmd->name);
  while (*mode)
    {
      switch (*mode++)
        {
	case '-': 
	  neg = 1;
	  continue;
        case '+':
	  neg = 0;
	  continue;
        case 'e': 
        case 'x': 
	  if (neg)
	    ACLBYTE(cmd->userbits[ACL_EXEC], u->id) &= ~ACLBIT(u->id);
	  else
	    ACLBYTE(cmd->userbits[ACL_EXEC], u->id) |= ACLBIT(u->id);
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
int
AclSetPermWin(u, mode, win)
struct user *u;
char *mode;
struct win *win;
{
  int neg = 0;
  int bit;

  if (!multi)
    return 0;
  debug3("AclSetPermWin %s %s %d\n", u->u_name, mode, win->w_number);
  while (*mode)
    {
      switch (*mode++)
        {
	case '-': 
	  neg = 1;
	  continue;
        case '+':
	  neg = 0;
	  continue;
        case 'r': 
	  bit = ACL_READ;
	  break;
	case 'w':
	  bit = ACL_WRITE;
	  break;
        case 'x':
	  bit = ACL_EXEC;
	  break;
	default:
	  return -1;
        }
      if (neg)
	ACLBYTE(win->w_userbits[bit], u->id) &= ~ACLBIT(u->id);
      else
	ACLBYTE(win->w_userbits[bit], u->id) |= ACLBIT(u->id);
    }
  return 0;
}

/* string is broken down into comand and window names, mode applies */
int
AclSetPerm(u, mode, s)
struct user *u;
char *mode, *s;
{
  struct win *w;
  int i;
  char *p;

  while (*s)
    {
      switch (*s)
	{
	case '*':	/* all windows and all commands */
	  return AclSetPerm(u, mode, "#?");
	case '#':	/* all windows */
	  for (w = windows; w; w = w->w_next)
	    AclSetPermWin(u, mode, w);
	  s++;
	  break;
	case '?':	/* all commands */
	  for (i = 0; i <= RC_LAST; i++)
	    AclSetPermCmd(u, mode, &comms[i]);
	  s++;
	  break;
	default:
	  for (p = s; *p && *p != ' ' && *p != '\t' && *p != ','; p++)
	    ;
	  if (*p)
	    *p++ = '\0';
	  else
	    *p = '\0';
	  if ((i = FindCommnr(s)) != RC_ILLEGAL)
	    AclSetPermCmd(u, mode, &comms[i]);
	  else if (((i = WindowByNoN(s)) >= 0) && wtab[i])
	    AclSetPermWin(u, mode, wtab[i]);
	  else
	    /* checking group name */
	    return -1;
	  s = p; 
	}    
    }
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

int 
AclCheckPermWin(u, mode, w)
struct user *u;
int mode;
struct win *w;
{
  if (!multi)
    return 0;
  if (mode < 0 || mode >= ACL_BITS_PER_WIN)
    return -1;
  debug3("AclCheckPermWin(%s, %d, %d) = ", u->u_name, mode, w->w_number);
  debug1("%d\n", !(ACLBYTE(w->w_userbits[mode], u->id) & ACLBIT(u->id)));
  return         !(ACLBYTE(w->w_userbits[mode], u->id) & ACLBIT(u->id));
}

int 
AclCheckPermCmd(u, mode, c)
struct user *u;
int mode;
struct comm *c;
{
  if (!multi)
    return 0;
  if (mode < 0 || mode >= ACL_BITS_PER_CMD)
    return -1;
  debug3("AclCheckPermCmd(%s %d %s) = ", u->u_name, mode, c->name); 
  debug1("%d\n", !(ACLBYTE(c->userbits[mode], u->id) & ACLBIT(u->id)));
  return         !(ACLBYTE(c->userbits[mode], u->id) & ACLBIT(u->id));
}

#endif /* MULTIUSER */
