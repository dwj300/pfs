C=gcc
CFLAGS= -Wall -g
CFLAGS2= -pthread -o

all: c1 c2 c3 server client grapevine cache test

c1: pfs.c test1-c1.c cache.c blockStructs.c
	$(CC) $(CFLAGS) $(CFLAGS2) c1 pfs.c test1-c1.c cache.c blockStructs.c

c2: pfs.c test1-c2.c cache.c blockStructs.c
	$(CC) $(CFLAGS) $(CFLAGS2) c2 pfs.c test1-c2.c cache.c blockStructs.c

c3: pfs.c test1-c3.c cache.c blockStructs.c
	$(CC) $(CFLAGS) $(CFLAGS2) c3 pfs.c test1-c3.c cache.c blockStructs.c

server: server.c
	$(CC) $(CFLAGS) $(CFLAGS2) server server.c

grapevine: grapevine.c dictionary.c
	$(CC) $(CFLAGS) $(CFLAGS2) grapevine grapevine.c dictionary.c

client: client.c
	$(CC) $(CFLAGS) $(CFLAGS2) client client.c

test: test_program.c pfs.c cache.c blockStructs.c
	$(CC) $(CFLAGS) $(CFLAGS2) test test_program.c pfs.c cache.c blockStructs.c

cache: test_cache.c cache.c blockStructs.c
	$(CC) $(CFLAGS) $(CFLAGS2) test_cache test_cache.c cache.c blockStructs.c

clean:
	rm c1 c2 c3 client server cache grapevine test
