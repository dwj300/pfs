#include "cache.h"

bool CacheIsTooCrowded(cache_t* cache) {
    return (cache->Occupancy > cache->HighWaterMark);
}

bool CacheIsTooVacant(cache_t* cache) {
    return (cache->Occupancy < cache->LowWaterMark);
}

//Should only be called if a lock on the block is held by the caller
//Should only return true if successful and if toPush->dirty has been set to false
bool pushBlockToServer(block_t* toPush) {
    fprintf(stderr, "Pushing block to server\n");
    server_t* server = get_server(toPush->host);
    write_block(server->hostname, server->port, toPush->id, toPush->data); ///Might want to catch error from this for retrys? i.e. don't mark clean if write-back fails
    toPush->dirty = false;
    //Check if block is the last in the file -> update stat
    return true; //for debugging
}

activity_table_t* initializeActivityTable(byte* rawMemoryToManage, uint32_t blockCount, uint32_t blockSize) {
    activity_table_t* newTable = (activity_table_t*)malloc(sizeof(activity_table_t));
    if(!newTable) {
        fprintf(stderr, "Memory allocation for activity table failed.\n");
        return NULL;
    }
    newTable->AccessQueue = initializeBlockList();
    newTable->FreeStack = initializeBlockList();
    if(!(newTable->AccessQueue) || !(newTable->FreeStack)) {
        return NULL;
    }

    newTable->lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

    //Populate the cache with free blocks
    uint32_t i=0;
    for(i=0; i < blockCount; i++) {
        block_t* newBlock = initializeBlock(rawMemoryToManage + (i * blockSize));
        block_list_node_t* newBlockNode = initializeBlockListNode(newBlock);
        if(newBlockNode) {
            if(!addNodeToHeadOfList(newTable->FreeStack, newBlockNode)) {
                fprintf(stderr, "Node could not be added to the free stack during activity table initialization.\n");
                return NULL;
            }
        }
        else {
            return NULL;
        }
    }

    newTable->ActiveBlockMap = (block_mapping_node_t**)malloc(sizeof(block_mapping_node_t*)* CACHE_MAP_SECTOR_COUNT);
    for(i=0;i<CACHE_MAP_SECTOR_COUNT;i++) {
            newTable->ActiveBlockMap[i] = NULL;
    }
    return newTable;
}

uint32_t lookupMapping(global_block_id_t blockID) {
    return blockID%CACHE_MAP_SECTOR_COUNT;
}

block_list_node_t* findBlockNodeInAccessQueue(activity_table_t* activityTable, global_block_id_t IDOfTargetBlock) {
    block_mapping_node_t* hostListSentinel = activityTable->ActiveBlockMap[lookupMapping(IDOfTargetBlock)];

    while(hostListSentinel) {
        if(hostListSentinel->mappedBlockNode->block->id == IDOfTargetBlock) {
            return hostListSentinel->mappedBlockNode;
        }
        hostListSentinel = hostListSentinel->next;
    }
    return NULL;
}

//Should only be called for active blocks (just added the access queue)
bool addBlockMapping(activity_table_t* hostTable, block_list_node_t* toAdd) {
    if(!hostTable) {
        fprintf(stderr, "The hosting activity table has no been properly initialized.\n");
        return false;
    }
    if(!toAdd || (toAdd->block->id == BLOCK_IS_FREE)) {
        fprintf(stderr, "You must fully initialize and assign a block to a block node before creating a mapping for it.\n");
        return false;
    }

    block_mapping_node_t* newNode = initializeMappingNode(toAdd);
    if(!newNode) {
        return false; //initializer outputs an err msg already
    }

    uint32_t targetSector = lookupMapping(toAdd->block->id);
    block_mapping_node_t* sentinel = hostTable->ActiveBlockMap[targetSector];

    if(!sentinel) {
        hostTable->ActiveBlockMap[targetSector] = newNode;
        return true;
    }
    else{
        //Adding to a list / sector
        while(sentinel) {
            if( !(sentinel->next) ) {
                sentinel->next = newNode;
                return true;
            }
            sentinel = sentinel->next;
        }
    }
    return false;
}

