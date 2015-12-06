#include "cache.h"
#define CACHE_MAP_SECTOR_COUNT 1024

bool CacheIsTooCrowded(cache_t * cache){
    return (cache->Occupancy > cache->HighWaterMark);
}

bool CacheIsTooVacant(cache_t * cache){
    return (cache->Occupancy < cache->LowWaterMark);

}


activity_table_t * initializeActivityTable(byte * rawMemoryToManage, uint32_t blockCount, uint32_t blockSize){
    activity_table_t * newTable = (activity_table_t *)malloc(sizeof(activity_table_t));
    if(!newTable){
        fprintf(stderr, "Memory allocation for activity table failed.");
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
                fprintf(stderr, "Node could not be added to the free stack during activity table initialization.");
                return NULL;
            }
        }
        else{
            return NULL;
        }
    }

    //Create the empty map
    for(i=0;i<CACHE_MAP_SECTOR_COUNT;i++){
            newTable->ActiveBlockMap[i] = NULL;
    }
    return newTable;
}


cache_t* InitializeCache(uint32_t blockSize, uint32_t blockCount, float highWaterMarkPercent, float lowWaterMarkPercent) {
    if( highWaterMarkPercent > 100.0F || highWaterMarkPercent < 0.0F || lowWaterMarkPercent > 100.0F || lowWaterMarkPercent < 0.0F || highWaterMarkPercent < lowWaterMarkPercent){
        fprintf(stderr, "Malformated or illogical cache cleanup parameters (for low and/or high water mark).");
    }

    cache_t * newCache = (cache_t*)malloc(sizeof(cache_t));
    newCache->ManagedMemory = (byte *)malloc(blockSize * blockCount);

    newCache->BlockCount = blockCount;
    newCache->BlockSize = blockSize;

    newCache->HighWaterMark = (uint32_t)(highWaterMarkPercent*blockCount);
    newCache->LowWaterMark = (uint32_t)(lowWaterMarkPercent*blockCount);
    newCache->BlockSize = blockSize;

    newCache->DirtyList = initializeBlockList();
    newCache->ActivityTable = initializeActivityTable(newCache->ManagedMemory, blockCount, blockSize);

    return NULL;
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
        fprintf(stderr, "The hosting activity table has no been properly initialized");
        return false;
    }
    if(!toAdd ||!(toAdd->block->id == BLOCK_IS_FREE)) {
        fprintf(stderr, "You must fully initialize and assign a block to a block node before creating a mapping for it.");
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
        fprintf(stderr, "The hosting activity table has no been properly initialized");
        return false;
    }
    if(!toRemove ||!(toRemove->block->id != BLOCK_IS_FREE)) {
        fprintf(stderr, "You cannot remove a mapping is fully initialize and assign a block to a block node before creating a mapping for it.");
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
            if(!destroyMappingNode(sentinel)){
                return false; //destructor will print err msg
            }
            return true;
        }
        sentinel = sentinel->next;
    }
    fprintf(stderr, "Block node was not mapped in the table.");
    return false;
}






block_t * GetBlock(cache_t* cache, global_block_id_t targetBlock) {
    fprintf(stderr, "ERROR: Not implemented yet.");
    exit(1);
    return NULL;
}


block_t * ReadBlockFromCache(cache_t* cache, global_block_id_t targetBlock) {
    if(!cache || !(cache->ActivityTable)) {
        fprintf(stderr, "The cache object has no been properly initialized");
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
        fprintf(stderr, "The cache object has no been properly initialized");
    }

    pthread_mutex_lock(cache->lock);

    //Look into activity table and attempt to find target block
    block_list_node_t * freeNode = removeNodeFromHeadOfList(cache->ActivityTable->FreeStack);

    if( freeNode ){
        freeNode->block->id = blockToReserveFor;

        //The block needs to be added to the access queue and recorded in the active block map
        addBlockMapping(cache->ActivityTable, freeNode);
        addNodeToHeadOfList(cache->ActivityTable->AccessQueue, freeNode);
        pthread_mutex_unlock(cache->lock);
        return freeNode->block;
    }
    else{
        pthread_mutex_unlock(cache->lock);
        return NULL;
    }
}




bool evictFromReferenceQueue(activity_table_t * hostTable){
/*    if(!hostTable) {
        fprintf(stderr, "You cannot evict within a NULL table.");
        return false;
    }
    if(!(hostTable->headOfReferenceQueue) || !(hostTable->tailOfReferenceQueue)) {
        fprintf(stderr, "You cannot evict from a table with no active blocks.");
        return false;
    }

    block_list_node_t* toEvict = hostTable->tailOfReferenceQueue;
    toEvict->previous = NULL;
    toEvict->next = NULL;

    //If the node was the only one in the reference queue, the ref. q. is now empty
    if(hostTable->headOfReferenceQueue == hostTable->tailOfReferenceQueue){
        hostTable->headOfReferenceQueue = hostTable->tailOfReferenceQueue = NULL;
    }
    else{
        //Otherwise, the ref. q. just has a new tail
        hostTable->tailOfReferenceQueue = hostTable->tailOfReferenceQueue->previous;
        hostTable->tailOfReferenceQueue->next = NULL;
    }

    //If the free stack is empty, the evicted block will be the only member of the free stack
    if(!(hostTable->headOfFreeStack)){
        hostTable->headOfFreeStack = toEvict;
    }
    else{
        //Otherwise we must reconnect the old free stack to the new one
        toEvict->next = headOfFreeStack;
        hostTable->headOfFreeStack->previous = toEvict
        hostTable->headOfFreeStack = toEvict;
    }

    //Remove the reference from the map into the queue
    removeFromMap(activity_table_t * )
*/
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


bool pushBlockToServer(block_t* toPush) {
    fprintf(stderr, "Pushing block to server");
    fprintf(stderr, "ERROR: Not implemented yet");
    exit(1);
    return false;
}


//Blocks represent cache blocks, which are commonly 'cleared':
//   -If the block is dirty, it's data is written back to the server
//   -The memory containing the block's data is deallocated
//   -The block's status information is reset
bool clearBlock(cache_t* cache, block_t* toClear) {
    if(!toClear) {
        fprintf(stderr, "You can not clear a null block");
        return false;
    }
    pthread_mutex_lock(toClear->lock);
    if(toClear->dirty) {
        if(!pushBlockToServer(toClear)) {
            return false;
        }
        toClear->dirty = false;
    }
    free(toClear->data);
    toClear->id = BLOCK_IS_FREE;
    pthread_mutex_unlock(toClear->lock);
    return true;
}



bool FetchBlockFromServer(cache_t* targetCache, global_block_id_t idOfBlockToFetch) {
    fprintf(stderr, "ERROR: Not implemented yet");
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
            fprintf(stderr, "block was already dirty");
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

int main(int argc, char* argv[]) {
    return 0;
}
