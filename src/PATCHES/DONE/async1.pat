From tym@bruce.cs.monash.edu.au Tue Nov 10 15:20 MET 1992
Received: from bruce.cs.monash.edu.au by immd4.informatik.uni-erlangen.de with SMTP;
	id AA13058 (5.65c-5/7.3m-FAU); Tue, 10 Nov 1992 15:19:47 +0100
Received: by bruce (5.57/1.34)
	id AA26780; Wed, 11 Nov 92 01:19:21 +1100
Received: by dibbler.cs.monash.edu.au (4.1)
        id AA25472; Wed, 11 Nov 92 01:19:16 EST
        (from tym for jnweiger@immd4.informatik.uni-erlangen.de)
From: tym@bruce.cs.monash.edu.au (Tim MacKenzie)
Message-Id: <9211101419.AA25472@dibbler.cs.monash.edu.au>
Subject: Patches to screen-3.2.15 for asyncronous output
To: mlschroe@immd4.informatik.uni-erlangen.de (Michael Schroeder),
        jnweiger@immd4.informatik.uni-erlangen.de (Juergen Weigert)
Date: Wed, 11 Nov 1992 01:19:14 +1000 (EST)
Cc: tym@dibbler.cs.monash.edu.au (Tim MacKenzie)
Organization: Computer Science, Monash University, Clayton, Vic, Australia.
Phone: +61 3 565 5375
X-Mailer: ELM [version 2.4 PL5]
Mime-Version: 1.0
Content-Type: text/plain; charset=US-ASCII
Content-Transfer-Encoding: 7bit
Content-Length: 8192      
Status: RO

Here are my patches for asynchronous output. It was rather simple in the end,
and it seems to work, although I haven't extensively tested it on multiple
platforms. Basically, AddChar() never Flush()'s, but adds output to an
expanding buffer. When the buffer reaches a maximum size then no more input
is accepted from that display's foreground window. Every time around the
select loop, the tty is tested for writing if there is output pending.

When the screen is cleared, the output buffer is cleared, and the terminal's
output buffer is flushed.

It is switched on with #define ASYNC_OUTPUT .
I also extended Flush() a little to check the result of it's write();
-- 
-Tim MacKenzie (tym@dibbler.cs.monash.edu.au)

diff -ur screen-3.2.15/display.c screen-3.2.15.new/display.c
--- screen-3.2.15/display.c	Fri Nov  6 00:45:51 1992
+++ screen-3.2.15.new/display.c	Wed Nov 11 00:56:51 1992
@@ -171,8 +171,12 @@
   disp->d_flow = 1;
   disp->d_userfd = fd;
   disp->d_OldMode = *Mode;
-  disp->d_obufp = disp->d_obuf;
+#ifdef ASYNC_OUTPUT
+  Resize_obuf(disp);  /* Allocate memory for buffer */
+#else
   disp->d_obuffree = sizeof(disp->d_obuf);
+#endif
+  disp->d_obufp = disp->d_obuf;
   disp->d_userpid = pid;
 #ifdef POSIX
   disp->d_dospeed = (short) cfgetospeed(&disp->d_OldMode.tio);
@@ -201,6 +205,10 @@
   if (disp->d_copybuffer)
     free(disp->d_copybuffer);
 # endif
+#ifdef ASYNC_OUTPUT
+  if (disp->d_obuf) free(disp->d_obuf);
+  disp->d_obuf = 0;
+#endif
   free(disp);
 #else
   ASSERT(disp == &TheDisplay);
