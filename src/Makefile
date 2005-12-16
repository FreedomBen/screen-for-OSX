#
# Makefile for screen
#
# See config.h (copied from config.h-dist) for configuration options.
#

# If you choose to compile with the tried and true:
CC= cc
CFLAGS= -O

# If you're using GNU C, be sure to use the -traditional flag:
#CC= gcc
#CFLAGS= -traditional -O

LIBS= -ltermcap

CFILES= screen.c ansi.c
OFILES= screen.o ansi.o

screen: $(OFILES)
	$(CC) $(CFLAGS) -s -o screen $(OFILES) $(LIBS)

screen.o: screen.c screen.h config.h
	$(CC) $(CFLAGS) -c screen.c

ansi.o: ansi.c screen.h config.h
	$(CC) $(CFLAGS) -c ansi.c

lint:
	lint -I. screen.c ansi.c

saber:
	#load $(CFLAGS) screen.c ansi.c $(LIBS)