//Should only be called for freed blocks (just removed the access queue)
bool removeBlockMapping(activity_table_t* hostTable, block_list_node_t* toRemove) {
    if(!hostTable) {
        fprintf(stderr, "The hosting activity table has no been properly initialized\n");
        return false;
    }
    if(!toRemove) {
        fprintf(stderr, "You cannot remove a mapping for a NULL block.\n");
        return false;
    }

    uint32_t targetSector = lookupMapping(toRemove->block->id);
    block_mapping_node_t* sentinel = hostTable->ActiveBlockMap[targetSector];
    block_mapping_node_t* last = NULL;


    while(sentinel) {
        if(sentinel->mappedBlockNode == toRemove) {
            if(last) {
                last->next = sentinel->next;
            }
            else{
                hostTable->ActiveBlockMap[targetSector] = sentinel->next;
            }
            if(!destroyMappingNode(sentinel)) {
                return false; //destructor will print err msg
            }
            return true;
        }
        sentinel = sentinel->next;
    }
    if(sentinel)
    fprintf(stderr, "Block node was not mapped in the table.\n");
    return false;
}

bool UnlockBlock(cache_t* cache, global_block_id_t targetBlock) {
    pthread_mutex_lock(cache->ActivityTable->lock);
    block_list_node_t* hostingNode = findBlockNodeInAccessQueue(cache->ActivityTable, targetBlock);
    if(!hostingNode) {
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }
    else {
        block_t* toUnlock = hostingNode->block;
        pthread_mutex_unlock(cache->ActivityTable->lock);

        pthread_mutex_unlock(toUnlock->lock);
    }
    return true;
}

block_t* GetBlockFromCache(cache_t* cache, global_block_id_t targetBlock) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
    }

    pthread_mutex_lock(cache->ActivityTable->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t* hostingCacheNode = findBlockNodeInAccessQueue(cache->ActivityTable, targetBlock);
    if( hostingCacheNode && removeNodeFromList(cache->ActivityTable->AccessQueue, hostingCacheNode) ) {
        //I the block is already active in the cache we need to note that the block was just referenced by
        //  moving it to the head of the access queue (from wherever in the access queue it currently is)
        addNodeToHeadOfList(cache->ActivityTable->AccessQueue, hostingCacheNode);
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return hostingCacheNode->block;
    }
    else{
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return NULL;
    }
}

bool EvictBlockToCache(cache_t* cache) {
    if(!cache) {
        fprintf(stderr, "You cannot evict within a NULL cache.");
        return false;
    }

    pthread_mutex_lock(cache->ActivityTable->lock);
    //Remove the node from the access queue and the activity table
    block_list_node_t* toEvict = removeNodeFromTailOfList(cache->ActivityTable->AccessQueue);
    if(!toEvict || !removeBlockMapping(cache->ActivityTable, toEvict) ) {
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }

    block_t* toRelease = toEvict->block;
    pthread_mutex_unlock(cache->ActivityTable->lock);

    //Clear the block / write back if needed
    pthread_mutex_lock(toRelease->lock);
    if( (toRelease->dirty) ) {
        if(!pushBlockToServer(toRelease)) {
            pthread_mutex_unlock(toRelease->lock);
            return false;
        }
        else{
            pthread_mutex_lock(cache->DirtyListLock);
            if( !removeIDFromIDList(&(cache->DirtyList), toRelease->id) ) {
                fprintf(stderr, "A dirty block was written back to the server, but it was not cleared from the dirty list.\n");
                pthread_mutex_unlock(cache->DirtyListLock);
            }
            else{
                pthread_mutex_unlock(cache->DirtyListLock);
            }
        }
    }
    toRelease->id = BLOCK_IS_FREE;
    pthread_mutex_unlock(toRelease->lock);

    pthread_mutex_lock(cache->ActivityTable->lock);
    //Add the node to the free stack
    if(!addNodeToHeadOfList(cache->ActivityTable->FreeStack, toEvict)) {
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }
    cache->Occupancy--;
    pthread_mutex_unlock(cache->ActivityTable->lock);

    return true;
}

