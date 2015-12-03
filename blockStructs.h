#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef int32_t file_desc_t;
typedef int32_t offset_t;
typedef char byte;

// General struct definitions
typedef struct block
{
    global_block_id_t id;
    pthread_mutex_t *lock;
    bool dirty;
    byte *data;
} block_t;

//Block Access Methods
block_t * initializeBlock();



typedef struct block_list_node {
    block_t * block;
    struct block_list_node * previous;
    struct block_list_node * next;
} block_list_node_t;

block_list_node_t * initializeBlockListNode();



typedef struct block_list {
    struct block_list_node * head;
    struct block_list_node * tail;
} block_list_t;

block_list_t * initializeBlockList();
