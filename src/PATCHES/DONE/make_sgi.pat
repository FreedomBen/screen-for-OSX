--- 1.1	1992/01/31 19:28:11
+++ config/Makefile.sgi	1992/01/31 20:06:23
@@ -6,11 +6,9 @@
 
 # If you choose to compile with the tried and true:
 CC= cc
-# Compiled on SGI MIPS Iris4D/20 running Irix 3.3.2, compiler doesn't define us 
-# any specific id so we define it here, maybe it should be defined in config.h
-#CFLAGS= -O -DMIPS -DSGI -I/usr/include/bsd
-#CFLAGS= -g -DTMPTEST -DDEBUG -DMIPS -DSGI -I/usr/include/bsd
-CFLAGS= -g -DMIPS -DSGI -I/usr/include/bsd
+CFLAGS= -O 
+#CFLAGS= -g -DTMPTEST -DDEBUG 
+#CFLAGS= -g 
 LDFLAGS=
 
 # If you're using GNU C, be sure to use the -traditional flag:
@@ -24,7 +22,7 @@
 #LIBS= nmalloc.o -ltermcap 
 #LIBS= -ltermcap
 # mld needed on MIPS to have nlist for loadaverage
-LIBS= -ltermcap -lsun -lbsd -lmld -lc_s
+LIBS= -ltermlib -lsun -lmld #-lc_s 
 
 CFILES=	screen.c ansi.c help.c fileio.c mark.c window.c socket.c
 OFILES=	screen.o ansi.o help.o fileio.o mark.o window.o socket.o
