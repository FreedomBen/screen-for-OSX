#!/bin/sh

if test -z "$AWK"; then
  AWK=awk
fi
if test -z "$srcdir"; then
  srcdir=.
fi

rm -f term.h
cat << EOF > term.h
/*
 * This file is automagically created from term.c -- DO NOT EDIT
 */

#define T_FLG 0
#define T_NUM 1
#define T_STR 2

struct term
{
  char *tcname;
  int type;
};

union tcu
{
  int flg;
  int num;
  char *str;
};

EOF

#
# SCO-Unix sufferers may need to use the following lines:
# perl -p < ${srcdir}/term.c \
#  -e 's/"/"C/ if /"[A-Z]."/;' \
#  -e 'y/[a-z]/[A-Z]/ if /"/;' \
#
sed < ${srcdir}/term.c \
  -e '/"[A-Z]."/s/"/"C/' \
  -e '/"/y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/' \
| $AWK '
/^  [{] ".*$/{
a=substr($2,2,length($2)-3);
b=substr($3,3,3);
if (nolist == 0)
  printf "#define %s (d_tcs[%d].%s)\n",a,s,b
s++;
}
/\/* define/{
printf "#define %s %d\n",$3,s
}
/\/* nolist/{
nolist = 1;
}
/\/* list/{
nolist = 0;
}
' | sed -e s/NUM/num/ -e s/STR/str/ -e s/FLG/flg/ \
>> term.h
chmod a-w term.h

