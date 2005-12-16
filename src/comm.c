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

#include "config.h"
#include "comm.h"

/* Must be in alpha order ! */

struct comm comms[RC_LAST + 1] =
{
#ifdef MULTIUSER
  { "acladd",		ARGS_ONE },
  { "acldel",		ARGS_ONE },
#endif
  { "activity",		ARGS_ONE },
  { "aka",		NEED_FORE|ARGS_ZEROONE },
  { "allpartial",	ARGS_ONE },
  { "autodetach",	ARGS_ONE },
#ifdef AUTO_NUKE
  { "autonuke",		NEED_DISPLAY|ARGS_ONE },
#endif
  { "bell",		ARGS_ONE },
  { "bind",		ARGS_ONE|ARGS_ORMORE },
  { "break",		NEED_FORE|ARGS_ZEROONE },
#ifdef COPY_PASTE
  { "bufferfile",	ARGS_ZEROONE },
#endif
  { "chdir",		ARGS_ZEROONE },
  { "clear",		NEED_FORE|ARGS_ZERO },
  { "colon",		NEED_DISPLAY|ARGS_ZERO },
  { "console",		NEED_FORE|ARGS_ZEROONE },
#ifdef COPY_PASTE
  { "copy",		NEED_FORE|ARGS_ZERO },
  { "copy_reg",		ARGS_ZEROONE },
  { "crlf",		ARGS_ONE },
#endif
  { "defflow",		ARGS_ONETWO },
#ifdef UTMPOK
  { "deflogin",		ARGS_ONE },
#endif
  { "defmode",		ARGS_ONE },
  { "defmonitor",	ARGS_ONE },
#ifdef COPY_PASTE
  { "defscrollback",	ARGS_ONE },
#endif
  { "defwrap",		ARGS_ONE },
  { "detach",		NEED_DISPLAY|ARGS_ZERO },
  { "dumptermcap",	ARGS_ZERO },
  { "echo",		ARGS_ONETWO },
  { "escape",		ARGS_ONE },
  { "exec", 		NEED_FORE|ARGS_ONE|ARGS_ORMORE },
  { "flow",		NEED_FORE|ARGS_ZEROONE },
  { "hardcopy",		NEED_FORE|ARGS_ZERO },
  { "hardcopy_append",	ARGS_ONE },
  { "hardcopydir",	ARGS_ONE },
  { "hardstatus",	ARGS_ONE },
  { "height",		NEED_DISPLAY|ARGS_ZEROONE },
  { "help",		NEED_DISPLAY|ARGS_ZERO },
#ifdef COPY_PASTE
  { "histnext",		NEED_FORE|ARGS_ZERO },
  { "history",		NEED_FORE|ARGS_ZERO },
#endif
  { "info",		NEED_DISPLAY|ARGS_ZERO },
#ifdef COPY_PASTE
  { "ins_reg",		NEED_DISPLAY|ARGS_ZEROONE },
#endif
  { "kill",		NEED_FORE|ARGS_ZERO },
  { "lastmsg",		NEED_DISPLAY|ARGS_ZERO },
  { "license",		NEED_DISPLAY|ARGS_ZERO },
#ifdef LOCK
  { "lockscreen",	NEED_DISPLAY|ARGS_ZERO },
#endif
  { "log",		ARGS_ZEROONE },
  { "logdir",		ARGS_ONE },
#ifdef UTMPOK
  { "login",		NEED_FORE|ARGS_ZEROONE },
#endif
#ifdef COPY_PASTE
  { "markkeys",		ARGS_ONE },
#endif
  { "meta",		NEED_DISPLAY|ARGS_ZERO },
  { "monitor",		NEED_FORE|ARGS_ZEROONE },
  { "msgminwait",	ARGS_ONE },
  { "msgwait",		ARGS_ONE },
#ifdef MULTIUSER
  { "multiuser",	ARGS_ONE },
#endif
#ifdef NETHACK
  { "nethack",		ARGS_ONE },
#endif
  { "next",		ARGS_ZERO },
  { "obuflimit",	NEED_DISPLAY|ARGS_ZEROONE },
  { "other",		NEED_DISPLAY|ARGS_ZERO },
  { "partial",		NEED_FORE|ARGS_ZEROONE },
#ifdef PASSWORD
  { "password",		ARGS_ZEROONE },
#endif
#ifdef COPY_PASTE
  { "paste",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "pow_break",	NEED_FORE|ARGS_ZEROONE },
#ifdef POW_DETACH
  { "pow_detach",	NEED_DISPLAY|ARGS_ZERO },
  { "pow_detach_msg",	NEED_DISPLAY|ARGS_ONE },
#endif
  { "prev",		ARGS_ZERO },
  { "process",		NEED_DISPLAY|ARGS_ZEROONE },
  { "quit",		ARGS_ZERO },
#ifdef COPY_PASTE
  { "readbuf",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "redisplay",	ARGS_ZERO },
  { "register",		ARGS_TWO },
#ifdef COPY_PASTE
  { "removebuf",	ARGS_ZERO },
#endif
  { "reset",		NEED_FORE|ARGS_ZERO },
  { "screen",		ARGS_ZERO|ARGS_ORMORE },
#ifdef COPY_PASTE
  { "scrollback",	NEED_FORE|ARGS_ONE },
#endif
  { "select",		ARGS_ONE },
  { "sessionname",	ARGS_ZEROONE },
  { "setenv",		ARGS_TWO },
  { "shell",		ARGS_ONE },
  { "shellaka",		ARGS_ONE },
  { "sleep",		ARGS_ONE },
  { "slowpaste",	ARGS_ONE },
  { "startup_message",	ARGS_ONE },
#ifdef BSDJOBS
  { "suspend",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "term",		ARGS_ONE },
  { "termcap",		ARGS_TWOTHREE },
  { "terminfo",		ARGS_TWOTHREE },
  { "time",		ARGS_ZERO },
  { "unsetenv",		ARGS_ONE },
  { "vbell",		ARGS_ZEROONE },
  { "vbell_msg",	ARGS_ONE },
  { "vbellwait",	ARGS_ONE },
  { "version",		ARGS_ZERO },
  { "width",		NEED_DISPLAY|ARGS_ZEROONE },
  { "windows",		ARGS_ZERO },
  { "wrap",		NEED_FORE|ARGS_ZEROONE },
#ifdef COPY_PASTE
  { "writebuf",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "xoff",		NEED_DISPLAY|ARGS_ZERO },
  { "xon",		NEED_DISPLAY|ARGS_ZERO }
};
