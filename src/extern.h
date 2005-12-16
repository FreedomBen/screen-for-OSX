/* Copyright (c) 1991 Juergen Weigert (jnweiger@immd4.uni-erlangen.de)
 *                    Michael Schroeder (mlschroe@immd4.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 * All rights reserved.  Not derived from licensed software.
 *
 * Permission is granted to freely use, copy, modify, and redistribute
 * this software, provided that no attempt is made to gain profit from it,
 * the authors are not construed to be liable for any results of using the
 * software, alterations are clearly marked as such, and this notice is
 * not modified.
 *
 * Noteworthy contributors to screen's design and implementation:
 *	Wayne Davison (davison@borland.com)
 *	Patrick Wolfe (pat@kai.com, kailand!pat)
 *	Bart Schaefer (schaefer@cse.ogi.edu)
 *	Nathan Glasser (nathan@brokaw.lcs.mit.edu)
 *	Larry W. Virden (lwv27%cas.BITNET@CUNYVM.CUNY.Edu)
 *	Howard Chu (hyc@hanauma.jpl.nasa.gov)
 *	Tim MacKenzie (tym@dibbler.cs.monash.edu.au)
 *	Markku Jarvinen (mta@{cc,cs,ee}.tut.fi)
 *	Marc Boucher (marc@CAM.ORG)
 *
 ****************************************************************
 * Thanks to Christos S. Zoulas (christos@ee.cornell.edu) who 
 * mangled the screen source through 'gcc -Wall'.
 *
 * this is his extern.h
 ****************************************************************
 * $Id$ FAU
 */
#ifndef SYSV
extern void bzero __P((char *, int));
#endif
#ifdef sun
extern char *memset __P((char *, int, size_t));
#endif
extern void bcopy __P((char *, char *, int));
struct rusage;
#ifdef BSDWAIT
union wait;
extern int wait3 __P((union wait *, int, struct rusage *));
#else
extern int wait3 __P((int *, int, struct rusage *));
#endif
extern int getdtablesize __P((void));
extern int setreuid __P((uid_t, uid_t));
extern int setregid __P((gid_t, gid_t));
extern char *crypt __P((char *, char *));
#ifdef sun
extern int getpgrp __P((int));
#endif
extern int mknod __P((char *, int, int));
extern int putenv __P((char *));
extern int kill __P((pid_t, int));
#ifndef SYSV
extern int killpg __P((pid_t, int));
#endif
extern int tgetent __P((char *, char *));
extern int tgetnum __P((char *));
extern int tgetflag __P((char *));
extern void tputs __P((char *, int, void (*)(int)));
#ifdef notdef
extern unsigned char     *_flsbuf __P((unsigned char, FILE *));
#endif
extern int _flsbuf __P((unsigned char, FILE *));
#ifdef POSIX
extern pid_t setsid __P((void));
extern int setpgid __P((pid_t, int));
extern int tcsetpgrp __P((int, pid_t));
#endif
extern pid_t getpid __P((void));
extern uid_t getuid __P((void)); 
extern uid_t geteuid __P((void));
extern gid_t getgid __P((void)); 
extern gid_t getegid __P((void));
extern int isatty __P((int)); 
#ifdef notdef
extern int chown __P((const char *, uid_t, gid_t)); 
#endif
extern int gethostname __P((char *, size_t));
extern off_t lseek __P((int, off_t, int));
#if defined(sun) && !defined(__GNUC__)		/* sun's exit returns ??? */
extern int exit __P((int));
#else
extern void exit __P((int));
#endif
extern char *getwd __P((char *));
extern char *getenv __P((const char *));
extern time_t time __P((time_t *));

extern char *getlogin(), *getpass(), *ttyname();
extern int fflush(); 
#if !__STDC__ || !defined(POSIX)
extern char *malloc(), *realloc();
#endif



