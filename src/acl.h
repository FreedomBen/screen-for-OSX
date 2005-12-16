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
 * RCS_ID("$Id$ FAU")
 */

#ifdef MULTIUSER

/* three known bits: */
#define ACL_EXEC 0		
#define ACL_WRITE 1
#define ACL_READ 2

#define ACL_BITS_PER_CMD 1	/* for comm.h */
#define ACL_BITS_PER_WIN 3	/* for window.h */

#define USER_CHUNK 8

#define ACLBYTE(data, w)   ((data)[(w) >> 3])
#define ACLBIT(w)   (0x80 >> ((w) & 7))

typedef unsigned char * AclBits;

/* a bitfield for windows and one for commands */
typedef struct
{
  char name[20 + 1];
  AclBits wins, cmds;
} AclGroup;

/* 
 * An AclGroupList is a chaind list of pointers to AclGroups.
 * Each user has such a list to reference groups he is in.
 * The aclgrouproot anchors all AclGroups. Delete and create
 * groups there.
 */
typedef struct grouplist
{
  AclGroup *group;
  struct grouplist *next;
} AclGroupList;

#endif

/***************
 *  ==> user.h
 */

/*
 * A User has a list of groups, and points to other users.  
 * users is the User entry of the session owner (creator)
 * and anchors all other users. Add/Delete users there.
 */
typedef struct user
{
  struct user *u_next;
  char u_name[20+1];		/* login name how he showed up */
  char u_password[20+1];	/* his password (may be zero length). */
  int  u_detachwin;		/* the window where he last detached */
  char u_Esc, u_MetaEsc;	/* the users screen escape character */
#ifdef COPY_PASTE
  char  *u_copybuffer;
  int   u_copylen;
#endif
#ifdef MULTIUSER
  int id;			/* a uniq index in the bitfields. */
#endif
} User;

extern char DefaultEsc, DefaultMetaEsc;