block_t* ReserveAndLockBlockInCache(cache_t* cache, global_block_id_t blockToReserveFor) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
    }

    pthread_mutex_lock(cache->ActivityTable->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t* freeNode = removeNodeFromHeadOfList(cache->ActivityTable->FreeStack);

    if( freeNode ) {
        block_t * freeBlock = freeNode->block;
        pthread_mutex_lock(freeBlock->lock);
        freeBlock->id = blockToReserveFor;
        //The block needs to be added to the access queue and recorded in the active block map
        addBlockMapping(cache->ActivityTable, freeNode);
        addNodeToHeadOfList(cache->ActivityTable->AccessQueue, freeNode);
        cache->Occupancy++;
        if(CacheIsTooCrowded(cache)) {
            //sem_post(cache->OccupancyMonitor);
        }
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return freeNode->block;
    }
    else{
        pthread_mutex_unlock(cache->ActivityTable->lock);
        if(EvictBlockToCache(cache)) {
            return ReserveAndLockBlockInCache(cache, blockToReserveFor);
        }
        fprintf(stderr, "Couldn't reserve a block. There were no free blocks, and eviction failed.\n");
        return NULL;
    }
}

bool ReleaseBlockToCache(cache_t* cache, global_block_id_t blockToRelease) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
        return false;
    }

    pthread_mutex_lock(cache->ActivityTable->lock);

    //Look into activity table and find the block node holding the block to be released
    block_list_node_t* hostingNode = findBlockNodeInAccessQueue(cache->ActivityTable, blockToRelease);
    if( !hostingNode ) {
        pthread_mutex_unlock(cache->ActivityTable->lock);
        fprintf(stderr, "The block is not in the access queue.\n");
        return false;
    }
    block_t* toRelease = hostingNode->block;
    pthread_mutex_unlock(cache->ActivityTable->lock);

    //Clear the block / write back if needed
    pthread_mutex_lock(toRelease->lock);
    if( (hostingNode->block->dirty) ) {
        if( !pushBlockToServer(hostingNode->block) ) {
            pthread_mutex_unlock(toRelease->lock);
            return false;
        }
        else{
            pthread_mutex_lock(cache->DirtyListLock);
            if( !removeIDFromIDList(&(cache->DirtyList), blockToRelease) ) {
                fprintf(stderr, "A dirty block was written back to the server, but it was not cleared from the dirty list.\n");
                pthread_mutex_unlock(cache->DirtyListLock);
            }
            else{
                pthread_mutex_unlock(cache->DirtyListLock);
            }
        }
    }
    hostingNode->block->id = BLOCK_IS_FREE;
    pthread_mutex_unlock(toRelease->lock);


    pthread_mutex_lock(cache->ActivityTable->lock);

    //Remove the node from the access queue and the activity table
    if(!removeNodeFromList(cache->ActivityTable->AccessQueue, hostingNode) || !removeBlockMapping(cache->ActivityTable, hostingNode) ) {
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }

    //Add the node to the free stack
    if(!addNodeToHeadOfList(cache->ActivityTable->FreeStack, hostingNode)) {
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }
    cache->Occupancy--;


    pthread_mutex_unlock(cache->ActivityTable->lock);
    return true;
}

//byte* ReadOrReserveBlockAndLock(cache_t* cache, global_block_id_t targetBlock, bool* present) {
bool ReadBlockFromCache(cache_t* cache, global_block_id_t targetBlock, bool* present, off_t offset, ssize_t nbyte, int server_id, byte** buffer) {
    block_t* blockInCache = GetBlockFromCache(cache, targetBlock);
    if(!blockInCache) {
        *present = false;
        blockInCache = ReserveAndLockBlockInCache(cache, targetBlock);
        server_t* server = get_server(server_id);
        read_block(server->hostname, server->port, targetBlock, &(blockInCache->data));
    }
    else{
        *present = true;
    }

    memcpy(buffer, blockInCache->data+offset, nbyte);
    pthread_mutex_unlock(blockInCache->lock);

    //return blockInCache->data;
    return true;
}

