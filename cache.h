#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "sharedresources.h"


typedef int32_t file_desc_t;
typedef int32_t offset_t;

// General struct definitions
typedef struct blockListNode
{
    block_t *block;
    struct blockListNode *next;
} block_list_node_t;


typedef struct block
{
    global_block_id_t id;
    pthread_mutex_t *lock;
    bool dirty;
    offset_t offset;
    void *data;
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



//Cache instance struct
typedef struct cache
{
    block_list_node_t *FreeList;
    block_list_node_t *DirtyList;
    Hashmap * ActiveBlocks;
} cache_t;




//Cache access methods

cache * InitializeCache();
block_t * GetBlock(cache * cache, global_block_id_t targetBlock);
bool ReleaseBlock(global_block_id_t targetBlock);
bool MarkBlockDirty(global_block_id_t targetBlock);
bool BlockIsDirty(global_block_id_t targetBlock);
