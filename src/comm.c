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

#include "config.h"
#include "acls.h"
#include "comm.h"

/* Must be in alpha order ! */

struct comm comms[RC_LAST + 1] =
{
#ifdef MULTIUSER
  { "acladd",		ARGS_ONE },
  { "aclchg",		ARGS_THREE },
  { "acldel",		ARGS_ONE },
  { "aclgrp",		ARGS_ONE },
#endif
  { "activity",		ARGS_ONE },
  { "aka",		NEED_FORE|ARGS_ZEROONE },	/* TO BE REMOVED */
  { "allpartial",	NEED_DISPLAY|ARGS_ONE },
  { "at",		NEED_DISPLAY|ARGS_TWO|ARGS_ORMORE },
  { "autodetach",	ARGS_ONE },
#ifdef AUTO_NUKE
  { "autonuke",		NEED_DISPLAY|ARGS_ONE },
#endif
  { "bell",		ARGS_ZEROONE },
  { "bell_msg",		ARGS_ZEROONE },
  { "bind",		ARGS_ONE|ARGS_ORMORE },
#ifdef MAPKEYS
  { "bindkey",		ARGS_ZERO|ARGS_ORMORE },
#endif
  { "break",		NEED_FORE|ARGS_ZEROONE },
#ifdef COPY_PASTE
  { "bufferfile",	ARGS_ZEROONE },
#endif
  { "c1",		NEED_FORE|ARGS_ZEROONE },
  { "charset",          NEED_FORE|ARGS_ONE },
  { "chdir",		ARGS_ZEROONE },
  { "clear",		NEED_FORE|ARGS_ZERO },
#ifdef MULTI
  { "clone",		NEED_DISPLAY|ARGS_ONE|ARGS_ORMORE },
#endif
  { "colon",		NEED_DISPLAY|ARGS_ZERO },
  { "command",		NEED_DISPLAY|ARGS_ZERO },
  { "console",		NEED_FORE|ARGS_ZEROONE },
#ifdef COPY_PASTE
  { "copy",		NEED_FORE|ARGS_ZERO },
  { "crlf",		ARGS_ONE },
#endif
  { "debug",		ARGS_ZEROONE },
#ifdef AUTO_NUKE
  { "defautonuke",	ARGS_ONE },
#endif
  { "defc1",		ARGS_ONE },
  { "defcharset",       ARGS_ZEROONE },
  { "defescape",	ARGS_ONE },
  { "defflow",		ARGS_ONETWO },
  { "defgr",		ARGS_ONE },
  { "defhstatus",	ARGS_ZEROONE },
#ifdef KANJI
  { "defkanji",		ARGS_ONE },
#endif
#if defined(UTMPOK) && defined(LOGOUTOK)
  { "deflogin",		ARGS_ONE },
#endif
  { "defmode",		ARGS_ONE },
  { "defmonitor",	ARGS_ONE },
  { "defobuflimit",	ARGS_ONE },
#ifdef COPY_PASTE
  { "defscrollback",	ARGS_ONE },
#endif
  { "defwrap",		ARGS_ONE },
  { "defwritelock",	ARGS_ONE },
  { "detach",		NEED_DISPLAY|ARGS_ZERO },
  { "digraph",		NEED_DISPLAY|ARGS_ZEROONE },
  { "displays",		NEED_DISPLAY|ARGS_ZERO },
  { "dumptermcap",	NEED_FORE|ARGS_ZERO },
  { "echo",		ARGS_ONETWO },
  { "escape",		NEED_DISPLAY|ARGS_ONE },
#ifdef PSEUDOS
  { "exec", 		NEED_FORE|ARGS_ZERO|ARGS_ORMORE },
#endif
  { "flow",		NEED_FORE|ARGS_ZEROONE },
  { "gr",		NEED_FORE|ARGS_ZEROONE },
  { "hardcopy",		NEED_FORE|ARGS_ZERO },
  { "hardcopy_append",	ARGS_ONE },
  { "hardcopydir",	ARGS_ONE },
  { "hardstatus",	NEED_DISPLAY|ARGS_ZEROONE },
  { "height",		NEED_DISPLAY|ARGS_ZEROONE },
  { "help",		NEED_DISPLAY|ARGS_ZERO },
#ifdef COPY_PASTE
  { "history",		NEED_FORE|ARGS_ZERO },
#endif
  { "info",		NEED_DISPLAY|ARGS_ZERO },
#ifdef KANJI
  { "kanji",		NEED_FORE|ARGS_ONETWO },
#endif
  { "kill",		NEED_FORE|ARGS_ZERO },
  { "lastmsg",		NEED_DISPLAY|ARGS_ZERO },
  { "license",		NEED_DISPLAY|ARGS_ZERO },
#ifdef LOCK
  { "lockscreen",	NEED_DISPLAY|ARGS_ZERO },
#endif
  { "log",		NEED_FORE|ARGS_ZEROONE },
  { "logfile",		ARGS_ZEROONE },
#if defined(UTMPOK) && defined(LOGOUTOK)
  { "login",		NEED_FORE|ARGS_ZEROONE },
#endif
#ifdef MAPKEYS
  { "mapdefault",	NEED_DISPLAY|ARGS_ZERO },
  { "mapnotnext",	NEED_DISPLAY|ARGS_ZERO },
  { "maptimeout",	ARGS_ZEROONE },
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
  { "next",		NEED_DISPLAY|NEED_FORE|ARGS_ZERO },
  { "number",		NEED_FORE|ARGS_ZEROONE },
  { "obuflimit",	NEED_DISPLAY|ARGS_ZEROONE },
  { "other",		NEED_DISPLAY|NEED_FORE|ARGS_ZERO },
  { "partial",		NEED_FORE|ARGS_ZEROONE },
#ifdef PASSWORD
  { "password",		ARGS_ZEROONE },
#endif
#ifdef COPY_PASTE
  { "paste",		NEED_DISPLAY|ARGS_ZEROONETWO },
  { "pastefont",	ARGS_ZEROONE },
#endif
  { "pow_break",	NEED_FORE|ARGS_ZEROONE },
#ifdef POW_DETACH
  { "pow_detach",	NEED_DISPLAY|ARGS_ZERO },
  { "pow_detach_msg",	ARGS_ZEROONE },
#endif
  { "prev",		NEED_DISPLAY|NEED_FORE|ARGS_ZERO },
  { "printcmd",		ARGS_ZEROONE },
  { "process",		NEED_DISPLAY|ARGS_ZEROONE },
  { "quit",		ARGS_ZERO },
#ifdef COPY_PASTE
  { "readbuf",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "readreg",          ARGS_ZEROONETWO },
  { "redisplay",	NEED_DISPLAY|ARGS_ZERO },
  { "register",		ARGS_TWO },
#ifdef COPY_PASTE
  { "removebuf",	ARGS_ZERO },
#endif
  { "reset",		NEED_FORE|ARGS_ZERO },
  { "screen",		ARGS_ZERO|ARGS_ORMORE },
#ifdef COPY_PASTE
  { "scrollback",	NEED_FORE|ARGS_ONE },
#endif
  { "select",		ARGS_ZEROONE },
  { "sessionname",	ARGS_ZEROONE },
  { "setenv",		ARGS_ZEROONETWO },
  { "shell",		ARGS_ONE },
  { "shellaka",		ARGS_ONE },			/* TO BE REMOVED */
  { "shelltitle",	ARGS_ONE },
  { "silence",		NEED_FORE|ARGS_ZEROONE },
  { "silencewait",	ARGS_ONE },
  { "sleep",		ARGS_ONE },
  { "slowpaste",	ARGS_ONE },
  { "sorendition",      ARGS_ZEROONETWO },
  { "startup_message",	ARGS_ONE },
  { "stuff",		NEED_DISPLAY|ARGS_ONE },
#ifdef BSDJOBS
  { "suspend",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "term",		ARGS_ONE },
  { "termcap",		ARGS_TWOTHREE },
  { "termcapinfo",	ARGS_TWOTHREE },
  { "terminfo",		ARGS_TWOTHREE },
  { "time",		ARGS_ZERO },
  { "title",		NEED_FORE|ARGS_ZEROONE },
  { "unsetenv",		ARGS_ONE },
  { "vbell",		ARGS_ZEROONE },
  { "vbell_msg",	ARGS_ZEROONE },
  { "vbellwait",	ARGS_ONE },
  { "version",		ARGS_ZERO },
  { "wall",		NEED_DISPLAY|ARGS_ONE|ARGS_ORMORE },
  { "width",		NEED_DISPLAY|ARGS_ZEROONE },
  { "windows",		NEED_DISPLAY|ARGS_ZERO },
  { "wrap",		NEED_FORE|ARGS_ZEROONE },
#ifdef COPY_PASTE
  { "writebuf",		NEED_DISPLAY|ARGS_ZERO },
#endif
  { "writelock",	NEED_FORE|ARGS_ZEROONE },
  { "xoff",		NEED_DISPLAY|ARGS_ZERO },
  { "xon",		NEED_DISPLAY|ARGS_ZERO },
  { "zombie",		ARGS_ZEROONE }
};
