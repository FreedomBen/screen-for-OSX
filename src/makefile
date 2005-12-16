# The following options can be set:
#
# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
#                (an array of three doubles called _avenrun; it is read
#                from /dev/kmem; _avenrun is taken from the namelist of
#                /vmunix).  Don't set this on a Sun.
# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
#                and the getttyent(3) library functions.
#
# -DsysV      -- For system V
#
# You should install as set-uid with owner root, so that it can read/write
# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.

OPTIONS=-DsysV
CFLAGS=-GO

CFILES= screen_trace.c ansi.c Berklib.c
OFILES= screen_trace.o tr.c ansi.o Berklib.o

screen_trace: $(OFILES)
	cc $(CFLAGS) -o screen_trace $(OFILES) -lcurses

screen_trace.o: screen_trace.c screen.h
	cc $(OPTIONS) $(CFLAGS) -c screen_trace.c

tr.o: tr.c 
	cc $(CFLAGS) -c tr.c

ansi.o: ansi.c screen.h
	cc $(CFLAGS) -c ansi.c
