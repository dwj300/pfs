#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "sharedresources.h"

typedef char byte;

int create_block(char *host, int port, int block_id);
int read_block(char *host, int port, int block_id, byte** data);
int write_block(char *host, int port, int block_id, byte* data);
int delete_block(char *host, int port, int block_id);
