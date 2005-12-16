*** ..\3.17\fileio.c	Sat Feb 01 17:07:36 1992
--- fileio.c	Sat Feb 01 17:39:36 1992
***************
*** 39,45 ****
  extern int errno;
  #endif
  #include <sys/types.h>
! #include <sys/file.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  
--- 39,47 ----
  extern int errno;
  #endif
  #include <sys/types.h>
! #ifndef sgi
! # include <sys/file.h>
! #endif /* sgi */
  #include <sys/stat.h>
  #include <fcntl.h>
  
***************
*** 73,79 ****
  #  include <nlist.h>
  
  static char KmemName[] = "/dev/kmem";
! #  if defined(_SEQUENT_) || defined(MIPS) || defined(SVR4) || defined(ISC)
  static char UnixName[] = "/unix";
  #  else
  #   ifdef sequent
--- 75,81 ----
  #  include <nlist.h>
  
  static char KmemName[] = "/dev/kmem";
! #  if defined(_SEQUENT_) || defined(MIPS) || defined(SVR4) || defined(ISC) || defined (sgi)
  static char UnixName[] = "/unix";
  #  else
  #   ifdef sequent
***************
*** 94,100 ****
  #  ifdef alliant
  static char AvenrunSym[] = "_Loadavg";
  #  else
! #   if defined(hpux) || defined(_SEQUENT_) || defined(SVR4) || defined(ISC)
  static char AvenrunSym[] = "avenrun";
  #   else
  static char AvenrunSym[] = "_avenrun";
--- 96,102 ----
  #  ifdef alliant
  static char AvenrunSym[] = "_Loadavg";
  #  else
! #   if defined(hpux) || defined(_SEQUENT_) || defined(SVR4) || defined(ISC) || defined(sgi)
  static char AvenrunSym[] = "avenrun";
  #   else
  static char AvenrunSym[] = "_avenrun";
***************
*** 138,143 ****
--- 140,146 ----
  extern nethackflag;
  #endif
  int hardcopy_append = 0;
+ int all_norefresh = 0;
  
  extern char *RcFileName, *home, *extra_incap, *extra_outcap;
  extern char *BellString, *ActivityString, *ShellProg, *ShellArgs[];
***************
*** 225,232 ****
    "activity", "all", "autodetach", "bell", "bind", "bufferfile", "chdir",
    "crlf", "echo", "escape", "flow", "hardcopy_append", "hardstatus", "login", 
    "markkeys", "mode", "monitor", "msgminwait", "msgwait", "nethack", "password",
!   "pow_detach_msg", "refresh", "screen", "scrollback", "shell", "shellaka",
!   "sleep", "slowpaste", "startup_message", "term", "termcap",
    "terminfo", "vbell", "vbell_msg", "vbellwait", "visualbell",
    "visualbell_msg", "wrap",
  };
--- 228,235 ----
    "activity", "all", "autodetach", "bell", "bind", "bufferfile", "chdir",
    "crlf", "echo", "escape", "flow", "hardcopy_append", "hardstatus", "login", 
    "markkeys", "mode", "monitor", "msgminwait", "msgwait", "nethack", "password",
!   "pow_detach_msg", "redraw", refresh", "screen", "scrollback", "shell", 
!   "shellaka", "sleep", "slowpaste", "startup_message", "term", "termcap",
    "terminfo", "vbell", "vbell_msg", "vbellwait", "visualbell",
    "visualbell_msg", "wrap",
  };
***************
*** 255,260 ****
--- 258,264 ----
    RC_NETHACK,
    RC_PASSWORD,
    RC_POW_DETACH_MSG,
+   RC_REDRAW,
    RC_REFRESH,
    RC_SCREEN,
    RC_SCROLLBACK,
***************
*** 272,278 ****
    RC_VISUALBELL,
    RC_VISUALBELL_MSG,
    RC_WRAP,
!   RC_RCEND,
  };
  
  #ifdef UTMPOK
--- 276,282 ----
    RC_VISUALBELL,
    RC_VISUALBELL_MSG,
    RC_WRAP,
!   RC_RCEND
  };
  
  #ifdef UTMPOK
***************
*** 878,883 ****
--- 882,888 ----
  	    default_monitor = (f == 0) ? MON_OFF : MON_ON;
  	}
        return;
+     case RC_REDRAW:
      case RC_REFRESH:
  	{
  	  int r;
***************
*** 886,892 ****
  	  if (fore && setflag)
  	    fore->norefresh = (r) ? 0 : 1;
  	  else
! 	    Msg(0, "Default refresh is ON, cannot be changed yet.");
  	}
        return;
      case RC_VBELL:
--- 891,903 ----
  	  if (fore && setflag)
  	    fore->norefresh = (r) ? 0 : 1;
  	  else
! 	    {
! 	      all_norefresh = (r) ? 0 : 1;
! 	      if (all_norefresh)
! 	        Msg(0, "No refresh on window change!\n");
! 	      else
! 	        Msg(0, "Window specific refresh\n");
! 	    }
  	}
        return;
      case RC_VBELL:
***************
*** 2369,2374 ****
--- 2380,2388 ----
        close(kmemf);
        return;
      }
+ #   ifdef sgi
+   nl[0].n_value &= ~(1 << 31); /* clear upper bit */
+ #   endif /* sgi */
    debug("AvenrunSym found!!\n");
  #  endif /* apollo */
    avenrun = 1;
