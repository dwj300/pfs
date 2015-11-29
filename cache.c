#include "cache.h"

//typedef int (*Hashmap_compare)(void *a, void *b);
Hashmap_compare * compareBlocks(void *a, void *b)
{
    return( *((global_block_id_t*)b) - *((global_block_id_t*)a));
}

cache_t * InitializeCache()
{
    cache * newCache = (cache *)malloc(sizeof(cache));
    newCache->FreeList = initializeBlockListNode();
    newCache->DirtyList = initializeBlockListNode();

    //Hashmap *Hashmap_create(Hashmap_compare compare, Hashmap_hash);
	newCache->ActiveBlocks = Hashmap_create((Hashmap_compare)

    block_list_node_t *FreeList;
    block_list_node_t *DirtyList;
    Hashmap * ActiveBlocks;
    struct blockListNode *next;

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
        newBlock->partOf = BLOCK_DOES_NOT_EXIST;
        newBlock->dirty = false;
		newBlock->offset = 0;
        newBlock->data = NULL;
        return newBlock;
    }
    return NULL;  
}



typedef struct blockListNode
{
    block_t *block;
    struct blockListNode *next;
} block_list_node_t;


block_t * GetBlock(cache * cache, global_block_id_t targetBlock)
{
    if(!cache || !cache->ActiveBlocks)
        print(
    //Look into active map and attempt to find target block
    //void *Hashmap_get(Hashmap *map, void *key);  
    (block_t *)Hashmap_get(cache->ActiveBlocks, targetBlock);
    
}
