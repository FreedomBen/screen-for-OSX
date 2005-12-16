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
 * $Id$ FAU
 */

/****************************************************************
 * Thanks to Christos S. Zoulas (christos@ee.cornell.edu) who 
 * mangled the screen source through 'gcc -Wall'.
 ****************************************************************
 */

#ifndef MEMFUNCS_DECLARED /* bsd386 */
# ifndef SYSV
extern void  bzero __P((char *, int));
extern void  bcopy __P((char *, char *, int));
# else
extern char *memset __P((char *, int, size_t));
extern char *memcpy __P((char *, char *, int));
#  ifdef USEBCOPY
extern void  bcopy __P((char *, char *, int));
#  endif
# endif
#endif /* MEMFUNCS_DECLARED */
#ifndef WAITSTUFF_DECLARED
# ifdef BSDWAIT
struct rusage;
union wait;
extern int   wait3 __P((union wait *, int, struct rusage *));
# else /* BSDWAIT */
extern pid_t waitpid __P((int, int *, int));
# endif /* BSDWAIT */
#endif /* WAITSTUFF_DECLARED */
#ifndef GETDTABLESIZE_DECLARED
extern int   getdtablesize __P((void));
#endif /* GETDTABLESIZE_DECLARED */
#ifndef REUID_DECLARED
# if !defined(NOREUID)
#  ifdef hpux
extern int   setresuid __P((uid_t, uid_t, uid_t));
extern int   setresgid __P((gid_t, gid_t, gid_t));
#  else
extern int   setreuid __P((uid_t, uid_t));
extern int   setregid __P((gid_t, gid_t));
#  endif
# endif
#endif /* REUID_DECLARED */
#ifndef CRYPT_DECLARED
extern char *crypt __P((char *, char *));
#endif /* CRYPT_DECLARED */
#ifdef sun
extern int   getpgrp __P((int));
#endif
#ifndef MKNOD_DECLARED
# ifdef POSIX
extern int   mknod __P((const char *, mode_t, dev_t));
# else
extern int   mknod __P((char *, int, int));
# endif
#endif /* MKNOD_DECLARED */
#ifndef PUTENV_DECLARED
extern int   putenv __P((char *));
#endif /* PUTENV_DECLARED */
#ifndef KILLSTUFF_DECLARED
extern int   kill __P((int, int));
# ifndef SYSV
extern int   killpg __P((int, int));
# endif
#endif /* KILLSTUFF_DECLARED */
extern int   tgetent __P((char *, char *));
extern int   tgetnum __P((char *));
extern int   tgetflag __P((char *));
extern void  tputs __P((char *, int, void (*)(int)));
# ifdef POSIX
extern pid_t setsid __P((void));
#  ifndef SETPGID_DECLARED
extern int   setpgid __P((int, int));
#  endif /* SETPGID_DECLARED */
#  if defined(BSDI) || defined(__386BSD__)
extern int   tcsetpgrp __P((int, pid_t));
#  else
extern int   tcsetpgrp __P((int, int));
#  endif
# endif /* POSIX */
extern pid_t getpid __P((void));
extern uid_t getuid __P((void)); 
extern uid_t geteuid __P((void));
extern gid_t getgid __P((void)); 
extern gid_t getegid __P((void));
extern int   isatty __P((int)); 
#ifdef notdef
extern int   chown __P((const char *, uid_t, gid_t)); 
#endif
#ifndef GETHOSTNAME_DECLARED
extern int   gethostname __P((char *, size_t));
#endif /* GETHOSTNAME_DECLARED */
extern off_t lseek __P((int, off_t, int));
#if defined(sun) && !defined(__GNUC__)		/* sun's exit returns ??? */
extern int   exit __P((int));
#else
extern void  exit __P((int));
#endif
extern char *getwd __P((char *));
extern char *getenv __P((const char *));
extern time_t time __P((time_t *));

extern char *getlogin(), *getpass(), *ttyname();
extern int   fflush(); 
#if !defined(__STDC__) || !defined(POSIX)
extern char *malloc(), *realloc();
#endif

