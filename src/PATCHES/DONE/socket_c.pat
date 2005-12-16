*** ..\3.17\socket.c	Sun Feb 02 16:59:24 1992
--- socket.c	Sun Feb 02 17:13:58 1992
***************
*** 41,47 ****
  #endif
  #include <sys/types.h>
  #include <sys/stat.h>
! #include <sys/file.h>
  #ifndef NAMEDPIPE
  #include <sys/socket.h>
  #endif
--- 41,49 ----
  #endif
  #include <sys/types.h>
  #include <sys/stat.h>
! #ifndef sgi
! # include <sys/file.h>
! #endif
  #ifndef NAMEDPIPE
  #include <sys/socket.h>
  #endif
***************
*** 140,150 ****
    return 1;
  }
  
! 
! /* socket mode 700 means we are Attached. 600 is detached.
!  * we return how many sockets we found. if it was exactly one, we come
   * back with a SockPath set to it and open it in a fd pointed to by fdp.
!  * if fdp == 0 we simply produce a list if all sockets.
   */
  int FindSocket(how, fdp)
  int how;
--- 142,152 ----
    return 1;
  }
  
! /*
!  * Socket mode 700 means we are Attached. 600 is detached.
!  * We return how many sockets we found. If it was exactly one, we come
   * back with a SockPath set to it and open it in a fd pointed to by fdp.
!  * If fdp == 0 we simply produce a list if all sockets.
   */
  int FindSocket(how, fdp)
  int how;
***************
*** 163,177 ****
      } foundsock[100];	/* 100 is hopefully enough. */
    int foundsockcount = 0;
  
!   /* user may or may not give us a (prefix) SockName. We want to search. */
    debug("FindSocket:\n");
    if (SockName)
      {
        debug1("We want to match '%s'\n", SockName);
        l = strlen(SockName);
! #ifdef FILNMLEN
!       if (l > FILNMLEN)
! 	l = FILNMLEN;
  #endif
      }
  
--- 165,179 ----
      } foundsock[100];	/* 100 is hopefully enough. */
    int foundsockcount = 0;
  
!   /* User may or may not give us a (prefix) SockName. We want to search. */
    debug("FindSocket:\n");
    if (SockName)
      {
        debug1("We want to match '%s'\n", SockName);
        l = strlen(SockName);
! #ifdef NAME_MAX
!       if (l > NAME_MAX)
! 	l = NAME_MAX;
  #endif
      }
  
***************
*** 185,191 ****
    found = *SockNamePtr;
    *SockNamePtr = '\0';
    if ((dirp = opendir(SockPath)) == NULL)
!     Msg(0, "Cannot opendir %s", SockPath);
    *SockNamePtr = found;
    found = 0;
    while ((dp = readdir(dirp)) != NULL)
--- 187,196 ----
    found = *SockNamePtr;
    *SockNamePtr = '\0';
    if ((dirp = opendir(SockPath)) == NULL)
!     {
!       Msg(0, "Cannot opendir %s", SockPath);
!       /* NOTREACHED */
!     }
    *SockNamePtr = found;
    found = 0;
    while ((dp = readdir(dirp)) != NULL)
***************
*** 238,246 ****
        debug2("FindSocket: %s has mode %04o...\n", Name, s);
        if (s == 0700 || s == 0600)
  	{
! 	  /* we try to connect through the socket. if successfull, 
! 	   * thats o.k. otherwise we record that mode as -1.
! 	   * MakeClientSocket must be careful not to block forever.
  	   */
  	  if ((s = MakeClientSocket(0, Name)) == -1)
  	    { 
--- 243,252 ----
        debug2("FindSocket: %s has mode %04o...\n", Name, s);
        if (s == 0700 || s == 0600)
  	{
! 	  /*
! 	   * We try to connect through the socket. If successfull, 
! 	   * thats o.k. Otherwise we record that mode as -1.
! 	   * MakeClientSocket() must be careful not to block forever.
  	   */
  	  if ((s = MakeClientSocket(0, Name)) == -1)
  	    { 
***************
*** 300,306 ****
  	        printf("Your inventory:\n");
  	      else
  #endif
! 	      printf((foundsockcount>1) ? "There are screens on:\n" : "There is a screen on:\n");
  	    }
            else
  	    {
--- 306,313 ----
  	        printf("Your inventory:\n");
  	      else
  #endif
! 	      printf((foundsockcount > 1) ?
! 	             "There are screens on:\n" : "There is a screen on:\n");
  	    }
            else
  	    {
***************
*** 309,332 ****
  	        printf("Nothing fitting exists in the game:\n");
  	      else
  #endif
! 	      printf((foundsockcount>1) ? "There are screens on:\n" : "There is a screen on:\n");
  	    }
            break;
          case 1:
  #ifdef NETHACK
            if (nethackflag)
!             printf((foundsockcount>1) ? "Prove thyself worthy or perish:\n" : "You see here a good looking screen:\n");
            else
  #endif
!           printf((foundsockcount>1) ? "There are several screens on:\n" : "There is a possible screen on:\n");
            break;
          default:
  #ifdef NETHACK
            if (nethackflag)
!             printf((foundsockcount>1) ? "You may whish for a screen, what do you want?\n" : "You see here a screen:\n");
            else
  #endif
!           printf((foundsockcount>1) ? "There are several screens on:\n" : "There is a screen on:\n");
            break;
          }
        for (s = 0; s < foundsockcount; s++)
--- 316,347 ----
  	        printf("Nothing fitting exists in the game:\n");
  	      else
  #endif
! 	      printf((foundsockcount > 1) ?
! 	             "There are screens on:\n" : "There is a screen on:\n");
  	    }
            break;
          case 1:
  #ifdef NETHACK
            if (nethackflag)
!             printf((foundsockcount > 1) ?
!                    "Prove thyself worthy or perish:\n" : 
!                    "You see here a good looking screen:\n");
            else
  #endif
!           printf((foundsockcount > 1) ? 
!                  "There are several screens on:\n" :
!                  "There is a possible screen on:\n");
            break;
          default:
  #ifdef NETHACK
            if (nethackflag)
!             printf((foundsockcount > 1) ? 
!                    "You may whish for a screen, what do you want?\n" : 
!                    "You see here a screen:\n");
            else
  #endif
!           printf((foundsockcount > 1) ?
!                  "There are several screens on:\n" : "There is a screen on:\n");
            break;
          }
        for (s = 0; s < foundsockcount; s++)
