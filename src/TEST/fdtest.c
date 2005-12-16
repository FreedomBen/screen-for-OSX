#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

main()
{
  test(0);
  test(1);
  test(2);
}

char *opfl[] = {"RDONLY", "WRONLY", "RDWR", "????"};

test(i)
int i;
{
  struct stat statb;
  int j;

  printf("\nDescriptor %d:\n", i);
  if (fstat(i, &statb))
    {
      printf("Not connected (%d)\n", errno);
      return;
    }
  if (isatty(i) == 0)
    printf("Not a typewriter\n");
  if ((j = fcntl(i, F_GETFL, 0)) == -1)
    {
      printf("fcntl F_GETFL (%d)\n", errno);
      return;
    }
  printf("Flags: %s%s%s%s\n", opfl[j & 3], (j & 4) ? " NDELAY" : "",
  (j & 8) ? " APPEND" : "",
  (j & 16) ? " SYNC" : "");
}

