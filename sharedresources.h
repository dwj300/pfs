#include <stdio.h>
#include <stdint.h>
// #include "itoa.h"

#define true 1
#define false 0
#define BLOCK_IS_FREE -1

typedef int bool;
typedef uint32_t global_block_id_t;

typedef struct file {
    char* filename;
} file_t;