bool FetchBlockFromServer(cache_t* cache, global_block_id_t idOfBlockToFetch, int server_id) {
    block_t* block = ReserveAndLockBlockInCache(cache, idOfBlockToFetch);
    block->host = server_id;
    block->dirty = false;

    server_t* server = get_server(server_id);
    int success = read_block(server->hostname, server->port, idOfBlockToFetch, &(block->data));
    if (!success) {
        pthread_mutex_unlock(block->lock);
        return false;
    }

    pthread_mutex_unlock(block->lock);
    return true;
}

//Reserves a space block if the block isn't in the cache already -> what if the block doesn't exist, is written to
bool WriteToBlockAndMarkDirty(cache_t* cache, global_block_id_t targetBlock, const byte* toCopy, uint32_t startOffset, uint32_t endPosition, int server_id, int* present) {
    block_t* toWriteInto = GetBlockFromCache(cache, targetBlock);
    if(!toWriteInto) {
        //Must get the block from the server previous to writing to it
        if(!FetchBlockFromServer(cache, targetBlock, server_id)) {
            return false;
        }
        else{
            *present = 0;
            return WriteToBlockAndMarkDirty(cache, targetBlock, toCopy, startOffset, endPosition, server_id, present);
        }
    }
    else {
        if(startOffset > cache->BlockSize || endPosition > cache->BlockSize || startOffset > endPosition) {
            fprintf(stderr, "Write Offsets are illogical.\n");
            return false;
        }
        pthread_mutex_lock(toWriteInto->lock);
        memcpy( ((void*)toWriteInto->data)+startOffset, (void*)toCopy, (endPosition - startOffset) );
        if(toWriteInto->dirty) {
            //fprintf(stderr, "WARN: block was already dirty\n");
            pthread_mutex_unlock(toWriteInto->lock);
            return true;
        }
        toWriteInto->dirty = true;
        //toWriteInto->host = server_id;
        pthread_mutex_unlock(toWriteInto->lock);

        pthread_mutex_lock(cache->DirtyListLock);
        //Add to dirty list
        if( !addIDToIDList(&(cache->DirtyList), targetBlock) ) {
            pthread_mutex_unlock(cache->DirtyListLock);
            return false;
        }
        else{
            pthread_mutex_unlock(cache->DirtyListLock);
        }
    }
    return true;
}

void CacheReport(cache_t* cache) {
    fprintf(stderr, "Used blocks: \n");
    uint32_t i = 0;
    uint32_t used = 0;
    for(; i<CACHE_MAP_SECTOR_COUNT; i++) {
        block_mapping_node_t* sentinel = cache->ActivityTable->ActiveBlockMap[i];
        while(sentinel) {
            used++;
            fprintf(stderr, "%u\n", sentinel->mappedBlockNode->block->id);
            if(sentinel->mappedBlockNode->block->dirty)
                fprintf(stderr, "*");
            fprintf(stderr, ", ");
            sentinel = sentinel->next;
        }
    }
    //fprintf(stderr, "\n");

    fprintf(stderr, "%u total active blocks\n", used);

    if(used != cache->Occupancy)
        fprintf(stderr, "Cache is corrupted, the occupancy state variable is not consistent with the map of active blocks. \n");

    uint32_t free = 0;
    bool dirtyAndFree = false; // Ke$ha's next album title
    block_list_node_t* sentinel = cache->ActivityTable->FreeStack->head;
    while(sentinel) {
        free++;
        if(sentinel->block->dirty) {
            fprintf(stderr, "%u*, ", sentinel->block->id);
            dirtyAndFree = true;
        }
        sentinel = sentinel->next;
    }
    if(dirtyAndFree) {
        fprintf(stderr, "Cache is corrupted, a dirty block was found on the free list. \n");
    }
    if(free < (cache->BlockCount-cache->Occupancy) ) {
        fprintf(stderr, "Cache is corrupted, there are less free blocks in the free stack than there are recorded as present in the cache.\n");
    }
    if(free > (cache->BlockCount-cache->Occupancy) ) {
        fprintf(stderr, "Cache is corrupted, there are more free blocks in the free stack than there are recorded as present in the cache.\n");
    }
}