#ifndef INDEX_DECLARED
extern char *index __P((char *, int));
extern char *rindex __P((char *, int));
#endif /* INDEX_DECLARED */


/************************************************************
 *  Screens function declarations
 */

/* screen.c */
extern void  main __P((int, char **));
extern sig_t SigHup __P(SIGPROTOARG);
extern void  eexit __P((int));
extern void  Detach __P((int));
extern void  Kill __P((int, int));
#ifdef USEVARARGS
extern void  Msg __P((int, char *, ...));
extern void  Panic __P((int, char *, ...));
#else
extern void  Msg __P(());
extern void  Panic __P(());
#endif
extern void  DisplaySleep __P((int));
extern sig_t Finit __P((int));
extern int   fgtty __P((int));
extern void  freetty __P((void));
extern void  brktty __P((int));
extern void  MakeNewEnv __P((void));

/* ansi.c */
extern void  Activate __P((int));
extern void  ResetWindow __P((struct win *));
extern void  WriteString __P((struct win *, char *, int));
extern void  NewAutoFlow __P((struct win *, int));
extern void  Redisplay __P((int));
extern void  CheckLP __P((int));
extern void  MakeBlankLine __P((char *, int));
extern void  SetCurr __P((struct win *));
extern void  AddLineToHist __P((struct win *, char **, char **, char **));

/* fileio.c */
extern void  StartRc __P((char *));
extern void  FinishRc __P((char *));
extern void  RcLine __P((char *));
extern FILE *secfopen __P((char *, char *));
extern int   secopen __P((char *, int, int));
extern void  WriteFile __P((int));
extern void  ReadFile __P((void));
extern void  KillBuffers __P((void));

/* tty.c */
extern void  nwin_compose __P((struct NewWindow *, struct NewWindow *, struct NewWindow *));
extern void  SendBreak __P((struct win *, int, int));
extern void  SetFlow __P((int));
extern int   OpenTTY __P((char *));
extern void  GetTTY __P((int, struct mode *));
extern void  SetTTY __P((int, struct mode *));
extern void  SetMode __P((struct mode *, struct mode *));
extern int   TtyGrabConsole __P((int, int, char *));
extern void  InitTTY __P((struct mode *));
#ifdef DEBUG
extern void  DebugTTY __P((struct mode *));
#endif /* DEBUG */

/* mark.c */
extern int   GetHistory __P((void));
extern void  MarkRoutine __P((void));
extern void  revto_line __P((int, int, int));
extern void  revto __P((int, int));

/* search.c */
extern void  Search __P((int));
extern void  ISearch __P((int));

/* input.c */
extern void  inp_setprompt __P((char *, char *));
extern void  Input __P((char *, int, void (*)(), int));

/* help.c */
extern void  exit_with_usage __P((char *));
extern void  display_help __P((void));
extern void  display_copyright __P((void));

/* exec.c */
extern void  execvpe __P((char *, char **, char **));
extern int   winexec __P((char **));

/* utmp.c */
#ifdef UTMPOK
extern void  InitUtmp __P((void));
extern void  RemoveLoginSlot __P((void));
extern void  RestoreLoginSlot __P((void));
extern int   SetUtmp __P((struct win *));
extern int   RemoveUtmp __P((struct win *));
#endif /* UTMPOK */
extern void  SlotToggle __P((int));
#ifdef USRLIMIT
extern int   CountUsers __P((void));
#endif

/* loadav.c */
#ifdef LOADAV
extern void  InitLoadav __P((void));
extern void  AddLoadav __P((char *));
#endif

/* pty.c */
extern int   OpenPTY __P((char **));

/* process.c */
extern void  InitKeytab __P((void));
extern void  ProcessInput __P((char *, int));
extern int   FindCommnr __P((char *));
extern void  DoAction __P((struct action *, int));
extern int   MakeWindow __P((struct NewWindow *));
extern void  KillWindow __P((struct win *));
extern void  FreeWindow __P((struct win *));
extern void  FreePseudowin __P((struct win *));
extern int   OpenDevice __P((char *, int, int *, char **));
extern int   ForkWindow __P((char **, char *, char *, char *, struct win *));
extern void  SetForeWindow __P((struct win *));
extern int   Parse __P((char *, char **));
extern int   ParseEscape __P((char *));
extern int   CompileKeys __P((char *, char *));
extern void  DoScreen __P((char *, char **));
extern struct acl **findacl __P((char *));

