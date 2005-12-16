# Directory where you want to install the files that are needed
# by screen at runtime:
#
LIBPATH= /usr/local/lib/screen

# Directory where you want to install the screen program:
#
BINPATH= /usr/local

# Directory where you want to install the manual page for screen:
#
MANPATH= /usr/man/man1

# The following compile-time options can be set:
#
# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
#                (an array of three doubles called _avenrun; it is read
#                from /dev/kmem; _avenrun is taken from the namelist of
#                /vmunix).
# -DSUNLOADAV -- the load average maintained by the kernel is in the
#                Sun format (three longs).  Set this in addition to
#                LOADAV.
# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
#                and the getttyent(3) library functions.
#
# -DUSEBCOPY  -- use the bcopy() from the system's C-library.  If this
#                is set, bcopy must support overlapping source and
#                destination.  If USEBCOPY is not set, screen uses its
#                own version of bcopy.
#
OPTIONS= -DLOADAV -DUSEBCOPY

CFILES= screen.c ansi.c
OFILES= screen.o ansi.o

screen: $(OFILES)
	$(CC) $(CFLAGS) -o screen $(OFILES) -ltermcap

screen.o: screen.c screen.h
	$(CC) $(OPTIONS) -DLIBPATH=\"$(LIBPATH)\" $(CFLAGS) -c screen.c

ansi.o: ansi.c screen.h
	$(CC) $(CFLAGS) -c ansi.c

# You should install as set-uid with owner root, so that it can read/write
# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.
#
install: screen
	install -c -m 644 COPYING $(LIBPATH)
	install -c -m 644 HELP $(LIBPATH)
	install -c -m 644 screen.1 $(MANPATH)
	install -c screen $(BINPATH)
	chmod 4755 $(BINPATH)/screen
	chown root $(BINPATH)/screen

clean:
	rm -f $(OFILES) screen

dist:
	tar cvf - README COPYING HELP Makefile screen.1 screen.h screen.c\
		  ansi.c | compress > screen.tar.Z
