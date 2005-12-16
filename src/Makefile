# The following options can be set:
#
# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
#                (an array of three doubles called _avenrun; it is read
#                from /dev/kmem; _avenrun is taken from the namelist of
#                /vmunix).  Don't set this on a Sun.
# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
#                and the getttyent(3) library functions.
#
# You should install as set-uid with owner root, so that it can read/write
# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.

OPTIONS= -DLOADAV

CFILES= screen.c ansi.c
OFILES= screen.o ansi.o

screen: $(OFILES)
	cc $(CFLAGS) -o screen $(OFILES) -ltermcap

screen.o: screen.c screen.h
	cc $(OPTIONS) $(CFLAGS) -c screen.c

ansi.o: ansi.c screen.h
	cc $(CFLAGS) -c ansi.c
