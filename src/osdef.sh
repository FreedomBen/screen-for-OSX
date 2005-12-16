#! /bin/sh

if test -z "$CC"; then
  CC=cc
fi
if test -z "$srcdir"; then
  srcdir=.
fi

sed < $srcdir/osdef.h.in -n -e '/^extern/s@.*[)* 	][)* 	]*\([^ *]*\) __P.*@/[)*, 	]\1[ 	(]/i\\\
\\/\\[^a-zA-Z_\\]\1 __P\\/d@p' > osdef1.sed
cat << EOF > osdef0.c
#include "config.h"
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <pwd.h>
#ifdef SHADOWPW
#include <shadow.h>
#endif
#ifndef sun
#include <sys/ioctl.h>
#endif
#ifndef NAMEDPIPE
#include <sys/socket.h>
#endif
#include "os.h"
#if defined(UTMPOK) && defined (GETTTYENT) && !defined(GETUTENT)
#include <ttyent.h>
#endif
EOF
cat << EOF > osdef2.sed
1i\\
/*
1i\\
 * This file is automagically created from osdef.sh -- DO NOT EDIT
1i\\
 */
EOF
$CC -I. -I$srcdir -E osdef0.c | sed -n -f osdef1.sed >> osdef2.sed
sed -f osdef2.sed < $srcdir/osdef.h.in > osdef.h
rm osdef0.c osdef1.sed osdef2.sed
