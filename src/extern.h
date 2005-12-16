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
 * $Id$ FAU
 */


/* screen.c */
extern int   main __P((int, char **));
extern sigret_t SigHup __P(SIGPROTOARG);
extern void  eexit __P((int));
extern void  Detach __P((int));
extern void  Kill __P((int, int));
#ifdef USEVARARGS
extern void  Msg __P((int, char *, ...))
# if __GNUC__ > 1
__attribute__ ((format (printf, 2, 3)))
# endif
;
extern void  Panic __P((int, char *, ...))
# if __GNUC__ > 1
__attribute__ ((format (printf, 2, 3)))
# endif
;
#else
extern void  Msg __P(());
extern void  Panic __P(());
#endif
extern void  DisplaySleep __P((int));
extern void  Finit __P((int));
extern void  MakeNewEnv __P((void));
extern char *MakeWinMsg __P((char *, struct win *, int, int));

/* ansi.c */
extern void  Activate __P((int));
extern void  ResetWindow __P((struct win *));
extern void  ResetCharsets __P((struct win *));
extern void  WriteString __P((struct win *, char *, int));
extern void  NewAutoFlow __P((struct win *, int));
extern void  Redisplay __P((int));
extern void  SetCurr __P((struct win *));
extern void  ChangeAKA __P((struct win *, char *, int));
extern void  SetCharsets __P((struct win *, char *));

/* fileio.c */
extern void  StartRc __P((char *));
extern void  FinishRc __P((char *));
extern void  RcLine __P((char *));
extern FILE *secfopen __P((char *, char *));
extern int   secopen __P((char *, int, int));
extern void  WriteFile __P((int));
extern char *ReadFile __P((char *, int *));
extern void  KillBuffers __P((void));
extern char *expand_vars __P((char *, struct display *));

/* tty.c */
extern int   OpenTTY __P((char *));
extern void  InitTTY __P((struct mode *, int));
extern void  GetTTY __P((int, struct mode *));
extern void  SetTTY __P((int, struct mode *));
extern void  SetMode __P((struct mode *, struct mode *));
extern void  SetFlow __P((int));
extern void  SendBreak __P((struct win *, int, int));
extern int   TtyGrabConsole __P((int, int, char *));
#ifdef DEBUG
extern void  DebugTTY __P((struct mode *));
#endif /* DEBUG */
extern int   fgtty __P((int));
extern void  brktty __P((int));

/* mark.c */
extern int   GetHistory __P((void));
extern void  MarkRoutine __P((void));
extern void  revto_line __P((int, int, int));
extern void  revto __P((int, int));
extern int   InMark __P((void));

/* search.c */
extern void  Search __P((int));
extern void  ISearch __P((int));

/* input.c */
extern void  inp_setprompt __P((char *, char *));
extern void  Input __P((char *, int, int, void (*)(), char *));
extern int   InInput __P((void));

/* help.c */
extern void  exit_with_usage __P((char *, char *, char *));
extern void  display_help __P((void));
extern void  display_copyright __P((void));
extern void  display_displays __P((void));
extern void  display_bindkey __P((char *, struct action *));

/* window.c */
extern int   MakeWindow __P((struct NewWindow *));
extern int   RemakeWindow __P((struct win *));
extern void  FreeWindow __P((struct win *));
extern void  CloseDevice __P((struct win *));
#ifdef PSEUDOS
extern int   winexec __P((char **));
extern void  FreePseudowin __P((struct win *));
#endif
#ifdef MULTI
extern int   execclone __P((char **));
#endif
extern void  nwin_compose __P((struct NewWindow *, struct NewWindow *, struct NewWindow *));

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
#ifdef MAPKEYS
extern void  ProcessInput2 __P((char *, int));
#endif
extern int   FindCommnr __P((char *));
extern void  DoCommand __P((char **));
extern void  KillWindow __P((struct win *));
extern int   ReleaseAutoWritelock __P((struct display *, struct win *));
extern void  SetForeWindow __P((struct win *));
extern int   Parse __P((char *, char **));
extern int   ParseEscape __P((struct user *, char *));
extern void  DoScreen __P((char *, char **));
extern int   IsNumColon __P((char *, int, char *, int));
extern void  ShowWindows __P((void));
extern int   WindowByNoN __P((char *));
#ifdef COPY_PASTE
extern int   CompileKeys __P((char *, unsigned char *));
#endif

/* termcap.c */
extern int   InitTermcap __P((int, int));
extern char *MakeTermcap __P((int));
extern char *gettermcapstring __P((char *));
#ifdef MAPKEYS
extern int   remap __P((int, int));
extern void  CheckEscape __P((void));
#endif
extern int   CreateTransTable __P((char *));
extern void  FreeTransTable __P((void));

/* attacher.c */
extern int   Attach __P((int));
extern void  Attacher __P((void));
extern sigret_t AttacherFinit __P(SIGPROTOARG);

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
extern void  Clear __P((int, int, int, int, int, int, int));
extern void  RefreshLine __P((int, int, int, int));
extern void  RefreshStatus __P((void));
extern void  DisplayLine __P((struct mline *, struct mline *, int, int, int));

