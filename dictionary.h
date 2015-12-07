#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DICTSIZE 100

typedef struct entry {
    struct entry* next;
    struct entry* prev;
    char* key;
    void* value;
} entry_t;

typedef entry_t** dictionary_t;

unsigned hash(char* word);
entry_t* lookup(dictionary_t dict, char* s);
int insert(dictionary_t dict, char* key, void* data);
int delete(dictionary_t dict, char* key);
void dictionary_print(dictionary_t dict);
void to_lower(char *s);
dictionary_t create_dictionary();