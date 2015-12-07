#include "blockStructs.h"

typedef int32_t file_desc_t;
typedef int32_t offset_t;
typedef char byte;

typedef struct activity_table {
    block_list_t * AccessQueue;
    block_list_t * FreeStack;
    block_mapping_node_t ** ActiveBlockMap;
} activity_table_t;


typedef struct cache {
    uint32_t BlockSize;
    uint32_t BlockCount;

    uint32_t Occupancy;
    uint32_t HighWaterMark;
    uint32_t LowWaterMark;

    block_list_t * DirtyList;
    activity_table_t * ActivityTable;
    pthread_mutex_t *lock;

    byte * ManagedMemory;
} cache_t;




//Cache access methods

cache_t * InitializeCache(uint32_t blockSize, uint32_t blockCount, float highWaterMarkPercent, float lowWaterMarkPercent);
block_t * GetBlock(cache_t* cache, global_block_id_t targetBlock);
bool ReleaseBlock(global_block_id_t targetBlock);
bool MarkBlockDirty(cache_t *cache, global_block_id_t targetBlock);
bool BlockIsDirty(cache_t *cache, global_block_id_t targetBlock);