extern void  CDisplayLine __P((struct mline *, int, int, int, int, int));
extern void  FixLP __P((int, int));
extern void  GotoPos __P((int, int));
extern int   CalcCost __P((char *));
extern void  ScrollH __P((int, int, int, int, struct mline *));
extern void  ScrollV __P((int, int, int, int, int));
extern void  ChangeScrollRegion __P((int, int));
extern void  InsertMode __P((int));
extern void  KeypadMode __P((int));
extern void  CursorkeysMode __P((int));
extern void  ReverseVideo __P((int));
extern void  CursorVisibility __P((int));
extern void  SetFont __P((int));
extern void  SetAttr __P((int));
extern void  SetColor __P((int));
extern void  SetRendition __P((struct mchar *));
extern void  SetRenditionMline __P((struct mline *, int));
extern void  MakeStatus __P((char *));
extern void  RemoveStatus __P((void));
extern void  SetLastPos __P((int, int));
extern int   ResizeDisplay __P((int, int));
extern int   InitOverlayPage __P((int, struct LayFuncs *, int));
extern void  ExitOverlayPage __P((void));
extern void  AddStr __P((char *));
extern void  AddStrn __P((char *, int));
extern void  Flush __P((void));
extern void  freetty __P((void));
extern void  Resize_obuf __P((void));
#ifdef AUTO_NUKE
extern void  NukePending __P((void));
#endif
#ifdef KANJI
extern int   badkanji __P((char *, int));
#endif

/* resize.c */
extern int   ChangeWindowSize __P((struct win *, int, int, int));
extern void  ChangeScreenSize __P((int, int, int));
extern void  CheckScreenSize __P((int));
extern void  DoResize __P((int, int));
extern char *xrealloc __P((char *, int));

/* socket.c */
extern int   FindSocket __P((int *, int *, int *, char *));
extern int   MakeClientSocket __P((int));
extern int   MakeServerSocket __P((void));
extern int   RecoverSocket __P((void));
extern int   chsock __P((void));
extern void  ReceiveMsg __P((void));
extern void  SendCreateMsg __P((char *, struct NewWindow *));
#ifdef USEVARARGS
extern void  SendErrorMsg __P((char *, ...))
# if __GNUC__ > 1
__attribute__ ((format (printf, 1, 2)))
# endif
;
#else
extern void  SendErrorMsg __P(());
#endif

/* misc.c */
extern char *SaveStr __P((const char *));
extern char *InStr __P((char *, const char *));
#ifndef HAVE_STRERROR
extern char *strerror __P((int));
#endif
extern void  centerline __P((char *));
extern char *Filename __P((char *));
extern char *stripdev __P((char *));
#ifdef NEED_OWN_BCOPY
extern void  xbcopy __P((char *, char *, int));
#endif
extern void  bclear __P((char *, int));
extern void  closeallfiles __P((int));
extern int   UserContext __P((void));
extern void  UserReturn __P((int));
extern int   UserStatus __P((void));
#if defined(POSIX) || defined(hpux)
extern void (*xsignal __P((int, void (*)SIGPROTOARG))) __P(SIGPROTOARG);
#endif
#ifdef NEED_RENAME
extern int   rename __P((char *, char *));
#endif
#if defined(HAVE_SETEUID) || defined(HAVE_SETREUID)
extern void  xseteuid  __P((int));
extern void  xsetegid  __P((int));
#endif
extern int   AddXChar __P((char *, int));
extern int   AddXChars __P((char *, int, char *));
extern void  opendebug __P((int, int));
#ifdef USEVARARGS
# ifndef HAVE_VSNPRINTF
extern int   xvsnprintf __P((char *, int, char *, va_list));
# endif
#else
extern int   xsnprintf __P(());
#endif

/* acl.c */
#ifdef MULTIUSER
extern int   AclInit __P((char *));
extern int   AclSetPass __P((char *, char *));
extern int   AclDelUser __P((char *));
extern int   UserFreeCopyBuffer __P((struct user *));
extern int   AclAddGroup __P((char *));
extern int   AclSetGroupPerm __P((char *, char *));
extern int   AclDelGroup __P((char *));
extern int   AclUserAddGroup __P((char *, char *));
extern int   AclUserDelGroup __P((char *, char *));
extern int   AclCheckPermWin __P((struct user *, int, struct win *));
extern int   AclCheckPermCmd __P((struct user *, int, struct comm *));
extern int   AclSetPerm __P((struct user *, struct user *, char *, char *));
extern int   UsersAcl __P((struct user *, int, char **));
extern void  AclWinSwap __P((int, int));
extern int   NewWindowAcl __P((struct win *, struct user *));
extern void  FreeWindowAcl __P((struct win *));
#endif /* MULTIUSER */
extern struct user **FindUserPtr __P((char *));
extern int   UserAdd __P((char *, char *, struct user **));
extern int   UserDel __P((char *, struct user **));
