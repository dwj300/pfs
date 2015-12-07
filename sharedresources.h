#include <stdio.h>
#include <stdint.h>

#define true 1
#define false 0
#define BLOCK_IS_FREE 0
#define MAX_BLOCKS 100

typedef int bool;
typedef uint32_t global_block_id_t;

typedef struct fs_block {
    int server_id;
    int block_id;
} fs_block_t;

typedef struct recipe {
    int num_blocks;
    fs_block_t blocks[MAX_BLOCKS];
} recipe_t;

typedef struct file {
    char* filename;
    struct pfs_stat* stat;
    recipe_t* recipe;
} file_t;

typedef struct server {
    char hostname[255];
    int port;
} server_t;

