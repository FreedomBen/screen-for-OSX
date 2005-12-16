--- 1.1	1992/01/31 19:28:11
+++ config/config.sgi	1992/01/31 19:36:31
@@ -1,33 +1,32 @@
 /****************************************************************
  * This file is by Andrea Malagoli (alagoli@mhd4.uchicago.edu).
  * It was done for a SGI MIPS Iris4D/20 running Irix 3.3.2.
+ * Enhanced by Marc Boucher (marc@CAM.ORG)
  ********************************************************** 
  * $Id$ FAU
  */
 
-#undef  POSIX
+#define POSIX
 #define BSDJOBS
-#define  TERMIO
+#define TERMIO
 #define TERMINFO
 #define SYSV
-#define  SIGVOID 
-#define  DIRENT
+#define SIGVOID 
+#define DIRENT
 #define SUIDROOT
-#undef UTMPOK
-#define LOGINDEFAULT	0
-#undef GETUTENT
-#undef  UTHOST
+#define UTMPOK
+#define LOGINDEFAULT	1
+#define GETUTENT
+#undef UTHOST
 #undef USRLIMIT
 #undef LOCKPTY
 #undef NOREUID
-#undef  LOADAV
-#undef  LOADAV_3DOUBLES
-#undef  LOADAV_3LONGS
+#undef LOADAV_3DOUBLES
+#define  LOADAV_3LONGS
 #undef  LOADAV_4LONGS
 #undef GETTTYENT
 #undef NFS_HACK
-#undef  LOCALSOCKDIR
-
+#define LOCALSOCKDIR
 #ifdef LOCALSOCKDIR
 # ifndef TMPTEST
 #  define SOCKDIR "/tmp/screens"
@@ -35,10 +34,10 @@
 #  define SOCKDIR "/tmp/testscreens"
 # endif
 #endif
-#define USEBCOPY
+#undef USEBCOPY
 #undef TOPSTAT
-#undef USEVARARGS
-#undef NAMEDPIPE
+#define USEVARARGS
+#define NAMEDPIPE
 #define LOCK
 #define PASSWORD
 #define COPY_PASTE
