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
    file_desc_t partOf;
    bool dirty;
    offset_t offset;
    void *data;
} block_t;

typedef struct blockListNode
{
    block_t *block;
    struct blockListNode *next;
} block_list_node_t;



//Cache instance struct
typedef struct cache
{
    block_list_node_t *FreeList;
    block_list_node_t *DirtyList;
    Hashmap * ActiveBlocks;
    struct blockListNode *next;
} cache_t;




//Cache access methods

cache * InitializeCache();
block_t * GetBlock(cache * cache, global_block_id_t targetBlock);
bool ReleaseBlock(global_block_id_t targetBlock);
bool MarkBlockDirty(global_block_id_t targetBlock);
bool BlockIsDirty(global_block_id_t targetBlock);