@@ -217,6 +225,11 @@
 {
   ASSERT(display);
   top = bot = -1;
+#ifdef ASYNC_OUTPUT
+  NukePending();
+#else
+  PutStr(CL);
+#endif
   PutStr(TI);
   PutStr(IS);
   /* Check for toggle */
@@ -232,7 +245,6 @@
   if (adapt == 0)
     ResizeDisplay(defwidth, defheight);
   ChangeScrollRegion(0, height - 1);
-  PutStr(CL);
   curx = cury = 0;
   Flush();
   debug1("we %swant to adapt all our windows to the display\n", 
@@ -648,9 +660,15 @@
     }
   if (xe == width - 1 && ye == height - 1)
     {
+#ifdef ASYNC_OUTPUT
+      if (xs == 0 && ys == 0)
+	{
+	  NukePending();
+#else
       if (xs == 0 && ys == 0 && CL)
 	{
 	  PutStr(CL);
+#endif
 	  cury = curx = 0;
 	  return;
 	}
@@ -1207,14 +1225,94 @@
 Flush()
 {
   register int l;
+  register char *p;
 
   ASSERT(display);
   l = obufp - obuf;
+#ifndef ASYNC_OUTPUT
   ASSERT(l + obuffree == sizeof(obuf));
-  if (l)
+#endif
+  if (!l) return;
+  p = obuf;
+  while (l)
+    {
+      register int wr;
+      wr = write(userfd, p, l);
+      if (wr < 0) 
+	{
+	  if (errno == EWOULDBLOCK) 
+	    {
+	      fd_set set;
+	      do {
+		FD_ZERO(&set);
+		FD_SET(userfd,&set);
+		wr = select(userfd+1,NULL,&set,NULL,NULL);
+	      } while (wr < 0 && errno == EINTR);
+	    }
+	  else if (errno != EINTR && errno != EPIPE)
+	    Msg(errno,"Writing to display");
+	}
+      else
+	{
+	  obuffree += wr;
+	  p += wr;
+	  l -= wr;
+	}
+    }
+  obufp = obuf;
+}
+
+#ifdef ASYNC_OUTPUT
+void
+Resize_obuf(disp)
+struct display *disp;
+{
+  register int ind;
+
+  ind  = disp->d_obufp - disp->d_obuf;
+  if (disp->d_obuflen && disp->d_obuf)
+    {
+      disp->d_obuflen += GRAIN;
+      disp->d_obuffree += GRAIN;
+      disp->d_obuf = realloc(disp->d_obuf,disp->d_obuflen);
+    }
+  else
+    {
+      disp->d_obuflen = GRAIN+1;  /* Leave a spare byte for safety :) */
+      disp->d_obuffree = GRAIN;
+      disp->d_obuf = malloc(disp->d_obuflen);
+    }
+  if (!disp->d_obuf) Panic(0,"Out of memory");
+  disp->d_obufp = disp->d_obuf + ind;
+}
+
+void
+NukePending()
+{/* Nuke pending output in current display, clear screen */
+  register int len;
+
+  write(userfd,"",1);
+  len = obufp - obuf;
+  
+  /* Throw away any output that we can... */
+# ifdef POSIX
+  tcflush(userfd, TCOFLUSH);
+# else
+  (void) ioctl(userfd, TCFLUSH, (char *) 1);
+# endif
+
+  obufp = obuf;
+  obuffree += len;
+  PutStr(CL); /* I know that this exists... we abort in ansi.c otherwise */
+  if (ME) 
+    PutStr(ME); /* Turn off all atributes */
+  else
     {
-      write(userfd, obuf, l);
-      obufp = obuf;
-      obuffree += l;
+      if (UE) 
+	PutStr(UE); /* End underline JIC (Just in Case)*/
+      if (SE && ((UE && strcmp(UE,SE)) || !UE))
+	/* Put Standend if it exists and there is a different (or no) UE */
+	PutStr(SE);
     }
 }
+#endif
diff -ur screen-3.2.15/display.h screen-3.2.15.new/display.h
--- screen-3.2.15/display.h	Thu Nov  5 10:48:31 1992
+++ screen-3.2.15.new/display.h	Tue Nov 10 17:24:57 1992
@@ -73,7 +73,12 @@
   int	d_userfd;		/* fd of the tty */
   struct mode d_OldMode;	/* tty mode when screen was started */
   struct mode d_NewMode;	/* New tty mode */
+#ifdef ASYNC_OUTPUT
+  char  *d_obuf;		/* output buffer */
+  int   d_obuflen;
+#else
   char  d_obuf[IOSIZE];		/* output buffer */
+#endif
   char  *d_obufp;		/* pointer in buffer */
   int   d_obuffree;		/* free bytes in buffer */
 #ifdef COPY_PASTE
@@ -144,6 +149,9 @@
 #define OldMode		DISPLAY(d_OldMode)
 #define NewMode		DISPLAY(d_NewMode)
 #define obuf		DISPLAY(d_obuf)
+#ifdef ASYNC_OUTPUT
+#define obuflen		DISPLAY(d_obuflen)
+#endif
 #define obufp		DISPLAY(d_obufp)
 #define obuffree	DISPLAY(d_obuffree)
 #define copybuffer	DISPLAY(d_copybuffer)
@@ -166,6 +174,22 @@
 #define utmp_logintty	DISPLAY(d_utmp_logintty)
 #define loginhost	DISPLAY(d_loginhost)
 
+#ifdef ASYNC_OUTPUT
+void Resize_obuf();
+void NukePending();
+#define GRAIN 4096  /* Allocation grain size for output buffer */
+#define OUTPUT_BLOCK_SIZE 80  /* Block size of output to tty */
+#define OBUF_MAX 16000 
+   /* Maximum amount of buffered output before input is blocked */
+#define AddChar(c) \
+  { \
+    if (--obuffree == 0) \
+      Resize_obuf(); \
+    *obufp++ = (c); \
+  }
+
+#else
+
 #define AddChar(c) \
   { \
     *obufp++ = (c); \
@@ -172,4 +196,4 @@
     if (--obuffree == 0) \
       Flush(); \
   }
-
+#endif
diff -ur screen-3.2.15/screen.c screen-3.2.15.new/screen.c
--- screen-3.2.15/screen.c	Fri Nov  6 00:46:09 1992
+++ screen-3.2.15.new/screen.c	Wed Nov 11 01:07:03 1992
@@ -296,6 +296,12 @@
   disp->d_userfd = -1;
   disp->d_obufp = 0;
   disp->d_obuffree = 0;
+#ifdef ASYNC_OUTPUT
+  if (disp->d_obuf) free(disp->d_obuf);
+  disp->d_obuffree = 0;
+  disp->d_obuflen = 0;
+  disp->d_obuf = 0;
+#endif
 }
 
 int
@@ -1276,8 +1282,17 @@
       FD_ZERO(&w);
       for (display = displays; display; display = display->d_next)
 	{
-	  if (obufp != obuf)
-	    Flush();
+	  register int len;
+	  len = obufp - obuf;
+	  if (len != 0)
+ 	    {
+#ifdef ASYNC_OUTPUT
+	      FD_SET(userfd, &w);
+#else
+	      Flush();
+#endif
+ 	    }
+
 	  if (dfore == 0)
 	    {
 	      FD_SET(userfd, &r);
@@ -1302,6 +1317,11 @@
 	    continue;
 	  if (p->wlay->l_block)
 	    continue;
+#ifdef ASYNC_OUTPUT
+	  if ((obufp-obuf) > OBUF_MAX)
+	    continue;  
+	/* Don't accept input if there is too much output pending on display*/
+#endif
 	  FD_SET(p->w_ptyfd, &r);
 	}
       FD_SET(s, &r);
@@ -1407,7 +1427,7 @@
                 {
 		  if ((len = write(tmp, p->inbuf, p->inlen)) > 0)
 		    {
-		      if (p->inlen -= len)
+		      if ((p->inlen -= len))
 		        bcopy(p->inbuf + len, p->inbuf, p->inlen);
 		    }
 		  if (--nsel == 0)
@@ -1424,6 +1444,31 @@
 	  int maxlen;
 
 	  ndisplay = display->d_next;
+#ifdef ASYNC_OUTPUT
+	  if (FD_ISSET(userfd, &w)) 
+	    {
+	      int size = OUTPUT_BLOCK_SIZE;
+	      int len = obufp - obuf;
+
+	      nsel --;
+	      if (len < size) size = len;
+	      size = write(userfd, obuf, size);
+	      if (size > 0) 
+		{
+		  /* Shift the buffer back...
+		   * hope memcpy handles overlapping regions :) 
+		   */
+		  memcpy(obuf,obuf+size,len-size);
+		  obufp -= size;
+		  obuffree += size;
+		} 
+	      else
+		{
+		  if (errno != EWOULDBLOCK && errno != EPIPE)
+		    Msg(errno,"Error writing output to display");
+		}
+	    }
+#endif
 	  if (! FD_ISSET(userfd, &r))
 	    continue;
 	  nsel--;
@@ -1986,7 +2031,7 @@
   freetty(display);
   pid = userpid;
   debug2("display: %#x displays: %#x\n", (unsigned int)display, (unsigned int)displays);
-  for (dp = &displays; d = *dp; dp = &d->d_next)
+  for (dp = &displays; (d = *dp) ; dp = &d->d_next)
     if (d == display)
       break;
   ASSERT(d);

