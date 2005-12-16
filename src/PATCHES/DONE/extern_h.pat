*** ..\3.17\extern.h	Sat Feb 01 17:07:28 1992
--- extern.h	Sat Feb 01 13:36:22 1992
***************
*** 47,53 ****
--- 47,55 ----
  # ifdef sun
  extern char *memset __P((char *, int, size_t));
  # endif
+ # ifndef bcopy
  extern void bcopy __P((char *, char *, int));
+ # endif /* bcopy */
  #endif /* MEMFUNCS_DECLARED */
  struct rusage;
  #ifndef WAITSTUFF_DECLARED
***************
*** 245,253 ****
  extern void WriteFile __P((int));
  extern void WriteString __P((struct win *, char *, int));
  extern void bclear __P((char *, int));
! #ifndef MEMFUNCS_DECLARED
  extern void bcopy __P((char *, char *, int));
! #endif /* MEMFUNCS_DECLARED */
  extern void eexit __P((int));
  extern void exit_with_usage __P((char *));
  extern void main __P((int, char **));
--- 247,255 ----
  extern void WriteFile __P((int));
  extern void WriteString __P((struct win *, char *, int));
  extern void bclear __P((char *, int));
! #if !defined(MEMFUNCS_DECLARED) && !defined(bcopy)
  extern void bcopy __P((char *, char *, int));
! #endif /* !MEMFUNCS_DECLARED && !bcopy */
  extern void eexit __P((int));
  extern void exit_with_usage __P((char *));
  extern void main __P((int, char **));
