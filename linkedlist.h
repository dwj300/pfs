
typedef struct node {
    void* data;
    struct node* next;
    struct node* prev;
} node_t;

typedef struct list {
    node_t* head;
    node_t* tail;
} list_t;

list_t* create_list();