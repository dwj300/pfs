#include "blockStructs.h"

block_list_node_t * initializeBlockListNode(block_t * toEncapsulate){
    if(!toEncapsulate){
        fprintf(stderr, "You must initialize a block before encapsulating it in a node.");
        return NULL;
    }

    block_list_node_t* newBlockListNode = (block_list_node_t* )malloc(sizeof(block_list_node_t));
    if(newBlockListNode) {
        newBlockListNode->block = toEncapsulate;
        newBlockListNode->previous = NULL;
        newBlockListNode->next = NULL;
        return newBlockListNode;
    }
    else{
        fprintf(stderr, "Memory allocation for block node failed.");
        return NULL;
    }
}


block_list_t * initializeBlockList(){
    block_list_t* newBlockList = (block_list_t* )malloc(sizeof(block_list_t));
    if(newBlockList){
        newBlockList->head = newBlockList->tail = NULL;
        return newBlockList;
    }
    else{
        fprintf(stderr, "Memory allocation for block list failed.");
        return NULL;
    }
}



//Creates and sets up a cache block object
//  This should ONLY be called on cache initialization
block_t* initializeBlock(byte * memorySegment) {
    block_t* newBlock = (block_t* )malloc(sizeof(block_t));
    if(newBlock) {
        newBlock->lock = (pthread_mutex_t* )malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(newBlock->lock, NULL);
        newBlock->id = BLOCK_IS_FREE;
        newBlock->dirty = false;
		//newBlock->offset = 0;
        newBlock->data = memorySegment;
        return newBlock;
    }
    else{
        fprintf(stderr, "Memory allocation for block failed.");
        return NULL;
    }
}


