#include "cache.h"

//typedef int (*Hashmap_compare)(void *a, void *b);
Hashmap_compare * compareBlocks(void *a, void *b)
{
    return( *((global_block_id_t*)b) - *((global_block_id_t*)a));
}


//typedef uint32_t (*Hashmap_hash)(void *key);
static uint32_t globalBlockIDHash(void *globalBlockID)
{
    return *globalBlockID; //global block ids are unique... so we should be able to use the identity function *as* our hash function
}



cache_t * InitializeCache()
{
    cache * newCache = (cache *)malloc(sizeof(cache));
    newCache->FreeList = initializeBlockListNode();
    newCache->DirtyList = initializeBlockListNode();

    //Hashmap *Hashmap_create(Hashmap_compare compare, Hashmap_hash);
	newCache->ActiveBlocks = Hashmap_create(compareBlocks, globalBlockIDHash)


    return NULL;
}

block_list_node_t *initializeBlockListNode()
{
    block_list_node_t * newBlockListNode = (block_list_node_t *)malloc(sizeof(block_list_node_t));
    if(newBlockListNode)
    {    
        newBlockListNode->block = initializeBlock();
        if(newBlockListNode->block)
        {
            newBlockListNode->next = NULL;
            return newBlockListNode;       
        }
    }
    return NULL;
}

block_t * initializeBlock()
{
    block_t * newBlock = (block_t *)malloc(sizeof(block_t));
    if(newBlock)
    {    
        newBlock->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newBlock->lock, NULL);
        newBlock->partOf = BLOCK_DOES_NOT_EXIST;
        newBlock->dirty = false;
		newBlock->offset = 0;
        newBlock->data = NULL;
        return newBlock;
    }
    return NULL;  
}



block_t * GetBlock(cache * cache, global_block_id_t targetBlock)
{
    if(!cache || !cache->ActiveBlocks)
        print("The cache object has no been properly initialized"); 
    //Look into active map and attempt to find target block
    //void *Hashmap_get(Hashmap *map, void *key);  
    return (block_t *)Hashmap_get(cache->ActiveBlocks, &targetBlock);    
}


bool MarkBlockDirty(cache * cache, global_block_id_t targetBlock)
{
    block_t toMark = GetBlock(cache, targetBlock);
    if(!toMark)
        return false;
    else
    { //CONCERN: what if multiple lock contentions are served in non-fifo order?
        pthread_mutex_lock(toMark->lock);
        toMark->dirty = true;
        pthread_mutex_unlock(toMark->lock);
    }
}

bool BlockIsDirty(cache * cache, global_block_id_t targetBlock)
{
    block_t toCheck = GetBlock(cache, targetBlock);
    if(!toCheck)
        return false;
    else
    { 
        //CONCERN: what if multiple lock contentions are served in non-fifo order?
	    //CONCERN: what if dirty state changes after releasing the lock and returning?
        pthread_mutex_lock(toMark->lock);
        bool state = toCheck->dirty;
        pthread_mutex_unlock(toMark->lock);
        return toCheck;
    }
}
