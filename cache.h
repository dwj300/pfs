#include "blockStructs.h"
#define CACHE_MAP_SECTOR_COUNT 8

typedef int32_t file_desc_t;
typedef int32_t offset_t;

typedef struct activity_table {
    block_list_t * AccessQueue;
    block_list_t * FreeStack;
    block_mapping_node_t ** ActiveBlockMap;
    pthread_mutex_t *lock;
} activity_table_t;


typedef struct cache {
    uint32_t BlockSize;
    uint32_t BlockCount;

    uint32_t Occupancy;
    uint32_t HighWaterMark;
    uint32_t LowWaterMark;

    id_list_node_t * DirtyList;
    pthread_mutex_t * DirtyListLock;

    activity_table_t * ActivityTable;

    byte * ManagedMemory;
} cache_t;




//Cache access methods

cache_t* InitializeCache(uint32_t blockSize, uint32_t blockCount, float highWaterMarkPercent, float lowWaterMarkPercent);
block_t* GetBlock(cache_t* cache, global_block_id_t targetBlock);
bool ReleaseBlock(global_block_id_t targetBlock);
bool MarkBlockDirty(cache_t *cache, global_block_id_t targetBlock);
bool BlockIsDirty(cache_t *cache, global_block_id_t targetBlock);
void test_cache(int argc, char* argv[]);
byte* ReadOrReserveBlockAndLock(cache_t* cache, global_block_id_t targetBlock, bool* present);
bool WriteToBlockAndMarkDirty(cache_t* cache, global_block_id_t targetBlock, byte* toCopy, uint32_t startOffset, uint32_t endPosition);
bool UnlockBlock(cache_t* cache, global_block_id_t targetBlock);