***************
*** 397,408 ****
    struct stat st;
  
    strcpy(SockNamePtr, SockName);
! #ifdef FILNMLEN
!   if (strlen(SockNamePtr) > FILNMLEN)
      {
        debug2("MakeClientSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, FILNMLEN);
!       SockNamePtr[FILNMLEN] = '\0';
      }
  #endif
  
--- 412,423 ----
    struct stat st;
  
    strcpy(SockNamePtr, SockName);
! #ifdef NAME_MAX
!   if (strlen(SockNamePtr) > NAME_MAX)
      {
        debug2("MakeClientSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, NAME_MAX);
!       SockNamePtr[NAME_MAX] = '\0';
      }
  #endif
  
***************
*** 457,468 ****
    register int s = 0;
  
    strcpy(SockNamePtr, name);
! #ifdef FILNMLEN
!   if (strlen(SockNamePtr) > FILNMLEN)
      {
        debug2("MakeClientSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, FILNMLEN);
!       SockNamePtr[FILNMLEN] = '\0';
      }
  #endif
    
--- 472,483 ----
    register int s = 0;
  
    strcpy(SockNamePtr, name);
! #ifdef NAME_MAX
!   if (strlen(SockNamePtr) > NAME_MAX)
      {
        debug2("MakeClientSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, NAME_MAX);
!       SockNamePtr[NAME_MAX] = '\0';
      }
  #endif
    
***************
*** 496,507 ****
      Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, SockName);
! #ifdef FILNMLEN
!   if (strlen(SockNamePtr) > FILNMLEN)
      {
        debug2("MakeServerSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, FILNMLEN);
!       SockNamePtr[FILNMLEN] = '\0';
      }
  #endif
  
--- 511,522 ----
      Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, SockName);
! #ifdef NAME_MAX
!   if (strlen(SockNamePtr) > NAME_MAX)
      {
        debug2("MakeServerSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, NAME_MAX);
!       SockNamePtr[NAME_MAX] = '\0';
      }
  #endif
  
***************
*** 548,554 ****
      Msg(errno, "listen");
  #ifdef F_SETOWN
    fcntl(s, F_SETOWN, getpid());
!   debug1("Serversocket owned by %d\n",fcntl(s, F_GETOWN, 0));
  #endif /* F_SETOWN */
    return s;
  }
--- 563,569 ----
      Msg(errno, "listen");
  #ifdef F_SETOWN
    fcntl(s, F_SETOWN, getpid());
!   debug1("Serversocket owned by %d\n", fcntl(s, F_GETOWN, 0));
  #endif /* F_SETOWN */
    return s;
  }
***************
*** 565,576 ****
      Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, name);
! #ifdef FILNMLEN
!   if (strlen(SockNamePtr) > FILNMLEN)
      {
        debug2("MakeClientSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, FILNMLEN);
!       SockNamePtr[FILNMLEN] = '\0';
      }
  #endif
  
--- 580,591 ----
      Msg(errno, "socket");
    a.sun_family = AF_UNIX;
    strcpy(SockNamePtr, name);
! #ifdef NAME_MAX
!   if (strlen(SockNamePtr) > NAME_MAX)
      {
        debug2("MakeClientSocket: '%s' truncated to %d chars\n",
! 	     SockNamePtr, NAME_MAX);
!       SockNamePtr[NAME_MAX] = '\0';
      }
  #endif
  
***************
*** 779,785 ****
    while (left > 0 && (len = read(s, p, left)) > 0)
      {
        /*		if (p == (char *)&m)
!        *		{	if (fcntl(s,F_SETFL,O_NDELAY) == -1)
         *			{	Msg(errno, "fcntl O_NDELAY !");
         *				return;
         *			}
--- 794,800 ----
    while (left > 0 && (len = read(s, p, left)) > 0)
      {
        /*		if (p == (char *)&m)
!        *		{	if (fcntl(s, F_SETFL, O_NDELAY) == -1)
         *			{	Msg(errno, "fcntl O_NDELAY !");
         *				return;
         *			}
***************
*** 831,837 ****
  	ExecCreate(&m);
        break;
      case MSG_CONT:
!       debug3("RecMsg: apid=%d,was %d, Detached=%d\n", m.m.attach.apid, AttacherPid, Detached);
  
  #ifdef SECURE_MSG_CONT
        if (m.m.attach.apid != AttacherPid || !Detached)
--- 846,853 ----
  	ExecCreate(&m);
        break;
      case MSG_CONT:
!       debug3("RecMsg: apid=%d,was %d, Detached=%d\n", m.m.attach.apid,
!              AttacherPid, Detached);
  
  #ifdef SECURE_MSG_CONT
        if (m.m.attach.apid != AttacherPid || !Detached)
***************
*** 879,886 ****
  	  debug1("ALERT: Cannot open %s!\n", m.m.attach.tty);
  #ifdef NETHACK
            if (nethackflag)
! 	    Msg(errno, "You can't open (%s). Perhaps there's a Monster behind it."
! 	      ,m.m.attach.tty);
            else
  #endif
  	  Msg(errno, "Attach: Could not open %s", m.m.attach.tty);
--- 895,903 ----
  	  debug1("ALERT: Cannot open %s!\n", m.m.attach.tty);
  #ifdef NETHACK
            if (nethackflag)
! 	    Msg(errno, 
! 	        "You can't open (%s). Perhaps there's a Monster behind it.",
! 	        m.m.attach.tty);
            else
  #endif
  	  Msg(errno, "Attach: Could not open %s", m.m.attach.tty);
***************
*** 904,926 ****
  	int pgrp, zero = 0;
  
  	if (ioctl(0, TIOCGPGRP, &pgrp) == -1)
! 	  debug1("tcgetpgrp: %d\n", errno);	/* save old pgrp from
! 						   tty */
  
  	if (ioctl(0, TIOCSPGRP, &zero) == -1)
! 	  debug1("tcsetpgrp: %d\n", errno);	/* detach tty from proc
! 						   grp */
  #ifdef hpux
  	setpgrp();
  #else
  	if (setpgrp(0, getpid()) == -1)
! 	  debug1("setpgrp: %d\n", errno);	/* make me process group
! 					      	   leader */
  #endif
  
! 	close(0);	/* reopen tty, now we should get it as our
! 			 * tty */
! 	(void) secopen(m.m.attach.tty, O_RDWR | O_NDELAY, 0);	/* */
  
  #ifdef hpux
  	setpgrp2(0, pgrp);
--- 921,939 ----
  	int pgrp, zero = 0;
  
  	if (ioctl(0, TIOCGPGRP, &pgrp) == -1)
! 	  debug1("tcgetpgrp: %d\n", errno);	/* save old pgrp from tty */
  
  	if (ioctl(0, TIOCSPGRP, &zero) == -1)
! 	  debug1("tcsetpgrp: %d\n", errno);	/* detach tty from proc grp */
  #ifdef hpux
  	setpgrp();
  #else
  	if (setpgrp(0, getpid()) == -1)
! 	  debug1("setpgrp: %d\n", errno);	/* make me proc group leader */
  #endif
  
! 	close(0);	/* reopen tty, now we should get it as our tty */
! 	(void)secopen(m.m.attach.tty, O_RDWR | O_NDELAY, 0);	/* */
  
  #ifdef hpux
  	setpgrp2(0, pgrp);
***************
*** 1064,1071 ****
  #if defined(_SEQUENT_) && !defined(NAMEDPIPE)
  #undef connect
  
! sconnect(s,sapp,len)
! int s,len;
  struct sockaddr *sapp;
  {
    register struct sockaddr_un *sap;
--- 1077,1084 ----
  #if defined(_SEQUENT_) && !defined(NAMEDPIPE)
  #undef connect
  
! sconnect(s, sapp, len)
! int s, len;
  struct sockaddr *sapp;
  {
    register struct sockaddr_un *sap;
***************
*** 1072,1083 ****
    struct stat st;
    int x;
  
!   sap=(struct sockaddr_un *)sapp;
    if (stat(sap->sun_path,&st))
!     return(-1);
!   chmod(sap->sun_path,0);
!   x=connect(s, (struct sockaddr *) sap, len);
!   chmod(sap->sun_path,st.st_mode);
!   return(x);
  }
  #endif
--- 1085,1096 ----
    struct stat st;
    int x;
  
!   sap = (struct sockaddr_un *)sapp;
    if (stat(sap->sun_path,&st))
!     return -1;
!   chmod(sap->sun_path, 0);
!   x = connect(s, (struct sockaddr *) sap, len);
!   chmod(sap->sun_path, st.st_mode);
!   return x;
  }
  #endif
