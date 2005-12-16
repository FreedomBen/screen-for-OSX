/*
 * This file is automagically created from comm.c -- DO NOT EDIT
 */

struct comm
{
  char *name;
  int flags;
#ifdef MULTIUSER
  AclBits userbits[ACL_BITS_PER_CMD];
#endif
};

#define ARGS_MASK	(3)

#define ARGS_ZERO	(0)
#define ARGS_ONE	(1)
#define ARGS_TWO	(2)
#define ARGS_THREE	(3)

#define ARGS_PLUSONE	(1<<2)
#define ARGS_PLUSTWO	(1<<3)
#define ARGS_ORMORE	(1<<4)

#define NEED_FORE	(1<<5)	/* this command needs a fore window */
#define NEED_DISPLAY	(1<<6)	/* this command needs a display */

#define ARGS_ZEROONE	(ARGS_ZERO|ARGS_PLUSONE)
#define ARGS_ONETWO	(ARGS_ONE |ARGS_PLUSONE)
#define ARGS_TWOTHREE	(ARGS_TWO |ARGS_PLUSONE)
#define ARGS_ZEROTWO	(ARGS_ZERO|ARGS_PLUSTWO)
#define ARGS_ZEROONETWO	(ARGS_ONE |ARGS_PLUSONE|ARGS_PLUSTWO)

struct action
{
  int nr;
  char **args;
};

#define RC_ILLEGAL -1

#define RC_ACLADD 0
#define RC_ACLCHG 1
#define RC_ACLDEL 2
#define RC_ACLGRP 3
#define RC_ACTIVITY 4
#define RC_AKA 5
#define RC_ALLPARTIAL 6
#define RC_AT 7
#define RC_AUTODETACH 8
#define RC_AUTONUKE 9
#define RC_BELL 10
#define RC_BIND 11
#define RC_BREAK 12
#define RC_BUFFERFILE 13
#define RC_CHDIR 14
#define RC_CLEAR 15
#define RC_CLONE 16
#define RC_COLON 17
#define RC_CONSOLE 18
#define RC_COPY 19
#define RC_COPY_REG 20
#define RC_CRLF 21
#define RC_DEBUG 22
#define RC_DEFAUTONUKE 23
#define RC_DEFFLOW 24
#define RC_DEFLOGIN 25
#define RC_DEFMODE 26
#define RC_DEFMONITOR 27
#define RC_DEFOBUFLIMIT 28
#define RC_DEFSCROLLBACK 29
#define RC_DEFWRAP 30
#define RC_DETACH 31
#define RC_DISPLAYS 32
#define RC_DUMPTERMCAP 33
#define RC_DUPLICATE 34
#define RC_ECHO 35
#define RC_ESCAPE 36
#define RC_EXEC 37
#define RC_FLOW 38
#define RC_HARDCOPY 39
#define RC_HARDCOPY_APPEND 40
#define RC_HARDCOPYDIR 41
#define RC_HARDSTATUS 42
#define RC_HEIGHT 43
#define RC_HELP 44
#define RC_HISTORY 45
#define RC_INFO 46
#define RC_INS_REG 47
#define RC_KILL 48
#define RC_LASTMSG 49
#define RC_LICENSE 50
#define RC_LOCKSCREEN 51
#define RC_LOG 52
#define RC_LOGDIR 53
#define RC_LOGIN 54
#define RC_MARKKEYS 55
#define RC_META 56
#define RC_MONITOR 57
#define RC_MSGMINWAIT 58
#define RC_MSGWAIT 59
#define RC_MULTIUSER 60
#define RC_NETHACK 61
#define RC_NEXT 62
#define RC_NUMBER 63
#define RC_OBUFLIMIT 64
#define RC_OTHER 65
#define RC_PARTIAL 66
#define RC_PASSWORD 67
#define RC_PASTE 68
#define RC_POW_BREAK 69
#define RC_POW_DETACH 70
#define RC_POW_DETACH_MSG 71
#define RC_PREV 72
#define RC_PROCESS 73
#define RC_QUIT 74
#define RC_READBUF 75
#define RC_REDISPLAY 76
#define RC_REGISTER 77
#define RC_REMOVEBUF 78
#define RC_RESET 79
#define RC_SCREEN 80
#define RC_SCROLLBACK 81
#define RC_SELECT 82
#define RC_SESSIONNAME 83
#define RC_SETENV 84
#define RC_SHELL 85
#define RC_SHELLAKA 86
#define RC_SHELLTITLE 87
#define RC_SILENCE 88
#define RC_SILENCEWAIT 89
#define RC_SLEEP 90
#define RC_SLOWPASTE 91
#define RC_STARTUP_MESSAGE 92
#define RC_SUSPEND 93
#define RC_TERM 94
#define RC_TERMCAP 95
#define RC_TERMINFO 96
#define RC_TIME 97
#define RC_TITLE 98
#define RC_UNSETENV 99
#define RC_VBELL 100
#define RC_VBELL_MSG 101
#define RC_VBELLWAIT 102
#define RC_VERSION 103
#define RC_WALL 104
#define RC_WIDTH 105
#define RC_WINDOWS 106
#define RC_WRAP 107
#define RC_WRITEBUF 108
#define RC_WRITELOCK 109
#define RC_XOFF 110
#define RC_XON 111
#define RC_ZOMBIE 112

#define RC_LAST 112