// TODO:
void Harvest(cache_t* cache) {
    int harvested = 0;

    while(!CacheIsTooVacant(cache)) {
        if(EvictBlockToCache(cache)) {
            harvested += 1;
        }
    }
    fprintf(stderr, "%d blocks harvested.\n", harvested);
    //pthread_exit(0);
}

void* HarvesterThread(void *cache) {
    cache_t* hostCache = (cache_t*)cache;
    while(!hostCache->exiting){
        //sem_wait(hostCache->OccupancyMonitor);
        Harvest(hostCache);
    }
    return NULL;
}

// TODO:
void SpawnHarvester(cache_t* cache) {
    fprintf(stderr, "Spawning Harvester Thread\n");
    pthread_create(&threads[1], NULL, &HarvesterThread, (void*)cache);
}

//DOESN'T remove the id from the dirty list, since you might supply the id by doing so in the caller via a pop preceding the call to this function
bool FlushBlockToServer(cache_t* hostCache, global_block_id_t id, bool close) {
    pthread_mutex_lock(hostCache->ActivityTable->lock);
    block_list_node_t* hostingNode = findBlockNodeInAccessQueue(hostCache->ActivityTable, id);

    if (!hostingNode && close) {
        pthread_mutex_unlock(hostCache->ActivityTable->lock);
        return false;
    }

    if(!hostingNode) {
        pthread_mutex_unlock(hostCache->ActivityTable->lock);
        fprintf(stderr, "Block referenced was not found to be active in the cache. Cache is likely corrupt. \n");
        return false;
    }
    else{
        block_t* blockToFlush = hostingNode->block;
        pthread_mutex_unlock(hostCache->ActivityTable->lock);
        pthread_mutex_lock(blockToFlush->lock);
        if( !blockToFlush->dirty ) {
            fprintf(stderr, "WARN: Block referenced was found to be clean in the cache. Cache is likely corrupt. \n");
        }
        else if(!pushBlockToServer(hostingNode->block)) { ///If we could meaningfully check success here we could retry in a loop
            //if a push fails, might as well allow a retry later
            pthread_mutex_lock(hostCache->DirtyListLock);
            addIDToIDList( &(hostCache->DirtyList), id);
            pthread_mutex_unlock(hostCache->DirtyListLock);
        }
        pthread_mutex_unlock(blockToFlush->lock);
        return true;
    }
}

void FlushDirtyBlocks(cache_t* hostCache) {
    fprintf(stderr, "Starting flushing. \n");
    int flushed = 0;

    pthread_mutex_lock(hostCache->DirtyListLock);
    global_block_id_t currentTarget = popIDFromIDList(&(hostCache->DirtyList));
    pthread_mutex_unlock(hostCache->DirtyListLock);

    while(currentTarget != BLOCK_IS_FREE) {
        if( FlushBlockToServer(hostCache, currentTarget, false)){
            flushed++;
        }
        pthread_mutex_lock(hostCache->DirtyListLock);
        currentTarget = popIDFromIDList( &(hostCache->DirtyList) );
        pthread_mutex_unlock(hostCache->DirtyListLock);
    }
    fprintf(stderr, "%d blocks flushed.\n", flushed);
}

// TODO:
void* FlusherThread(void* cache) {
    cache_t* hostCache = (cache_t*)cache;
    while(!hostCache->exiting) {
        FlushDirtyBlocks(hostCache);
        sleep(30);
    }
    return NULL;
}

void SpawnFlusher(cache_t* cache) {
    fprintf(stderr, "Spawning Flusher Thread\n");
    pthread_create(&threads[0], NULL, &FlusherThread, (void*)cache); //FlushDirtyBlocks, (void*)cache); //int flusher = ?
}

