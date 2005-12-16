#!/bin/sh

test -z "$AWK" && AWK=awk

rm -f comm.h
cat << EOF > comm.h
/*
 * This file is automagically created from comm.c -- DO NOT EDIT
 */

struct comm
{
  char *name;
  int flags;
};


#define ARGS_MASK	(3)

#define ARGS_ZERO	(0)
#define ARGS_ONE	(1)
#define ARGS_TWO	(2)
#define ARGS_THREE	(3)

#define ARGS_PLUSONE	(1<<2)
#define ARGS_ORMORE	(1<<3)

#define NEED_FORE	(1<<4)	/* this command needs a fore window */
#define NEED_DISPLAY	(1<<5)	/* this command needs a display */

#define ARGS_ZEROONE	(ARGS_ZERO|ARGS_PLUSONE)
#define ARGS_ONETWO	(ARGS_ONE |ARGS_PLUSONE)
#define ARGS_TWOTHREE	(ARGS_TWO |ARGS_PLUSONE)

struct action
{
  int nr;
  char **args;
};

#define RC_ILLEGAL -1

EOF
$AWK < comm.c >> comm.h '
/^  [{] ".*/	{   if (old > $2) {
		printf("***ERROR: %s <= %s !!!\n\n", $2, old);
		exit 1;
	    }
	old = $2;
	}
'
cc -E comm.c > comm.cpp
sed < comm.cpp \
  -n \
  -e '/^  { "/y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/' \
  -e '/^  { "/s/^  { "\([^"]*\)".*/\1/p' \
| $AWK '
/.*/ {	printf "#define RC_%s %d\n",$0,i++;
     }
END  {	printf "\n#define RC_LAST %d\n",i-1;
     }
' >> comm.h
chmod a-w comm.h
rm -f comm.cpp
