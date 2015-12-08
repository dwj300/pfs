#include "cache.h"

bool CacheIsTooCrowded(cache_t * cache){
    return (cache->Occupancy > cache->HighWaterMark);
}

bool CacheIsTooVacant(cache_t * cache){
    return (cache->Occupancy < cache->LowWaterMark);

}

//Should only be called if a lock on the block is held by the caller
//Should only return true if successful and if toPush->dirty has been set to false
bool pushBlockToServer(block_t* toPush) {
    //fprintf(stderr, "Pushing block to server\n");
    //fprintf(stderr, "ERROR: Not implemented yet\n");
    toPush->dirty = false;
    //Check if block is the last in the file -> update stat
    return true; //for debugging
}


activity_table_t * initializeActivityTable(byte * rawMemoryToManage, uint32_t blockCount, uint32_t blockSize){
    activity_table_t * newTable = (activity_table_t *)malloc(sizeof(activity_table_t));
    if(!newTable){
        fprintf(stderr, "Memory allocation for activity table failed.\n");
        return NULL;
    }
    newTable->AccessQueue = initializeBlockList();
    newTable->FreeStack = initializeBlockList();
    if(!(newTable->AccessQueue) || !(newTable->FreeStack)){
        return NULL;
    }

    newTable->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    //Populate the cache with free blocks
    uint32_t i=0;
    for(i=0;i<blockCount;i++){
        block_t * newBlock = initializeBlock( rawMemoryToManage + (i*(blockSize)) );
        block_list_node_t * newBlockNode = initializeBlockListNode(newBlock);
        if(newBlockNode){
            if( !addNodeToHeadOfList(newTable->FreeStack, newBlockNode)){
                fprintf(stderr, "Node could not be added to the free stack during activity table initialization.\n");
                return NULL;
            }
        }
        else{
            return NULL;
        }
    }

    newTable->ActiveBlockMap = (block_mapping_node_t **)malloc(sizeof(block_mapping_node_t *) * CACHE_MAP_SECTOR_COUNT);
    for(i=0;i<CACHE_MAP_SECTOR_COUNT;i++){
            newTable->ActiveBlockMap[i] = NULL;
    }
    return newTable;
}


cache_t* InitializeCache(uint32_t blockSize, uint32_t blockCount, float highWaterMarkPercent, float lowWaterMarkPercent) {
    if( highWaterMarkPercent > 100.0F || highWaterMarkPercent < 0.0F || lowWaterMarkPercent > 100.0F || lowWaterMarkPercent < 0.0F || highWaterMarkPercent < lowWaterMarkPercent){
        fprintf(stderr, "Malformated or illogical cache cleanup parameters (for low and/or high water mark).\n");
        return NULL;
    }

    cache_t * newCache = (cache_t*)malloc(sizeof(cache_t));
    newCache->ManagedMemory = (byte *)malloc(blockSize * blockCount);


    newCache->BlockCount = blockCount;
    newCache->BlockSize = blockSize;

    newCache->HighWaterMark = (uint32_t)((highWaterMarkPercent/100)*blockCount);
    newCache->LowWaterMark = (uint32_t)((lowWaterMarkPercent/100)*blockCount);
    newCache->BlockSize = blockSize;

    newCache->DirtyList = NULL;
    newCache->DirtyListLock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    newCache->ActivityTable = initializeActivityTable(newCache->ManagedMemory, blockCount, blockSize);
    return newCache;
}


uint32_t lookupMapping(global_block_id_t blockID){
    return blockID%CACHE_MAP_SECTOR_COUNT;
}


block_list_node_t * findBlockNodeInAccessQueue(activity_table_t * activityTable, global_block_id_t IDOfTargetBlock){
    block_mapping_node_t * hostListSentinel = activityTable->ActiveBlockMap[lookupMapping(IDOfTargetBlock)];

    while(hostListSentinel){
        if(hostListSentinel->mappedBlockNode->block->id == IDOfTargetBlock){
            return hostListSentinel->mappedBlockNode;
        }
        hostListSentinel = hostListSentinel->next;
    }
    return NULL;
}


