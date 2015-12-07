#include "cache.h"
#define CACHE_MAP_SECTOR_COUNT 1024

bool CacheIsTooCrowded(cache_t * cache){
    return (cache->Occupancy > cache->HighWaterMark);
}

bool CacheIsTooVacant(cache_t * cache){
    return (cache->Occupancy < cache->LowWaterMark);

}


bool pushBlockToServer(block_t* toPush) {
    fprintf(stderr, "Pushing block to server");
    fprintf(stderr, "ERROR: Not implemented yet\n");
    exit(1);
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
    newCache->lock = (pthread_mutex_t* )malloc(sizeof(pthread_mutex_t));
    newCache->ManagedMemory = (byte *)malloc(blockSize * blockCount);

    newCache->BlockCount = blockCount;
    newCache->BlockSize = blockSize;

    newCache->HighWaterMark = (uint32_t)((highWaterMarkPercent/100)*blockCount);
    newCache->LowWaterMark = (uint32_t)((lowWaterMarkPercent/100)*blockCount);
    newCache->BlockSize = blockSize;

    newCache->DirtyList = initializeBlockList();
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


block_t * ReadBlockFromCache(cache_t* cache, global_block_id_t targetBlock) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
    }

    pthread_mutex_lock(cache->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t * hostingCacheNode = findBlockNodeInAccessQueue(cache->ActivityTable, targetBlock);
    if( hostingCacheNode && removeNodeFromList(cache->ActivityTable->AccessQueue, hostingCacheNode) ){
        //I the block is already active in the cache we need to note that the block was just referenced by
        //  moving it to the head of the access queue (from wherever in the access queue it currently is)
        addNodeToHeadOfList(cache->ActivityTable->AccessQueue, hostingCacheNode);
        pthread_mutex_unlock(cache->lock);
        return hostingCacheNode->block;
    }
    else{
        pthread_mutex_unlock(cache->lock);
        return NULL;
    }
}


block_t * ReserveBlockInCache(cache_t* cache, global_block_id_t blockToReserveFor) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
    }

    pthread_mutex_lock(cache->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t * freeNode = removeNodeFromHeadOfList(cache->ActivityTable->FreeStack);

    if( freeNode ){
        freeNode->block->id = blockToReserveFor;

        //The block needs to be added to the access queue and recorded in the active block map
        addBlockMapping(cache->ActivityTable, freeNode);
        addNodeToHeadOfList(cache->ActivityTable->AccessQueue, freeNode);
        cache->Occupancy++;
        pthread_mutex_unlock(cache->lock);
        return freeNode->block;
    }
    else{
        pthread_mutex_unlock(cache->lock);
        return NULL;
    }
}


bool ReleaseBlockToCache(cache_t* cache, global_block_id_t blockToRelease) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized\n");
        return false;
    }

    pthread_mutex_lock(cache->lock);

    //Look into activity table and find the block node holding the block to be released
    block_list_node_t * hostingNode = findBlockNodeInAccessQueue(cache->ActivityTable, blockToRelease);
    if( !hostingNode ){
        fprintf(stderr, "The block is not in the access queue.\n");
        return false;
    }

    //Remove the node from the access queue and the activity table
    if(!removeNodeFromList(cache->ActivityTable->AccessQueue, hostingNode) || !removeBlockMapping(cache->ActivityTable, hostingNode) ){
        return false;
    }

    //Clear the block / write back if needed
    if( (hostingNode->block->dirty) ){
        if( !pushBlockToServer(hostingNode->block) || !removeNodeFromList(cache->DirtyList, hostingNode)){
            return false;
        }
    }
    hostingNode->block->id = BLOCK_IS_FREE;


    //Add the node to the free stack
    if(!addNodeToHeadOfList(cache->ActivityTable->FreeStack, hostingNode)){
        return false;
    }
    cache->Occupancy--;


    pthread_mutex_unlock(cache->lock);
    return true;
}


bool EvictBlockToCache(cache_t* cache){
    if(!cache) {
        fprintf(stderr, "You cannot evict within a NULL cache.");
        return false;
    }

    pthread_mutex_lock(cache->lock);

    //Remove the node from the access queue and the activity table
    block_list_node_t* toEvict = removeNodeFromTailOfList(cache->ActivityTable->AccessQueue);
    if(!toEvict || !removeBlockMapping(cache->ActivityTable, toEvict) ){
        return false;
    }

    //Clear the block / write back if needed
    if( (toEvict->block->dirty) ){
        if( !pushBlockToServer(toEvict->block) || !removeNodeFromList(cache->DirtyList, toEvict)){
            return false;
        }
    }
    toEvict->block->id = BLOCK_IS_FREE;


    //Add the node to the free stack
    if(!addNodeToHeadOfList(cache->ActivityTable->FreeStack, toEvict)){
        return false;
    }
    cache->Occupancy--;


    pthread_mutex_unlock(cache->lock);
    return true;
}







