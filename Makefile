C=gcc
CFLAGS= -Wall
CFLAGS2= -lpthread -o

all: c1 c2 c3

c1:
	$(CC) $(CFLAGS) $(CFLAGS2) c1 pfs.c test1-c1.c

c2:
	$(CC) $(CFLAGS) $(CFLAGS2) c2 pfs.c test1-c2.c

c3:
	$(CC) $(CFLAGS) $(CFLAGS2) c3 pfs.c test1-c3.c

clean:
	rm *.o c1 c2 c3