//Should only be called for active blocks (just added the access queue)
bool addBlockMapping(activity_table_t * hostTable, block_list_node_t * toAdd){
    if(!hostTable) {
        fprintf(stderr, "The hosting activity table has no been properly initialized.\n");
        return false;
    }
    if(!toAdd || (toAdd->block->id == BLOCK_IS_FREE)) {
        fprintf(stderr, "You must fully initialize and assign a block to a block node before creating a mapping for it.\n");
        return false;
    }

    block_mapping_node_t * newNode = initializeMappingNode(toAdd);
    if(!newNode){
        return false; //initializer outputs an err msg already
    }

    uint32_t targetSector = lookupMapping(toAdd->block->id);
    block_mapping_node_t * sentinel = hostTable->ActiveBlockMap[targetSector];

    if(!sentinel){
        hostTable->ActiveBlockMap[targetSector] = newNode;
        return true;
    }
    else{
        //Adding to a list / sector
        while(sentinel){
            if( !(sentinel->next) ){
                sentinel->next = newNode;
                return true;
            }
            sentinel = sentinel->next;
        }
    }
    return false;
}


//Should only be called for freed blocks (just removed the access queue)
bool removeBlockMapping(activity_table_t * hostTable, block_list_node_t * toRemove){
    if(!hostTable) {
        fprintf(stderr, "The hosting activity table has no been properly initialized\n");
        return false;
    }
    if(!toRemove) {
        fprintf(stderr, "You cannot remove a mapping for a NULL block.\n");
        return false;
    }

    uint32_t targetSector = lookupMapping(toRemove->block->id);
    block_mapping_node_t * sentinel = hostTable->ActiveBlockMap[targetSector];
    block_mapping_node_t * last = NULL;


    while(sentinel){
        if(sentinel->mappedBlockNode == toRemove){
            if(last){
                last->next = sentinel->next;
            }
            else{
                hostTable->ActiveBlockMap[targetSector] = sentinel->next;
            }
            if(!destroyMappingNode(sentinel)){
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


block_t * GetBlock(cache_t* cache, global_block_id_t targetBlock) {
    fprintf(stderr, "ERROR: Not implemented yet.\n");
    exit(1);
    return NULL;
}


bool UnlockBlock(cache_t* cache, global_block_id_t targetBlock){
    block_list_node_t * hostingNode = findBlockNodeInAccessQueue(cache->ActivityTable, targetBlock);
    if(!hostingNode) {
        return false;
    }
    else {
        block_t * toUnlock = hostingNode->block;
        pthread_mutex_unlock(toUnlock->lock);
    }
    return true;
}


block_t * GetBlockFromCache(cache_t* cache, global_block_id_t targetBlock) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
    }

    pthread_mutex_lock(cache->ActivityTable->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t * hostingCacheNode = findBlockNodeInAccessQueue(cache->ActivityTable, targetBlock);
    if( hostingCacheNode && removeNodeFromList(cache->ActivityTable->AccessQueue, hostingCacheNode) ){
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


bool EvictBlockToCache(cache_t* cache){
    if(!cache) {
        fprintf(stderr, "You cannot evict within a NULL cache.");
        return false;
    }

    pthread_mutex_lock(cache->ActivityTable->lock);
    //Remove the node from the access queue and the activity table
    block_list_node_t* toEvict = removeNodeFromTailOfList(cache->ActivityTable->AccessQueue);
    if(!toEvict || !removeBlockMapping(cache->ActivityTable, toEvict) ){
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }
    pthread_mutex_unlock(cache->ActivityTable->lock);

    //Clear the block / write back if needed
    block_t * toRelease = toEvict->block;
    pthread_mutex_lock(toRelease->lock);
    if( (toRelease->dirty) ){
        if( !pushBlockToServer(toRelease) ){
            pthread_mutex_unlock(toRelease->lock);
            return false;
        }
        else{
            pthread_mutex_lock(cache->DirtyListLock);
            if( !removeIDFromIDList(&(cache->DirtyList), toRelease->id) ){
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
    if(!addNodeToHeadOfList(cache->ActivityTable->FreeStack, toEvict)){
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return false;
    }
    cache->Occupancy--;
    pthread_mutex_unlock(cache->ActivityTable->lock);

    return true;
}


block_t * ReserveBlockInCache(cache_t* cache, global_block_id_t blockToReserveFor) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
    }

    pthread_mutex_lock(cache->ActivityTable->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t * freeNode = removeNodeFromHeadOfList(cache->ActivityTable->FreeStack);

    if( freeNode ){
        freeNode->block->id = blockToReserveFor;

        //The block needs to be added to the access queue and recorded in the active block map
        addBlockMapping(cache->ActivityTable, freeNode);
        addNodeToHeadOfList(cache->ActivityTable->AccessQueue, freeNode);
        cache->Occupancy++;
        pthread_mutex_unlock(cache->ActivityTable->lock);
        return freeNode->block;
    }
    else{
        pthread_mutex_unlock(cache->ActivityTable->lock);
        if(EvictBlockToCache(cache)){
            return ReserveBlockInCache(cache, blockToReserveFor);
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

    //Look into activity table and find the block node holding the block to be released
    block_list_node_t * hostingNode = findBlockNodeInAccessQueue(cache->ActivityTable, blockToRelease);
    if( !hostingNode ){
        fprintf(stderr, "The block is not in the access queue.\n");
        return false;
    }
    block_t * toRelease = hostingNode->block;

    //Clear the block / write back if needed
    pthread_mutex_lock(toRelease->lock);
    if( (hostingNode->block->dirty) ){
        if( !pushBlockToServer(hostingNode->block) ){
            return false;
        }
        else{
            pthread_mutex_lock(cache->DirtyListLock);
            if( !removeIDFromIDList(&(cache->DirtyList), blockToRelease) ){
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
    if(!removeNodeFromList(cache->ActivityTable->AccessQueue, hostingNode) || !removeBlockMapping(cache->ActivityTable, hostingNode) ){
        return false;
    }

    //Add the node to the free stack
    if(!addNodeToHeadOfList(cache->ActivityTable->FreeStack, hostingNode)){
        return false;
    }
    cache->Occupancy--;


    pthread_mutex_unlock(cache->ActivityTable->lock);
    return true;
}


byte * ReadOrReserveBlockAndLock(cache_t* cache, global_block_id_t targetBlock, bool * present){
    block_t * blockInCache = GetBlockFromCache(cache, targetBlock);
    if(!blockInCache){
        *present = false;
        blockInCache = ReserveBlockInCache(cache, targetBlock);
    }
    else{
        *present = true;
    }

    if(blockInCache){
        pthread_mutex_lock(blockInCache->lock);
    }
    return blockInCache->data;
}



bool FetchBlockFromServer(cache_t* targetCache, global_block_id_t idOfBlockToFetch) {
    fprintf(stderr, "ERROR: Not implemented yet\n");
    exit(1);
    //Lookup in the recipe which server has the block
    return false;
}


//Reserves a space block if the block isn't in the cache already -> what if the block doesn't exist, is written to
bool WriteToBlockAndMarkDirty(cache_t * cache, global_block_id_t targetBlock, byte * toCopy, uint32_t startOffset, uint32_t endPosition ) {
    block_list_node_t * hostingNode = findBlockNodeInAccessQueue(cache->ActivityTable, targetBlock);
    if(!hostingNode) {
        //Must get the block from the server previous to writing to it
        if(!FetchBlockFromServer(cache, targetBlock)){
            return false;
        }
        else{
            return WriteToBlockAndMarkDirty(cache, targetBlock, toCopy, startOffset, endPosition);
        }
    }
    else {
        if(startOffset > cache->BlockSize || endPosition > cache->BlockSize || startOffset > endPosition){
            fprintf(stderr, "Write Offsets are illogical.\n");
            return false;
        }
        block_t * toWriteInto = hostingNode->block;
        pthread_mutex_lock(toWriteInto->lock);
        memcpy( ((void*)toWriteInto->data)+startOffset, (void *)toCopy, (endPosition - startOffset) );
        if(toWriteInto->dirty){
            //fprintf(stderr, "WARN: block was already dirty\n");
            pthread_mutex_unlock(toWriteInto->lock);
            return true;
        }
        toWriteInto->dirty = true;
        pthread_mutex_unlock(toWriteInto->lock);

        pthread_mutex_lock(cache->DirtyListLock);
        //Add to dirty list
        if( !addIDToIDList(&(cache->DirtyList), targetBlock) ){
            pthread_mutex_unlock(cache->DirtyListLock);
            return false;
        }
        else{
            pthread_mutex_unlock(cache->DirtyListLock);
        }
    }
    return true;
}


uint32_t findWidthUnsigned(uint32_t input){
    uint32_t width = 1;
    while(input/10 > 0){
        width++;
        input/=10;
    }
    return width;
}


char * itoaWrapUn(uint32_t input){
    char * buf = (char *)malloc( sizeof(char) * (1+findWidthUnsigned(input)) );
    sprintf(buf, "%u", input);
    // ultoa(buf, input);
    return buf;
}


void printUInt(uint32_t input){
    char * toPrint = itoaWrapUn(input);
    fprintf(stderr, "%s", toPrint);
    free(toPrint);
}


void CacheReport(cache_t * cache){
    fprintf(stderr, "Used blocks: \n");
    uint32_t i = 0;
    uint32_t used = 0;
    for(; i<CACHE_MAP_SECTOR_COUNT; i++){
        block_mapping_node_t * sentinel = cache->ActivityTable->ActiveBlockMap[i];
        while(sentinel){
            used++;
            printUInt(sentinel->mappedBlockNode->block->id);
            if(sentinel->mappedBlockNode->block->dirty)
                fprintf(stderr, "*");
            fprintf(stderr, ", ");
            sentinel = sentinel->next;
        }
    }
    fprintf(stderr, "\n");
    printUInt(used);
    fprintf(stderr, " total active blocks\n");

    if(used != cache->Occupancy)
        fprintf(stderr, "Cache is corrupted, the occupancy state variable is not consistent with the map of active blocks. \n");

    uint32_t free = 0;
    bool dirtyAndFree = false; // Ke$ha's next album title
    block_list_node_t * sentinel = cache->ActivityTable->FreeStack->head;
    while(sentinel){
        free++;
        if(sentinel->block->dirty){
            printUInt(sentinel->block->id);
            fprintf(stderr, "*, ");
            dirtyAndFree = true;
        }
        sentinel = sentinel->next;
    }
    if(dirtyAndFree){
        fprintf(stderr, "Cache is corrupted, a dirty block was found on the free list. \n");
    }
    if(free < (cache->BlockCount-cache->Occupancy) ){
        fprintf(stderr, "Cache is corrupted, there are less free blocks in the free stack than there are recorded as present in the cache.\n");
    }
    if(free > (cache->BlockCount-cache->Occupancy) ){
        fprintf(stderr, "Cache is corrupted, there are more free blocks in the free stack than there are recorded as present in the cache.\n");
    }
}


void * FlushDirtyBlocks(void * cache){
    fprintf(stderr, "Starting flushing. \n");
    int flushed = 0;
    cache_t * hostCache = (cache_t*)cache;

    pthread_mutex_lock(hostCache->DirtyListLock);
    global_block_id_t currentTarget = popIDFromIDList( &(hostCache->DirtyList) );
    pthread_mutex_unlock(hostCache->DirtyListLock);

    while(currentTarget != BLOCK_IS_FREE){
        block_list_node_t * hostingNode = findBlockNodeInAccessQueue(hostCache->ActivityTable, currentTarget);

        if(!hostingNode){
            fprintf(stderr, "WARN: Block referenced by the dirty list was not found to be active in the cache. Cache is likely corrupt. \n");
        }
        else{
            block_t * blockToFlush = hostingNode->block;
            pthread_mutex_lock(blockToFlush->lock);
            if( !blockToFlush->dirty ){
                fprintf(stderr, "WARN: Block referenced by the dirty list was found to be clean in the cache. Cache is likely corrupt. \n");
                pthread_mutex_unlock(blockToFlush->lock);
            }
            else if(!pushBlockToServer(hostingNode->block)){
                //if a push fails, might as well allow a retry later
                pthread_mutex_lock(hostCache->DirtyListLock);
                addIDToIDList( &(hostCache->DirtyList), currentTarget);
                pthread_mutex_unlock(hostCache->DirtyListLock);
                pthread_mutex_unlock(blockToFlush->lock);
            }
            else{
                flushed++;
                pthread_mutex_unlock(blockToFlush->lock);
            }
        }
        pthread_mutex_lock(hostCache->DirtyListLock);
        currentTarget = popIDFromIDList( &(hostCache->DirtyList) );
        pthread_mutex_unlock(hostCache->DirtyListLock);
    }
    printUInt(flushed);
    fprintf(stderr, " blocks flushed.\n");
    pthread_exit(0);
}


void * Harvest(void * cache){
    fprintf(stderr, "Done harvesting. \n");
    int harvested = 0;

    while( CacheIsTooCrowded((cache_t*)cache) ){
        if( EvictBlockToCache((cache_t*)cache) ){
            harvested ++;
        }
    }
    printUInt(harvested);
    fprintf(stderr, " blocks harvested. \n");
    pthread_exit(0);
}


void SpawnHarvester(cache_t * cache){
    pthread_t * hostThread = (pthread_t *)malloc(sizeof(pthread_t));
    //int harvester =
    pthread_create(hostThread, NULL, Harvest, (void*)cache);
    pthread_join(*hostThread, malloc(4));
    fprintf(stderr, "Done harvesting. \n");
}


void SpawnFlusher(cache_t * cache){
    pthread_t * hostThread = (pthread_t *)malloc(sizeof(pthread_t));
    pthread_create(hostThread, NULL, FlushDirtyBlocks, (void*)cache); //int flusher = ?
    pthread_join(*hostThread, malloc(4));
    fprintf(stderr, "Done flushing. \n");
}


void test_cache(int argc, char* argv[]) {

    //Smoke tests

    //Batched reservations / release / evict
    uint32_t blockCount = 4096;
    uint32_t blockSize = 1024;
    float highWaterMark = 20.0F;
    float lowWaterMark = 5.0F;
    cache_t * cache = InitializeCache(blockSize, blockCount, highWaterMark, lowWaterMark);
    CacheReport(cache);

    uint32_t i = 1;
    for(;i<=blockCount;i++){
        ReserveBlockInCache(cache, i);
        byte* data = malloc(blockSize);
        WriteToBlockAndMarkDirty(cache, i, data, 0, 1024);
    }
    CacheReport(cache);
    SpawnHarvester(cache);
    CacheReport(cache);
    SpawnFlusher(cache);
    CacheReport(cache);

/*


    uint32_t i = 1;
    for(;i<=blockCount;i++){
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount;i++){
        ReleaseBlockToCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount;i++){
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount;i++){
        EvictBlockToCache(cache);
    }
    CacheReport(cache);


    //Interlaced reservations / release / evict
    i = 1;
    for(;i<=blockCount;i++){
        ReserveBlockInCache(cache, i);
        ReleaseBlockToCache(cache, i);
        ReserveBlockInCache(cache, i);
        EvictBlockToCache(cache);
    }


    i = 1;
    for(;i<=blockCount/4;i++){
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount/8;i++){
        ReleaseBlockToCache(cache, i);
    }
    CacheReport(cache);
    for(;i<=blockCount/4;i++){
        MarkBlockDirty(cache, i);
    }



    i = 1;
    for(;i<=blockCount/4;i++){
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);
    i = 1;
    for(;i<=blockCount/8;i++){
        ReleaseBlockToCache(cache, 2*i);
    }
    CacheReport(cache);
    for(;i<=blockCount/4;i++){
        EvictBlockToCache(cache);
    }
    CacheReport(cache);



    i = 1;
    for(;i<=blockCount/4;i++){
        ReserveBlockInCache(cache, i);
    }
    CacheReport(cache);

    i = 1;
    for(;i<=blockCount/4;i++){
        MarkBlockDirty(cache->ActivityTable->, i);
    }
    CacheReport(cache);

    i = 1;
    for(;i<=blockCount/8;i++){
        ReleaseBlockToCache(cache, i);
    }
    CacheReport(cache);
    for(;i<=blockCount/4;i++){
        EvictBlockToCache(cache);
    }
*/
}



