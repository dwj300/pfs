#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cache.h"
#include "client.h"
#include "config.h"
#include "sharedresources.h"


int pfs_create(const char *filename, int stripe_width);
int pfs_open(const char *filename, const char mode);
ssize_t pfs_read(int filedes, void *buf, ssize_t nbyte, off_t offset, int *cache_hit);
ssize_t pfs_write(int filedes, const void *buf, size_t nbyte, off_t offset, int *cache_hit);
int pfs_close(int filedes);
int pfs_delete(const char *filename);
int pfs_fstat(int filedes, struct pfs_stat *buf);
void initialize(int argc, char **argv);

file_t files[MAX_FILES]; 
cache_t *cache;
int current_fd;
char *grapevine_host;
int grapevine_port;
char hostname[1024];
int my_port;
int client_id;

pthread_t revoker_thread;