cache_t* InitializeCache(uint32_t blockSize, uint32_t blockCount, float highWaterMarkPercent, float lowWaterMarkPercent) {
    if(highWaterMarkPercent > 100.0F || highWaterMarkPercent < 0.0F ||
        lowWaterMarkPercent > 100.0F || lowWaterMarkPercent < 0.0F  ||
        highWaterMarkPercent < lowWaterMarkPercent) {
        fprintf(stderr, "Malformated or illogical cache cleanup parameters (for low and/or high water mark).\n");
        return NULL;
    }

    cache_t* newCache = (cache_t*)malloc(sizeof(cache_t));
    newCache->exiting = false;
    newCache->ManagedMemory = (byte*)malloc(blockSize* blockCount);

    newCache->BlockCount = blockCount;
    newCache->BlockSize = blockSize;

    newCache->HighWaterMark = (uint32_t)((highWaterMarkPercent/100)*blockCount);
    newCache->LowWaterMark = (uint32_t)((lowWaterMarkPercent/100)*blockCount);
    newCache->Occupancy = 0;
    //newCache->OccupancyMonitor = (sem_t*)malloc(sizeof(sem_t));
    //sem_init(newCache->OccupancyMonitor, 0, 0);

    newCache->DirtyList = NULL;
    newCache->DirtyListLock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

    newCache->ActivityTable = initializeActivityTable(newCache->ManagedMemory, blockCount, blockSize);
    SpawnFlusher(newCache);
    //SpawnHarvester(newCache);

    return newCache;
}

void test_cache(int argc, char* argv[]) {
    //Smoke tests

    //Batched reservations / release / evict
    uint32_t blockCount = 4096;
    uint32_t blockSize = 1024;
    float highWaterMark = 20.0F;
    float lowWaterMark = 5.0F;
    cache_t* cache = InitializeCache(blockSize, blockCount, highWaterMark, lowWaterMark);
    CacheReport(cache);

    bool here;
    uint32_t i = 0;
    for(;i<cache->HighWaterMark;i++) {
        block_t * block = ReserveAndLockBlockInCache(cache, i);
        pthread_mutex_unlock(block->lock);
        WriteToBlockAndMarkDirty(cache, i, "test", 0, 5, 0, &here);
    }
    sleep(2);
    for(;i<cache->BlockCount;i++) {
        block_t * block = ReserveAndLockBlockInCache(cache, i);
        pthread_mutex_unlock(block->lock);
        WriteToBlockAndMarkDirty(cache, i, "test", 0, 5, 0, &here);
    }


/*


    uint32_t i = 1;
    for(;i<=blockCount;i++) {
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount;i++) {
        ReleaseBlockToCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount;i++) {
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount;i++) {
        EvictBlockToCache(cache);
    }
    CacheReport(cache);


    //Interlaced reservations / release / evict
    i = 1;
    for(;i<=blockCount;i++) {
        ReserveBlockInCache(cache, i);
        ReleaseBlockToCache(cache, i);
        ReserveBlockInCache(cache, i);
        EvictBlockToCache(cache);
    }


    i = 1;
    for(;i<=blockCount/4;i++) {
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount/8;i++) {
        ReleaseBlockToCache(cache, i);
    }
    CacheReport(cache);
    for(;i<=blockCount/4;i++) {
        MarkBlockDirty(cache, i);
    }



    i = 1;
    for(;i<=blockCount/4;i++) {
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount/8;i++) {
        ReleaseBlockToCache(cache, 2*i);
    }
    CacheReport(cache);
    for(;i<=blockCount/4;i++) {
        EvictBlockToCache(cache);
    }
    CacheReport(cache);



    i = 1;
    for(;i<=blockCount/4;i++) {
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);

    i = 1;
    for(;i<=blockCount/4;i++) {
        MarkBlockDirty(cache->ActivityTable->, i);
    }
    CacheReport(cache);

    i = 1;
    for(;i<=blockCount/8;i++) {
        ReleaseBlockToCache(cache, i);
    }
    CacheReport(cache);
    for(;i<=blockCount/4;i++) {
        EvictBlockToCache(cache);
    }
*/
}
