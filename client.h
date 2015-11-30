#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int create_block(char *host, int port, int block_id);
int read_block(char *host, int port, int block_id, char **data);
int write_block(char *host, int port, int block_id, char *data);
int delete_block(char *host, int port, int block_id);