extern char *Filename __P((char *));
extern char *MakeTermcap __P((int));
extern char *ParseChar __P((char *, char *));
extern char *ProcessInput __P((char *, int *, char *, int *, int));
extern char *SaveStr __P((char *));
extern char *findcap __P((char *, char **, int));
extern char *findrcfile __P((char *));
extern char *strdup __P((const char *));
extern int ChangeScrollback __P((struct win *, int, int));
extern int ChangeWindowSize __P((struct win *, int, int));
extern int CompileKeys __P((char *, char *));
extern int CountUsers __P((void));
extern int FindSocket __P((int, int *));
extern int GetAvenrun __P((void));
extern int GetSockName __P((void));
extern int MakeClientSocket __P((int, char *));
extern int MakeServerSocket __P((void));
extern int MakeWindow __P((char *, char **, int, int, int, char *, int, int));
extern int MarkRoutine __P((int));
extern int ParseEscape __P((char *));
extern int RcLine __P((char *));
extern int RecoverSocket __P((void));
extern int RemoveUtmp __P((struct win *));
extern int SetUtmp __P((struct win *, int));
extern int UserContext __P((void));
extern int UserStatus __P((void));
extern int display_help __P((void));
#ifdef DEBUG
extern sig_t FEChld __P((void));
#endif
extern sig_t SigHup __P((void));
extern void AbortRc __P((void));
extern void Activate __P((void));
extern void ChangeScreenSize __P((int, int, int));
extern void ChangeScrollRegion __P((int, int));
extern void CheckLP __P((int));
extern void CheckScreenSize __P((int));
extern void ClearDisplay __P((void));
extern void Detach __P((int));
extern void DisplayLine __P((char *, char *, char *, char *, char *, char *, int, int, int));
extern void DoScreen __P((char *, char **));
extern void DoSet __P((char **));
extern void ExitOverlayPage __P((void));
extern void FinishRc __P((char *));
extern void FinitTerm __P((void));
extern void FixLP __P((int, int));
extern void GetTTY __P((int, struct mode *));
extern void GotoPos __P((int, int));
extern void InitKmem __P((void));
extern void InitOverlayPage __P((void (*)(), void (*)(), int (*)(), int));
extern void InitTerm __P((void));
extern void InitTermcap __P((void));
extern void InitUtmp __P((void));
extern void InputAKA __P((void));
extern void InputColon __P((void));
extern void InsertMode __P((int));
extern void Kill __P((int, int));
extern int CheckPid __P((int));
extern void KillBuffers __P((void));
extern void MakeBlankLine __P((char *, int));
extern void MakeStatus __P((char *));
#ifdef USEVARARGS
extern void Msg __P((int, char *, ...));
#else
extern void Msg __P(());
#endif
extern void NewCharset __P((int));
extern void NewRendition __P(());
extern void PUTCHAR __P((int));
extern void ParseNum __P((int, char *[], int *));
extern void ParseOnOff __P((int, char *[], int *));
extern void PutChar __P((int));
extern void PutStr __P((char *));
extern void ReInitUtmp __P((void));
extern void ReadFile __P((void));
extern void ReceiveMsg __P((int));
extern void Redisplay __P((void));
extern void RefreshLine __P((int, int, int));
struct utmp;
extern void RemoveLoginSlot __P((slot_t, struct utmp *));
extern void RemoveStatus __P((void));
extern void Report __P((struct win *, char *, int, int));
extern void ResetScreen __P((struct win *));
extern void ResizeScreen __P((struct win *));
extern void RestoreAttr __P((void));
extern void RestoreLoginSlot __P((void));
extern void SaveSetAttr __P((int, int));
extern void ScrollRegion __P((int, int, int));
extern void SendCreateMsg __P((int, int, char **, int, int, int, int));
#ifdef USEVARARGS
extern void SendErrorMsg __P((char *, ...));
#else
extern void SendErrorMsg __P(());
#endif
extern void SetFlow __P((int));
extern void SetLastPos __P((int, int));
extern void SetMode __P((struct mode *, struct mode *));
extern void SetOvlCurr __P((void));
extern void SetTTY __P((int, struct mode *));
extern void SlotToggle __P((int));
extern void StartRc __P((char *));
extern void SwitchWindow __P((int));
extern void ToggleFlow __P((void));
extern void UserReturn __P((int));
extern void WSresize __P((int, int));
extern void WriteFile __P((int));
extern void WriteString __P((struct win *, char *, int));
extern void add_key_to_buf __P((char *, int));
extern void bclear __P((char *, int));
extern void bcopy __P((char *, char *, int));
extern void brktty __P((void));
extern void dofg __P((void));
extern void eexit __P((int));
extern void exit_with_usage __P((char *));
extern void main __P((int, char **));
extern void screen_builtin_lck __P((void));
extern void KillWindow __P((int));
extern char *xrealloc __P((char *, int));
extern void AddLineToHist __P((struct win *, char **, char **, char **));
extern FILE *secfopen __P((char *, char *));
extern int secopen __P((char *, int, int));
