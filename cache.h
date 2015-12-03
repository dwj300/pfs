#include "blockStructs.h"
#include "sharedresources.h"

typedef int32_t file_desc_t;
typedef int32_t offset_t;
typedef char byte;

typedef struct activity_table {
    block_list_t * accessQueue;
    block_list_t * freeStack;
    block_list_t ** activeBlockMap;
} activity_table_t;


typedef struct cache {
    block_list_t * DirtyList;
    activity_table_t * ActivityTable;
} cache_t;




//Cache access methods

cache_t * InitializeCache();
block_t * GetBlock(cache_t* cache, global_block_id_t targetBlock);
bool ReleaseBlock(global_block_id_t targetBlock);
bool MarkBlockDirty(cache_t *cache, global_block_id_t targetBlock);
bool BlockIsDirty(cache_t *cache, global_block_id_t targetBlock);
