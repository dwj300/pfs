C=gcc
CFLAGS= -Wall -g -pthread
CFLAGS2= -lpthread -o

all: c1 c2 c3 server client grapevine

c1: pfs.c test1-c1.c
	$(CC) $(CFLAGS) $(CFLAGS2) c1 pfs.c test1-c1.c

c2: pfs.c test1-c2.c
	$(CC) $(CFLAGS) $(CFLAGS2) c2 pfs.c test1-c2.c

c3: pfs.c test1-c3.c
	$(CC) $(CFLAGS) $(CFLAGS2) c3 pfs.c test1-c3.c

server: server.c
	$(CC) $(CFLAGS) $(CFLAGS2) server server.c

grapevine: grapevine.c dictionary.c
	$(CC) $(CFLAGS) $(CFLAGS2) grapevine grapevine.c dictionary.c

client: client.c
	$(CC) $(CFLAGS) $(CFLAGS2) client client.c

cache: cache.c
	$(CC) cache.c blockStructs.c -pthread -Wall -g -o cache

test: test_program.c pfs.c
	$(CC) $(CFLAGS) $(CFLAGS2) test test_program.c pfs.c

clean:
	rm c1 c2 c3 client server cache grapevine
