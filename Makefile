C=gcc
CFLAGS= -Wall -g -std=gnu99
CFLAGS2= -pthread -o

all: c1 c2 c3 server client grapevine cache test test1 test2 test3

c1: pfs.c test1-c1.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) c1 pfs.c test1-c1.c cache.c blockStructs.c client.c sharedresources.c

c2: pfs.c test1-c2.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) c2 pfs.c test1-c2.c cache.c blockStructs.c client.c sharedresources.c

c3: pfs.c test1-c3.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) c3 pfs.c test1-c3.c cache.c blockStructs.c client.c sharedresources.c

server: server.c
	$(CC) $(CFLAGS) $(CFLAGS2) server server.c

grapevine: grapevine.c dictionary.c sharedresources.c client.c
	$(CC) $(CFLAGS) $(CFLAGS2) grapevine grapevine.c dictionary.c sharedresources.c client.c

client: test_client.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) client test_client.c sharedresources.c

test: test_program.c pfs.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) test test_program.c pfs.c cache.c blockStructs.c client.c sharedresources.c

test1: test1.c pfs.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) test1 test1.c pfs.c cache.c blockStructs.c client.c sharedresources.c

test2: test2.c pfs.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) test2 test2.c pfs.c cache.c blockStructs.c client.c sharedresources.c

test3: test_program.c pfs.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) test3 test3.c pfs.c cache.c blockStructs.c client.c sharedresources.c

cache: test_cache.c cache.c blockStructs.c client.c sharedresources.c
	$(CC) $(CFLAGS) $(CFLAGS2) test_cache test_cache.c cache.c blockStructs.c client.c sharedresources.c

clean:
	rm test1 test2 test3 c1 c2 c3 client server cache grapevine test test_cache dict dummyc; rm -r *.dSYM
