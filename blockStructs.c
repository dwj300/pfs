#include "blockStructs.h"

block_list_node_t* initializeBlockListNode() {
    block_list_node_t* newBlockListNode = (block_list_node_t* )malloc(sizeof(block_list_node_t));
    if(newBlockListNode) {
        newBlockListNode->block = initializeBlock();
        if(newBlockListNode->block) {
            newBlockListNode->previous = NULL;
            newBlockListNode->next = NULL;
            return newBlockListNode;
        }
    }
    return NULL;
}

block_list_t * initializeBlockList() {
    block_list_t* newBlockList = (block_list_t* )malloc(sizeof(block_list_t));
    if(newBlockList){
        newBlockList->head = newBlockList->tail = NULL;
        return newBlockList;
    }
    return NULL;
}


bool addNodeToHeadOfList(block_list_t * targetList, block_list_node_t * newHead)
{
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
        *targetQueueHead = newHead;
        return true;
    }
}


block_list_node_t * removeNodeFromHeadOfList(block_list_t * sourceList)
{
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


block_list_node_t * removeNodeFromTailOfList(block_list_t * sourceList)
{
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


block_list_node_t * removeNodeFromList(block_list_t * hostList, block_list_node_t * toRemove){
    if(!hostList){
        fprintf(stderr, "Host list doesn't exist, aborting block removal");
        return NULL;
    }
    if(!(sourceList->head) || !(sourceList->tail)){
        fprintf(stderr, "Source list is empty or corrupt, aborting block removal");
        return NULL;
    }
    if(toRemove == hostList->head){
        return removeNodeFromHeadOfList(toRemove);
    }
    if(toRemove == hostList->tail){
        return removeNodeFromTailOfList(toRemove);
    }
    else{
        //Already checked the head
        block_list_node_t * sentinel = hostList->head;
        while(sentinel != hostList->tail){
            if(sentinel == toRemove){
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
