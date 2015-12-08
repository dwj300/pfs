#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "sharedresources.h"
#include <string.h>

typedef int32_t file_desc_t;
typedef int32_t offset_t;
typedef char byte;

// General struct definitions
typedef struct block{
    global_block_id_t id;
    global_server_id_t host;
    pthread_mutex_t *lock;
    bool dirty;
    byte *data;
} block_t;

//Block Access Methods
block_t * initializeBlock();
bool destroyBlock(block_t* toDeallocate);


typedef struct block_list_node{
    block_t * block;
    struct block_list_node * previous;
    struct block_list_node * next;
} block_list_node_t;

block_list_node_t * initializeBlockListNode();


typedef struct block_mapping_node{
    block_list_node_t * mappedBlockNode;
    struct block_mapping_node * next;
} block_mapping_node_t;

block_mapping_node_t * initializeMappingNode(block_list_node_t * blockNodeBeingMapped);
bool destroyMappingNode(block_mapping_node_t * toDestroy);


typedef struct block_list{
    block_list_node_t* head;
    block_list_node_t * tail;
} block_list_t;

block_list_t * initializeBlockList();


bool addNodeToHeadOfList(block_list_t * targetList, block_list_node_t * newHead);
bool blockIsCleared(block_t* block);
block_list_node_t * removeNodeFromHeadOfList(block_list_t * sourceList);
block_list_node_t * removeNodeFromTailOfList(block_list_t * sourceList);
block_list_node_t * removeNodeFromListByID(block_list_t * hostList, global_block_id_t IDOfBlockToRemove);
bool removeNodeFromList(block_list_t * hostList, block_list_node_t * toRemove);


typedef struct id_list_node{
    uint32_t id;
    struct id_list_node * next;
} id_list_node_t;

id_list_node_t * initializeIDListNode(global_block_id_t IDToEncapsulate);
bool destroyIDListNode(id_list_node_t * toDestroy);
bool removeIDFromIDList(id_list_node_t ** headOfTargetList, global_block_id_t toRemove);
bool addIDToIDList(id_list_node_t ** headOfTargetList, global_block_id_t newID);
global_block_id_t popIDFromIDList(id_list_node_t ** headOfTargetList);
