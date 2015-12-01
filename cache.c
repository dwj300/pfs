#include "cache.h"

//typedef int (*Hashmap_compare)(void* a, void* b);
Hashmap_compare compareBlocks(void* a, void* b) {
    return(*((global_block_id_t*)b) -* ((global_block_id_t*)a));
}

//typedef uint32_t (*Hashmap_hash)(void* key);
Hashmap_hash globalBlockIDHash(void* globalBlockID) {
    return* ((uint32_t* )globalBlockID); //global block ids are unique... so we should be able to use the identity function* as* our hash function
}

cache_t* InitializeCache() {
    cache_t* newCache = (cache_t*)malloc(sizeof(cache_t));
    newCache->FreeList = initializeBlockListNode();
    newCache->DirtyList = initializeBlockListNode();

    //Hashmap* Hashmap_create(Hashmap_compare compare, Hashmap_hash);
    newCache->ActiveBlockMap = Hashmap_create(&compareBlocks, &globalBlockIDHash);

    return NULL;
}

block_list_node_t* initializeBlockListNode() {
    block_list_node_t* newBlockListNode = (block_list_node_t* )malloc(sizeof(block_list_node_t));
    if(newBlockListNode) {
        newBlockListNode->block = initializeBlock();
        if(newBlockListNode->block) {
            newBlockListNode->next = NULL;
            return newBlockListNode;
        }
    }
    return NULL;
}

block_t* GetBlock(cache_t* cache, global_block_id_t targetBlock) {
    fprintf(stderr, "ERROR: Not implemented yet.");
    exit(1);
    return NULL;
}


//Destroying a block list node DOES NOT modify the block within it, it simplies frees the list node holding it
void destroyBlockListNode(block_list_node_t* toDestroy) {
    if(!toDestroy) {
        fprintf(stderr, "You can not destroy a null block list node");
    }
    free(toDestroy);
}

void addBlockToBlockList(block_t* blockToAdd, block_list_node_t* headOfTargetList) {
    block_list_node_t* newNode = malloc(sizeof(block_list_node_t));
    newNode->block = blockToAdd;
    newNode->next = NULL;

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

//  -Finds a given block in given block list
//  -Removes the node holding the block from the list
//      -Redefines the head of the list iff the block is in the head node
//  -Returns a reference to the block itself
//      -Returns NULL if the block isn't found in the list
block_t* extractBlockFromBlockList(global_block_id_t idOfBlockToRemove, block_list_node_t* * referenceToHostList) {
    if(!(*referenceToHostList)) {
        fprintf(stderr, "Host block list doesn't exist, aborting block removal");
        return NULL;
    }
    block_list_node_t* sentinel =* (referenceToHostList);
    block_list_node_t* last = NULL;
    while(sentinel) {
        if(sentinel->block->id == idOfBlockToRemove) {
            block_t* toReturn = sentinel->block;
            if(last) {
                //Reconnect
                last->next = sentinel->next;
            }
            else {
                //Redefine the head of the list
               * (referenceToHostList) = sentinel->next;
            }
            destroyBlockListNode(sentinel);
            return toReturn;
        }
        else {
            last = sentinel;
            sentinel = sentinel->next;
        }
    }
    fprintf(stderr, "Host block list didn't contain the specified block, aborting block removal");
    return NULL;
}


//  -Removes the node at the head of the given list
//      -Redefines the head of the list
//  -Returns a reference to the block that was in that node
//      -Returns NULL if the list is NULL
block_t* popBlockFromBlockList(block_list_node_t* * referenceToHostList)
{
    if(!(*referenceToHostList))
    {
        fprintf(stderr, "Host block list doesn't exist, aborting block pop");
        return NULL;
    }
    block_list_node_t* head =* referenceToHostList;
    block_t* toReturn = head->block;
    if(head->next)
    {
        //Redefine the head of the list
       *referenceToHostList = head->next;
    }
    destroyBlockListNode(head);
    return toReturn;
}

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