/* termcap.c */
extern int   InitTermcap __P((int, int));
extern char *MakeTermcap __P((int));
extern char *gettermcapstring __P((char *));

/* attacher.c */
extern int   Attach __P((int));
extern void  Attacher __P((void));

/* display.c */
extern struct display *MakeDisplay __P((char *, char *, char *, int, int, struct mode *));
extern void  FreeDisplay __P((void));
extern void  DefProcess __P((char **, int *));
extern void  DefRedisplayLine __P((int, int, int, int));
extern void  DefClearLine __P((int, int, int));
extern int   DefRewrite __P((int, int, int, int));
extern void  DefSetCursor __P((void));
extern int   DefResize __P((int, int));
extern void  DefRestore __P((void));
extern void  PutStr __P((char *));
extern void  CPutStr __P((char *, int));
extern void  InitTerm __P((int));
extern void  FinitTerm __P((void));
extern void  INSERTCHAR __P((int));
extern void  PUTCHAR __P((int));
extern void  PUTCHARLP __P((int));
extern void  RAW_PUTCHAR __P((int));
extern void  ClearDisplay __P((void));
extern void  Clear __P((int, int, int, int));
extern void  RefreshLine __P((int, int, int, int));
extern void  DisplayLine __P((char *, char *, char *, char *, char *, char *, int, int, int));
extern void  FixLP __P((int, int));
extern void  GotoPos __P((int, int));
extern int   CalcCost __P((char *));
extern void  ScrollRegion __P((int, int, int));
extern void  ChangeScrollRegion __P((int, int));
extern void  InsertMode __P((int));
extern void  KeypadMode __P((int));
extern void  CursorkeysMode __P((int));
extern void  SetFont __P((int));
extern void  SetAttr __P((int));
extern void  SetAttrFont __P((int, int));
extern void  MakeStatus __P((char *));
extern void  RemoveStatus __P((void));
extern void  SetLastPos __P((int, int));
extern int   ResizeDisplay __P((int, int));
extern int   InitOverlayPage __P((int, struct LayFuncs *, int));
extern void  ExitOverlayPage __P((void));
extern void  AddStr __P((char *));
extern void  AddStrn __P((char *, int));
extern void  Flush __P((void));
extern void  Resize_obuf __P((void));
#ifdef AUTO_NUKE
extern void  NukePending __P((void));
#endif

/* window.c */
extern int   ChangeScrollback __P((struct win *, int, int));
extern int   ChangeWindowSize __P((struct win *, int, int));
extern void  ChangeScreenSize __P((int, int, int));
extern void  CheckScreenSize __P((int));
extern void  DoResize __P((int, int));
extern char *xrealloc __P((char *, int));

/* socket.c */
extern int   FindSocket __P((int, int *));
extern int   MakeClientSocket __P((int, char *));
extern int   MakeServerSocket __P((void));
extern int   RecoverSocket __P((void));
extern int   chsock __P((void));
extern void  ReceiveMsg __P(());
extern void  SendCreateMsg __P((int, struct NewWindow *));
#ifdef USEVARARGS
extern void  SendErrorMsg __P((char *, ...));
#else
extern void  SendErrorMsg __P(());
#endif

/* misc.c */
extern char *SaveStr __P((const char *));
extern void  centerline __P((char *));
extern char *Filename __P((char *));
extern char *stripdev __P((char *));
#if !defined(MEMFUNCS_DECLARED) && !defined(bcopy)
extern void  bcopy __P((char *, char *, int));
#endif /* !MEMFUNCS_DECLARED && !bcopy */
extern void  bclear __P((char *, int));
extern void  closeallfiles __P((int));
extern int   UserContext __P((void));
extern void  UserReturn __P((int));
extern int   UserStatus __P((void));
