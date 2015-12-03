#include "cache.h"
#define BLOCK_TABLE_BUCKET_COUNT 1024

cache_t* InitializeCache() {
    cache_t* newCache = (cache_t*)malloc(sizeof(cache_t));
    newCache->FreeList = initializeBlockListNode();
    newCache->DirtyList = initializeBlockListNode();

    //Hashmap* Hashmap_create(Hashmap_compare compare, Hashmap_hash);
    newCache->ActiveBlockMap = Hashmap_create(&compareBlocks, &globalBlockIDHash);

    return NULL;
}

block_t* GetBlock(cache_t* cache, global_block_id_t targetBlock) {
    fprintf(stderr, "ERROR: Not implemented yet.");
    exit(1);
    return NULL;
}


activity_table_t * initializeActivityTable(){
    activity_table_t * newTable = (activity_table_t *)malloc(sizeof(activity_table_t));
    if(!newTable){
        fprintf(stderr, "Memory allocation for activity table failed.");
        return NULL;
    }

    newTable->accessQueue = initializeBlockList();
    newTable->freeStack = initializeBlockList();
    if(!(newTable->accessQueue) || !(newTable->freeStack)){
        fprintf(stderr, "List initialization during activity table initialization failed.");
        return NULL;
    }

    //Populate the cache with free blocks
    int i=0;
    for(i=0;i<CACHE_BLOCK_COUNT;i++){
        block_list_node_t* newBlockNode = initializeBlockListNode();
        if(newBlockNode){
            if( !addNodeToHeadOfList(newTable->freeStack, newBlockNode)){
                fprintf(stderr, "Node could not be added to the free stack during activity table initialization.");
                return NULL;
            }
        }
        else{
            fprintf(stderr, "Node initialization during activity table initialization failed.");
            return NULL;
        }
    }

    //Create the empty map
    for(i=0;i<CACHE_MAP_SECTOR_COUNT;i++){
            newTable->activeBlockMap[i] = initializeBlockList();
    }
}


uint32_t lookupMapping(global_block_id_t blockID){
    return blockID%BLOCK_TABLE_BUCKET_COUNT;
}





bool evictFromReferenceQueue(activity_table_t * hostTable)
{
    if(!hostTable) {
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
    return true;
}


bool removeFromMap(block_list_node_t ** map, global_block_id_t keyOfTarget){
    if(!map){
        fprintf(stderr, "You cannot remove an entry from a NULL map.");
        return false;
    }
    return removeFromList(key_t, );
}


bool addToMap(block_list_node_t ** map, block_list_node_t * toAdd){
    if(!map){
        fprintf(stderr, "You cannot remove an entry from a NULL map.");
        return false;
    }
    block_list_node_t * targetList = map[lookupMapping(toAdd->block->id)];
    return addToList(toAdd, targetList)


}

void addBlockNodeToBlockList(block_list_node_t* blockNodeToAdd, block_list_node_t* headOfTargetList){
    if(!headOfTargetList) {
        fprintf(stderr, "Target block list doesn't exist, aborting block add");
        return;
    }
    if(!blockToAdd) {
        fprintf(stderr, "You can't add a NULL block to a list, aborting block add");
        return;
    }
    block_list_node_t* sentinel = headOfTargetList;
    while(sentinel->next) {
        sentinel = sentinel->next;
    }
    sentinel->next = newNode;
}







bool removeFromList(block_list_node * toRemove){
    if(!toRemove) {
        fprintf(stderr, "You cannot remove a NULL node.");
        return false;
    }
    if(toRemove->previous){
        toRemove->previous->next = toRemove->next;
    }
    if(toRemove->next){
        toRemove->next->previous = toRemove->previous;
    }

}


//each block can be in the reference queue OR the Free Stack
bool recordReference(aglobal_block_id_t referencedBlockID)
{

}

//AccessBlock(block_id)
//First, we check the cache for the block
    //We attempt to find the block in the reference queue
        //We look in the hashmap for a reference to the queue node
//If the block is NOT found:
   //get it from the server
   //create a new queue node to hold it
   //add a reference to the new queue node to the hashmap
//Push the queue node holding the block to the front of the reference queue




//Creates and sets up a cache block object
//  This should ONLY be called on cache initialization
block_t* initializeBlock() {
    block_t* newBlock = (block_t* )malloc(sizeof(block_t));
    if(newBlock) {
        newBlock->lock = (pthread_mutex_t* )malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newBlock->lock, NULL);
        newBlock->id = BLOCK_IS_FREE;
        newBlock->dirty = false;
		//newBlock->offset = 0;
        newBlock->data = NULL;
        return newBlock;
    }
    return NULL;
}

//Destroys (de-allocates) a cache block object
//  This should ONLY be called on cache destruction / closing the file system
//  Returns true if successful, false otherwise
//      -Returns false if someone is holding the lock on the block
//                     OR if the block is dirty
bool destroyBlock(block_t* toDeallocate) {
    pthread_mutex_lock(toDeallocate->lock);

    free(toDeallocate->lock);

    block_t* newBlock = toDeallocate; // ????
    if(newBlock) {
        newBlock->lock = (pthread_mutex_t* )malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newBlock->lock, NULL);
        newBlock->id = BLOCK_IS_FREE;
        newBlock->dirty = false;
        newBlock->data = NULL;
        return true;
    }
    return false;
}

// bool resetBlock ???
//A 'clear' block is effectively empty and free for population
//  All blocks on the free list (and no other blocks) should be cleared
//  Note that this is only reliable if called by a thread with a lock on the block
bool blockIsCleared(block_t* block) {
    if(!block) {
        fprintf(stderr, "The block being checked does not exist.");
        return false;
    }
    return (block->dirty && (block->id == BLOCK_IS_FREE));
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

block_t* GetBlockFromCache(cache_t* cache, global_block_id_t targetBlock) {
    if(!cache || !cache->ActiveBlockMap) {
        fprintf(stderr, "The cache object has no been properly initialized");
    }
    //Look into active map and attempt to find target block
    //void* Hashmap_get(Hashmap* map, void* key);
    return (block_t* )Hashmap_get(cache->ActiveBlockMap, &targetBlock);
}

block_t* GetFreeBlockFromCache(cache_t* cache, global_block_id_t targetBlock) {
    if(!cache || !cache->FreeList){
        fprintf(stderr, "The cache object has no been properly initialized");
    }
    //Attempt to get a block from the free list

    //void* Hashmap_get(Hashmap* map, void* key);
    return (block_t* )Hashmap_get(cache->ActiveBlockMap, &targetBlock);
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
