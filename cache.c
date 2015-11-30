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

//Destroying a block list node DOES NOT modify the block within it, it simplies frees the list node holding it
void destroyBlockListNode(block_list_node_t * toDestroy)
{
    if(!toDestroy)
    {
        print("You can not destroy a null block list node");
    }
    free(toDestroy);     
}


void addBlockToBlockList(block_t * blockToAdd, block_list_node_t * headOfTargetList)
{
    if(!headOfTargetList)
    {
        print("Target block list doesn't exist, aborting block add");
        return;
    }
    if(!blockToAdd)
    {
        print("You can't add a NULL block to a list, aborting block add");
        return;
    }
    block_list_node_t * sentinel = headOfTargetList;
    while(sentinel->next)
    {
        sentinel = sentinel->next;
    }
    sentinel->next = blockToAdd;
}


void removeBlockFromBlockList(global_block_id_t idOfBlockToRemove, block_list_node_t * headOfHostList)
{
    if(!headOfHostList)
    {
        print("Host block list doesn't exist, aborting block removal");
        return;
    }
    block_list_node_t * sentinel = headOfHostList;
    block_list_node_t * last = NULL;
    while(sentinel)
    {
        if(sentinel->block->id == idOfBlockToRemove)
        {
            if(last)
            {
                //Reconnect
                last->next = sentinel->next;
            }
            destroyBlockListNode(sentinel);
        }
        else
            sentinel = sentinel->next;
    }
    print("Host block list didn't contain the specified block, aborting block removal");
}



block_t * popBlockFromBlockList(block_list_node_t * headOfHostList)
{
    if(!headOfHostList)
    {
        print("Host block list doesn't exist, aborting block pop");
        return;
    }
    if(!headOfHostList->block)
    {
        print("Host block list is empty, aborting block pop");
        return;
    }
    block_t * freeBlock = headOfHostList->block;
    headOfHostList
    headOfHostList = headOfHostList->next;
    while(sentinel)
    {
        if(sentinel->block->id == idOfBlockToRemove)
            return sentinel->block;
        else
            sentinel = sentinel->next;
    }
    print("Host block list didn't contain the specified block, aborting block removal");
}





block_t * initializeBlock()
{
    block_t * newBlock = (block_t *)malloc(sizeof(block_t));
    if(newBlock)
    {    
        newBlock->lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newBlock->lock, NULL);
        newBlock->global_block_id_t = BLOCK_IS_FREE;
        newBlock->dirty = false;
		newBlock->offset = 0;
        newBlock->data = NULL;
        return newBlock;
    }
    return NULL;  
}

//Blocks represent cache blocks, which aren't actually 'destroyed' - just 'cleared':
//   after clearing, the block should be on the free list only, it's state should be equivalent to its initalized state
//   if the block is dirty, it must be written through to its host server
void clearBlock(cache_t * cache, block_t * toClear)
{
    if(!toClear)
    {
        print("You can not clear a null block");
    }
    pthread_mutex_lock(toClear->lock);
    if(toClear->dirty)
        print("Pushing block to server");
        
    pthread_mutex_unlock(toClear->lock);

}

block_t * GetBlockFromCache(cache * cache, global_block_id_t targetBlock)
{
    if(!cache || !cache->ActiveBlocks)
        print("The cache object has no been properly initialized"); 
    //Look into active map and attempt to find target block
    //void *Hashmap_get(Hashmap *map, void *key);  
    return (block_t *)Hashmap_get(cache->ActiveBlocks, &targetBlock);    
}

block_t * GetFreeBlockFromCache(cache * cache, global_block_id_t targetBlock)
{
    if(!cache || !cache->FreeList)
        print("The cache object has no been properly initialized"); 
    //Attempt to get a block from the free list
    
    //void *Hashmap_get(Hashmap *map, void *key);  
    return (block_t *)Hashmap_get(cache->ActiveBlocks, &targetBlock);    
}

void FetchBlockFromServer(cache * targetCache, global_block_id_t idOfBlockToFetch)
{
    //Lookup in the recipe which server has the block
    return    
}




bool MarkBlockDirty(cache * cache, global_block_id_t targetBlock)
{
    block_t toMark = GetBlock(cache, targetBlock);
    if(!toMark)
        return false;
    else
    {
        if(toMark->dirty)
            print("block was already dirty"); 
        //CONCERN: what if multiple lock contentions are served in non-fifo order?
        pthread_mutex_lock(toMark->lock);
        toMark->dirty = true;
        //Add to dirty list
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
