/* screen -- screen manager with VT100/ANSI terminal emulation
 * Copyright (C) 1987 Oliver Laumann
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
 */

enum state_t {
    LIT,         /* Literal input */
    ESC,         /* Start of escape sequence */
    STR,         /* Start of control string */
    TERM,        /* ESC seen in control string */
    CSI,         /* Reading arguments in "CSI Pn ; Pn ; ... ; XXX" */
    PRIN,        /* Printer mode */
    PRINESC,     /* ESC seen in printer mode */
    PRINCSI,     /* CSI seen in printer mode */
    PRIN4        /* CSI 4 seen in printer mode */
};

enum string_t {
    NONE,
    DCS,         /* Device control string */
    OSC,         /* Operating system command */
    APC,         /* Application program command */
    PM,          /* Privacy message */
};

#define MAXSTR       128
#define	MAXARGS      64

#define IOSIZE       80

struct win {
    int wpid;
    int ptyfd;
    int aflag;
    char outbuf[IOSIZE];
    int outlen;
    char cmd[MAXSTR];
    char tty[MAXSTR];
    int args[MAXARGS];
    char GotArg[MAXARGS];
    int NumArgs;
    int slot;
    char **image;
    char **attr;
    char **font;
    int LocalCharset;
    int charsets[4];
    int ss;
    int active;
    int x, y;
    char LocalAttr;
    int saved;
    int Saved_x, Saved_y;
    char SavedLocalAttr;
    int SavedLocalCharset;
    int SavedCharsets[4];
    int top, bot;
    int wrap;
    int origin;
    int insert;
    int keypad;
    enum state_t state;
    enum string_t StringType;
    char string[MAXSTR];
    char *stringp;
    char *tabs;
    int vbwait;
    int bell;
};

#define MAXLINE 1024

#define MSG_CREATE    0
#define MSG_ERROR     1
#define MSG_ATTACH    2
#define MSG_CONT      3

struct msg {
    int type;
    union {
	struct {
	    int aflag;
	    int nargs;
	    char line[MAXLINE];
	    char dir[1024];
	} create;
	struct {
	    int apid;
	    char tty[1024];
	} attach;
	char message[MAXLINE];
    } m;
};
