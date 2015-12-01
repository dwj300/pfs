#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "hashmap.h"
#include "sharedresources.h"

typedef int32_t file_desc_t;
typedef int32_t offset_t;
typedef char byte;
typedef int (cond_t)();

// General struct definitions
typedef struct block
{
    global_block_id_t id;
    pthread_mutex_t *lock;
    bool dirty;
    byte *data;
} block_t;

typedef struct blockListNode
{
    block_t *block;
    struct blockListNode *next;
} block_list_node_t;

//Block Access Methods
block_t * initializeBlock();

//Block List Access Methods
block_list_node_t *initializeBlockListNode();
void addBlockToBlockList(block_t * blockToAdd, block_list_node_t * headOfTargetList);
void removeBlockFromBlockList(global_block_id_t idOfBlockToRemove, block_list_node_t * headOfHostList);

typedef struct block_queue_node {
    pthread_cond_t *cVar;
    cond_t *progress_condition;
    int priority;
    void *data;
    struct queue_node *next;
} block_queue_node_t;

typedef struct queue {
    block_queue_node_t *head;
} queue_t;

//Cache instance struct
typedef struct cache {
    block_list_node_t *FreeList;
    block_list_node_t *DirtyList;
    Hashmap * ActiveBlockLookupMap;
    Hashmap * ActiveBlockMap;
} cache_t;


//Cache access methods

cache_t * InitializeCache();
block_t * GetBlock(cache_t* cache, global_block_id_t targetBlock);
bool ReleaseBlock(global_block_id_t targetBlock);
bool MarkBlockDirty(cache_t *cache, global_block_id_t targetBlock);
bool BlockIsDirty(cache_t *cache, global_block_id_t targetBlock);