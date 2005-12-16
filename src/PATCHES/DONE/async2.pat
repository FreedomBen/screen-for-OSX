From tym@bruce.cs.monash.edu.au Wed Nov 11 08:24 MET 1992
Received: from bruce.cs.monash.edu.au by immd4.informatik.uni-erlangen.de with SMTP;
	id AA01936 (5.65c-5/7.3m-FAU); Wed, 11 Nov 1992 08:23:39 +0100
Received: by bruce (5.57/1.34)
	id AA10241; Wed, 11 Nov 92 18:23:16 +1100
Received: by dibbler.cs.monash.edu.au (4.1)
        id AA29409; Wed, 11 Nov 92 18:23:09 EST
        (from tym for mlschroe@immd4.informatik.uni-erlangen.de)
From: tym@bruce.cs.monash.edu.au (Tim MacKenzie)
Message-Id: <9211110723.AA29409@dibbler.cs.monash.edu.au>
Subject: Patches upon patches for async output.
To: jnweiger@immd4.informatik.uni-erlangen.de (Juergen Weigert),
        mlschroe@immd4.informatik.uni-erlangen.de (Michael Schroeder)
Date: Wed, 11 Nov 1992 18:23:06 +1000 (EST)
Cc: tym@dibbler.cs.monash.edu.au (Tim MacKenzie)
Organization: Computer Science, Monash University, Clayton, Vic, Australia.
Phone: +61 3 565 5375
X-Mailer: ELM [version 2.4 PL5]
Mime-Version: 1.0
Content-Type: text/plain; charset=US-ASCII
Content-Transfer-Encoding: 7bit
Content-Length: 1392      
Status: RO

The following patches to screen.c and display.c repair some problems in
my patch that I sent yesterday...
-- 
-Tim MacKenzie (tym@dibbler.cs.monash.edu.au)

--- display.c.orig	Wed Nov 11 18:15:26 1992
+++ display.c	Wed Nov 11 18:18:14 1992
@@ -1249,8 +1249,11 @@
 		wr = select(userfd+1,NULL,&set,NULL,NULL);
 	      } while (wr < 0 && errno == EINTR);
 	    }
-	  else if (errno != EINTR && errno != EPIPE)
-	    Msg(errno,"Writing to display");
+	  else if (errno != EINTR) 
+	    {
+	      Msg(errno,"Writing to display");
+	      break;
+	    }
 	}
       else
 	{
@@ -1259,6 +1262,7 @@
 	  l -= wr;
 	}
     }
+  obuffree += l;
   obufp = obuf;
 }
 
@@ -1291,7 +1295,6 @@
 {/* Nuke pending output in current display, clear screen */
   register int len;
 
-  write(userfd,"",1);
   len = obufp - obuf;
   
   /* Throw away any output that we can... */
--- screen.c.orig	Wed Nov 11 18:12:46 1992
+++ screen.c	Wed Nov 11 18:17:16 1992
@@ -1318,7 +1318,7 @@
 	  if (p->wlay->l_block)
 	    continue;
 #ifdef ASYNC_OUTPUT
-	  if ((obufp-obuf) > OBUF_MAX)
+	  if (display && (obufp-obuf) > OBUF_MAX)
 	    continue;  
 	/* Don't accept input if there is too much output pending on display*/
 #endif
@@ -1464,7 +1464,7 @@
 		} 
 	      else
 		{
-		  if (errno != EWOULDBLOCK && errno != EPIPE)
+		  if (errno != EWOULDBLOCK)
 		    Msg(errno,"Error writing output to display");
 		}
 	    }

