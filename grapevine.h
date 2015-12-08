#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "sharedresources.h"
#include "dictionary.h"
#include "config.h"
#include "client.h"

server_t *servers;
dictionary_t files;
int current_block_id;