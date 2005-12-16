
struct event
{
  struct event *next;
  void (*handler) __P((struct event *, char *));
  char *data;
  int fd;
  int type;
  int pri;
  struct timeval timeout;
  int queued;		/* in evs queue */
  int active;		/* in fdset */
  int *condpos;		/* only active if condpos - condneg > 0 */
  int *condneg;
};

#define EV_TIMEOUT	0
#define EV_READ		1
#define EV_WRITE	2
#define EV_ALWAYS	3