//Destroys (de-allocates) a cache block object
//  This should ONLY be called on cache destruction / closing the file system
//  Returns true if successful, false otherwise
//      -Returns false if someone is holding the lock on the block
//                     OR if the block is dirty
bool destroyBlock(block_t* toDeallocate) {
    if(!toDeallocate){
        fprintf(stderr, "A block must exist to destroy it.");
        return false;
    }

    pthread_mutex_lock(toDeallocate->lock);

    if(!blockIsCleared(toDeallocate)){
        fprintf(stderr, "A block must be cleared to destroy it.");
        return false;
    }

    free(toDeallocate->lock);
    free(toDeallocate);
    return true;
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



block_mapping_node_t * initializeMappingNode(block_list_node_t * blockNodeBeingMapped){
    if(!blockNodeBeingMapped){
        fprintf(stderr, "You cannot initialize a mapping node without giving the block node it will map to.");
        return NULL;
    }

    block_mapping_node_t * newMappingNode = (block_mapping_node_t *)malloc(sizeof(block_mapping_node_t));
    if(!newMappingNode){
        fprintf(stderr, "Allocation for the mapping node failed.");
        return NULL;
    }

    newMappingNode->mappedBlockNode = blockNodeBeingMapped;
    newMappingNode->next = NULL;
    return newMappingNode;
}


bool destroyMappingNode(block_mapping_node_t * toDestroy){
    if(!toDestroy){
        fprintf(stderr, "You cannot destroy a null mapping node.");
        return false;
    }
    free(toDestroy);
    return true;
}


bool addNodeToHeadOfList(block_list_t * targetList, block_list_node_t * newHead){
    if(!newHead){
        fprintf(stderr, "Block node doesn't exist, aborting block push");
        return false;
    }
    if(!targetList){
        fprintf(stderr, "List doesn't exist, aborting block push");
        return false;
    }
    //Sanity checks
    if(newHead->previous || newHead->next){
        newHead->previous = newHead->next = NULL;
        fprintf(stderr, "WARN: adding a previously-linked node to a list, it's previous host list might be broken");
    }

    //If we're inserting into an empty list
    if(!(targetList->head) && !(targetList->tail)){
        targetList->head = targetList->tail = newHead;
    }
    else
    {
        //If we are moving the tail to the head, we have a new tail
        if( newHead == targetList->tail){
            targetList->tail = targetList->tail->previous;
        }

        //Connect the new head to the target queue
        newHead->next = targetList->head;
        targetList->head->previous = newHead;

        //Replace the head
        targetList->head = newHead;
    }
    return true;
}


block_list_node_t * removeNodeFromHeadOfList(block_list_t * sourceList){
    if(!sourceList){
        fprintf(stderr, "Source list doesn't exist, aborting block removal");
        return NULL;
    }
    if(!(sourceList->head) || !(sourceList->tail)){
        fprintf(stderr, "Source list is empty or corrupt, aborting block removal");
        return NULL;
    }

    block_list_node_t * toReturn = sourceList->head;

    //If the node isn't the only one in the list, we promote the second in the list
    if(sourceList->head->next){
        sourceList->head->next->previous = NULL;
        sourceList->head = sourceList->head->next;
    }
    else{//If the node was the only one in the list, the list is now empty
        sourceList->head = sourceList->tail = NULL;
    }

    //Unlink the node being removed
    toReturn->previous = toReturn->next = NULL;
    return toReturn;
}


block_list_node_t * removeNodeFromTailOfList(block_list_t * sourceList){
    if(!sourceList){
        fprintf(stderr, "Source list doesn't exist, aborting block removal");
        return NULL;
    }
    if(!(sourceList->head) || !(sourceList->tail)){
        fprintf(stderr, "Source list is empty or corrupt, aborting block removal");
        return NULL;
    }

    block_list_node_t * toReturn = sourceList->tail;

    //If the node isn't the only one in the list, we demote the penultimate node
    if(sourceList->tail->previous){
        sourceList->tail->previous->next = NULL;
        sourceList->tail = sourceList->tail->previous;
    }
    else{//If the node was the only one in the list, the list is now empty
        sourceList->head = sourceList->tail = NULL;
    }

    //Unlink the node being removed
    toReturn->previous = toReturn->next = NULL;
    return toReturn;
}


block_list_node_t * removeNodeFromListByID(block_list_t * hostList, global_block_id_t IDOfBlockToRemove){
    if(!hostList){
        fprintf(stderr, "Host list doesn't exist, aborting block removal");
        return NULL;
    }
    if(!(hostList->head) || !(hostList->tail)){
        fprintf(stderr, "Source list is empty or corrupt, aborting block removal");
        return NULL;
    }
    if(hostList->head->block->id == IDOfBlockToRemove){
        return removeNodeFromHeadOfList(hostList);
    }
    if(hostList->tail->block->id == IDOfBlockToRemove){
        return removeNodeFromTailOfList(hostList);
    }
    else{
        //Already checked the head
        block_list_node_t * sentinel = hostList->head;
        while(sentinel != hostList->tail){
            if(sentinel->block->id == IDOfBlockToRemove){
                sentinel->previous->next = sentinel->next;
                sentinel->next->previous = sentinel->previous;
                sentinel->previous = sentinel->next = NULL;
                return sentinel;
            }
            sentinel = sentinel->next;
        }
        fprintf(stderr, "WARN: Source list didn't contain the node requested, returning NULL.");
        return NULL;
    }
}


bool removeNodeFromList(block_list_t * hostList, block_list_node_t * toRemove){
    if(!hostList){
        fprintf(stderr, "Host list doesn't exist, aborting block removal");
        return false;
    }
    if(!(hostList->head) || !(hostList->tail)){
        fprintf(stderr, "Source list is empty or corrupt, aborting block removal");
        return false;
    }

    if(toRemove == hostList->head){
        removeNodeFromHeadOfList(hostList);
        return true;
    }
    if(toRemove == hostList->tail){
        removeNodeFromTailOfList(hostList);
        return true;
    }
    //Already checked the head
    toRemove->previous->next = toRemove->next;
    toRemove->next->previous = toRemove->previous;
    toRemove->previous = toRemove->next = NULL;
    return true;
}

