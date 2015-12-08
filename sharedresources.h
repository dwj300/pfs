#pragma once

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>

#define true 1
#define false 0
#define BLOCK_IS_FREE -1
#define MAX_BLOCKS 100

typedef int bool;
typedef char byte;
typedef int32_t global_block_id_t;
typedef int32_t global_server_id_t;

typedef struct fs_block {
    int server_id;
    int block_id;
} fs_block_t;

typedef struct recipe {
    int num_blocks;
    fs_block_t blocks[MAX_BLOCKS];
} recipe_t;

typedef struct file {
    const char* filename;
    struct pfs_stat* stat;
    recipe_t* recipe;
} file_t;

typedef struct server {
    char hostname[255];
    int port;
} server_t;

int connect_socket(char *host, int port);
server_t* get_server(int server_id);

server_t *servers;
