
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>

main()
{
  fd_set r, w;
  int n, fd;

  FD_ZERO(&r);
  FD_ZERO(&w);
  if ((fd = open("/dev/tty", O_RDWR)) < 0)
    {
      perror("open");
      exit(1);
    }
  FD_SET(fd, &r);
  FD_SET(fd, &w);
  sleep(1);
  n = select(fd + 1, &r, &w, 0, 0);
  printf("Select returned %d\n", n);
  if (FD_ISSET(fd, &r))
    printf("Can read from %d\n", fd);
  if (FD_ISSET(fd, &w))
    printf("Can write to %d\n", fd);
  close(fd);
}

