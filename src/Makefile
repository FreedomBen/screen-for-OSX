# @(#)Makefile	1.5 88/12/13 22:13:36
# The following options can be set:
#
# -DLOCK      -- you want to have the possibility to lock your terminal
#		 while you are running screen
#		 (Please provide a suitable value for "DefaultLock" in
#		  screen.c - reflecting your sytem responsibilities)
# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
#                (an array of three doubles called _avenrun; it is read
#                from /dev/kmem; _avenrun is taken from the namelist of
#                /vmunix).  Don't set this on a Sun under SunOS 3.x.
# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
#                and the getttyent(3) library functions.
# -DSUNOS     -- If you have SunOS 4.0 or up, you should set this to use
#                the kernel virtual memory library, when you want
#                to have LOADAV ( set LOADAV also).
# -DSEQUENT   -- eliminates bugs in DYNIX and chooses different
#                pty naming for the sequent.
#
# You should install as set-uid with owner root, so that it can read/write
# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.
SHELL=/bin/sh
CFLAGS= -g

BINDIR=/usr/local/bin
PART=1
MANDIR=/usr/local/man/man$(PART)

OPTIONS=-DLOCK -DSEQUENT -DSEQUENT -DLOADAV
LIBS= -ltermcap

CFILES= screen.c ansi.c
OFILES= screen.o ansi.o

screen:$(P) $(OFILES) screen.1
	$(CC) $(CFLAGS) -o screen $(OFILES) $(LIBS)

screen.o:$(P) screen.c screen.h
	$(CC) $(OPTIONS) $(CFLAGS) -c screen.c

ansi.o:$(P) ansi.c screen.h
	$(CC) $(CFLAGS) -c ansi.c

install: screen screen.1
	install -c -s -o root -g wheel -m 4755 screen $(BINDIR)
	install -c -m 444 screen.1 $(MANDIR)/screen.$(PART)

clean:
	rm -f *.o screen
	sccs clean

screen.c:
	sccs get screen.c

screen.h:
	sccs get screen.h

ansi.c:
	sccs get ansi.c
