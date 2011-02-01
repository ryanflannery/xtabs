
CC?=/usr/bin/cc
# NOTE: xcb does not conform to c89
CFLAGS+=-c -std=c99 -Wall -Wextra -I/usr/X11R6/include
LDFLAGS+=-L/usr/X11R6/lib -lxcb -lxcb-atom -lxcb-icccm

OBJS=clients.o events.o session.o xtabs.o xutil.o

xtabs: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $<

clean:
	rm -f xtabs $(OBJS)
