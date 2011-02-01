
CC?=/usr/bin/cc
#CFLAGS+=-c -std=c89 -Wall -Wextra -Wno-unused-value -I/usr/X11R6/include
CFLAGS+=-c -Wall -Wextra -I/usr/X11R6/include
LDFLAGS+=-L/usr/X11R6/lib -lxcb -lxcb-atom -lxcb-icccm

OBJS=xtabs.o

xtabs: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $<

clean:
	rm -f xtabs $(OBJS)
