#include <fcntl.h>
#include <sys/signal.h>

static char PtyProto[] = "/dev/ptyXY";
static char TtyProto[] = "/dev/ttyXY";

static char PtyName[20], TtyName[20];

main()
{
  register char *p, *q, *l, *d;
  register int f, tf;

  signal(SIGHUP, SIG_IGN);
  setsid();

  strcpy(PtyName, PtyProto);
  strcpy(TtyName, TtyProto);

  for (p = PtyName; *p != 'X'; ++p)
    ;
  for (q = TtyName; *q != 'X'; ++q)
    ;
  for (l = "qpr"; (*p = *l) != '\0'; ++l)
    {
      for (d = "0123456789abcdef"; (p[1] = *d) != '\0'; ++d)
	{
	  if ((f = open(PtyName, O_RDWR)) != -1)
	    {
	      q[0] = *l;
	      q[1] = *d;
	      if ((tf = open(TtyName, O_RDWR)) != -1)
		{
		  close(f);
		  printf("OK: master %s  slave %s\n", PtyName, TtyName);
		  pause();
		  close(tf);
		  exit(0);
		}
	      close(f);
	    }
	}
    }
  exit(10);
}
