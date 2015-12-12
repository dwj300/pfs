#pragma once

#define h_addr h_addr_list[0] /* for backward compatibility */

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>

#define true 1
#define false 0
#define BLOCK_IS_FREE -1
#define MAX_BLOCKS 100
#define INVALID_TOKEN -1
#define INFINITE_TOKEN -1
#define INVALID_FILE -1

typedef int bool;
typedef char byte;
typedef int32_t global_block_id_t;
typedef int32_t global_server_id_t;

typedef struct token {
    int start_block;
    int end_block;
    int client_id;
} token_t;

typedef struct token_node {
    token_t* token;
    struct token_node* next;
} token_node_t;

typedef struct fs_block {
    int server_id;
    int block_id;
} fs_block_t;

typedef struct recipe {
    int num_blocks;
    fs_block_t blocks[MAX_BLOCKS];
} recipe_t;

typedef struct file {
    const char* filename; //idk why this is a const
    int stripe_width;
    struct pfs_stat* stat;
    recipe_t* recipe;
    token_node_t* read_tokens;
    token_node_t* write_tokens;
    int last_write; 
    int last_read;
    int current_server;
    int current_stripe; 
    char mode;
} file_t;

typedef struct server {
    char hostname[255];
    int port;
} server_t;

int connect_socket(char *host, int port);
server_t* get_server(int server_id);

server_t *servers;