//Place the given data into the given block, with the specified global block id
//  Returns true on success, false otherwise:
//      Returns false if the block isn't clear
bool populateBlock(block_t* targetBlock, byte* data, global_block_id_t id) {
    if(!targetBlock) {
        fprintf(stderr, "You cannot populate a NULL block.");
        return false;
    }
    pthread_mutex_lock(targetBlock->lock);
    if(!blockIsCleared(targetBlock)) {
        fprintf(stderr, "You cannot populate a uncleared block");
        return false;
    }
    targetBlock->id = id;
    targetBlock->dirty = false;
    targetBlock->data = data;
    pthread_mutex_unlock(targetBlock->lock);
    return true;
}



//Blocks represent cache blocks, which are commonly 'cleared':
//   -If the block is dirty, it's data is written back to the server
//   -The memory containing the block's data is deallocated
//   -The block's status information is reset
bool clearBlock(block_t* toClear) {
    if(!toClear) {
        fprintf(stderr, "You can not clear a null block\n");
        return false;
    }
    pthread_mutex_lock(toClear->lock);
    if(toClear->dirty) {
        if(!pushBlockToServer(toClear)) {
            return false;
        }
        toClear->dirty = false;
    }
    //TODO clear/zero out memory maybe? more of a security thing, probably not necessary
    toClear->id = BLOCK_IS_FREE;
    pthread_mutex_unlock(toClear->lock);
    return true;
}



bool FetchBlockFromServer(cache_t* targetCache, global_block_id_t idOfBlockToFetch) {
    fprintf(stderr, "ERROR: Not implemented yet\n");
    exit(1);
    //Lookup in the recipe which server has the block
    return false;
}


bool MarkBlockDirty(cache_t* cache, global_block_id_t targetBlock) {
    block_t* toMark = GetBlock(cache, targetBlock);
    if(!toMark) {
        return false;
    }
    else {
        if(toMark->dirty)
            fprintf(stderr, "block was already dirty\n");
        //CONCERN: what if multiple lock contentions are served in non-fifo order?
        pthread_mutex_lock(toMark->lock);
        toMark->dirty = true;
        //Add to dirty list

        pthread_mutex_unlock(toMark->lock);
        return true;
    }
}


bool BlockIsDirty(cache_t* cache, global_block_id_t targetBlock) {
    block_t* toCheck = GetBlock(cache, targetBlock);
    if(!toCheck) {
        return false;
    }
    else {
        //CONCERN: what if multiple lock contentions are served in non-fifo order?
	    //CONCERN: what if dirty state changes after releasing the lock and returning?
        pthread_mutex_lock(toCheck->lock);
        // bool state = toCheck->dirty;
        pthread_mutex_unlock(toCheck->lock);
        return true;
    }
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
    ultoa(buf, input);
    return buf;
}


void printUInt(uint32_t input){
    char * toPrint = itoaWrapUn(input);
    fprintf(stderr, toPrint);
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
    while( CacheIsTooCrowded((cache_t*)cache) && EvictBlockToCache((cache_t*)cache) );
    pthread_exit(0);
}

void SpawnHarvester(){

}

void SpawnFlusher(cache_t * cache){
    pthread_t * hostThread = (pthread_t *)malloc(sizeof(pthread_t));
    int flusher = pthread_create(hostThread, NULL, FlushDirtyBlocks, (void*)cache);
    pthread_join(*hostThread, malloc(4));
}



int main(int argc, char* argv[]) {

    //Smoke tests

    //Batched reservations / release / evict
    uint32_t blockCount = 2048;
    uint32_t blockSize = 1024;
    float highWaterMark = 80.0F;
    float lowWaterMark = 20.0F;
    cache_t * cache = InitializeCache(blockSize, blockCount, highWaterMark, lowWaterMark);
    CacheReport(cache);

    ReserveBlockInCache(cache, 1);

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
        EvictBlockToCache(cache);
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

    */

    return 0;
}



