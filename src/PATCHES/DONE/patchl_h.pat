*** ..\3.17\patchlev.h	Sat Feb 01 17:07:44 1992
--- patchlev.h	Sat Feb 01 17:16:16 1992
***************
*** 63,74 ****
   *                     SECURITY PROBLEM cleared..
   * 28.01.92 -- 3.01.07 screen after su allowed. Pid became part of 
   *                     SockName. sysvish 14 character restriction considered.
   */
  
  #define ORIGIN "FAU"
  #define REV 3
! #define VERS 1
! #define PATCHLEVEL 7
! #define DATE "01/28/92"
  #define STATE ""
- 
--- 63,75 ----
   *                     SECURITY PROBLEM cleared..
   * 28.01.92 -- 3.01.07 screen after su allowed. Pid became part of 
   *                     SockName. sysvish 14 character restriction considered.
+  * 31.01.92 -- 3.02.00 Ultrix port, Irix 3.3 SGI port, shadow pw support,
+  *                     data loss on stdin overflow fixed. "refresh off".
   */
  
  #define ORIGIN "FAU"
  #define REV 3
! #define VERS 2
! #define PATCHLEVEL 0
! #define DATE "01/31/92"
  #define STATE ""
