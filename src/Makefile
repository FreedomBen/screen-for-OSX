# The following options can be set:
#
# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
#                (an array of three doubles called _avenrun; it is read
#                from /dev/kmem; _avenrun is taken from the namelist of
#                /vmunix).
# -DLOCK      -- you want to have the possibility to lock your terminal
#		 while you are running screen
#		 (Default lock comMan is "/usr/local/bin/lck", set
#	   	  -DLOCKCMD="lockcmd" ).
# -DSUNLOADAV -- the load average maintained by the kernel is in the
#                Sun format (three longs).  Set this in addition to
#                LOADAV.
# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
#                and the getttyent(3) library functions.
# -DSUNOS     -- If you have SunOS 4.0 or up, you should set this to use
#                the kernel virtual memory library, when you want
#                to have LOADAV ( set LOADAV also).
#
# -DUSEBCOPY  -- use the bcopy() from the system's C-library.  If this
#                is set, bcopy must support overlapping source and
#                destination.  If USEBCOPY is not set, screen uses its
#                own version of bcopy.
#
# You should install as set-uid with owner root, so that it can read/write
# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.

OPTIONS= -DLOCK
# CFLAGS = -O
CFLAGS = -g
# disallow dereferencing through NULL pointers
LDFLAGS = 
# for BSD you only need termcap
TERMCAP = -ltermcap
# for hpux and other SYSV, you may need curses
# TERMCAP = -lcurses
# if you have defined SUNOS, you must include the kvm library
LIBS = -lcurses
# for esd5.2:
# LIBS = -lbsd

BINDIR=/usr/local/bin
PART=1
MANDIR=/usr/local/man/man$(PART)


CFILES= screen.c ansi.c mark.c
OFILES= screen.o ansi.o mark.o

screen: $(OFILES)
	$(CC) $(LDFLAGS) $(CFLAGS) -o screen $(OFILES) $(TERMCAP) $(LIBS)

all: screen screen.1

screen.o: screen.c screen.h
	$(CC) $(OPTIONS) $(CFLAGS) -c screen.c

ansi.o: ansi.c screen.h
	$(CC) $(CFLAGS) -c ansi.c

mark.o: mark.c screen.h
	$(CC) $(CFLAGS) -c mark.c

lck.o: lck.c
	$(CC) $(CFLAGS) -c lck.c

install: screen screen.1
	install -c -s -o root -g wheel -m 4755 screen $(BINDIR)
	install -c -m 444 screen.1 $(MANDIR)/screen.$(PART)

clean:
	rm -f *.o screen

clobber: clean
	sccs clean
