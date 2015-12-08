#include <stdio.h>
#include <stdint.h>

#define true 1
#define false 0
#define BLOCK_IS_FREE 0

typedef int bool;
typedef int32_t global_block_id_t;
typedef int32_t global_server_id_t;

typedef struct file {
    char* filename;
    struct pfs_stat* stat;
    //recipie_t* recipie;
} file_t;
