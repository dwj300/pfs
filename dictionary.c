#include "dictionary.h"

dictionary_t create_dictionary() {
    fprintf(stderr, "Initializing dictionary. Size:%lu bytes\n", (long unsigned)(DICTSIZE * sizeof(entry_t*)));
    return calloc(DICTSIZE, sizeof(entry_t*));
}

unsigned hash(char *s) {
    fprintf(stderr, "hashing: %s\n", s);
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++) {
        hashval = *s + 31 * hashval;
    }
    fprintf(stderr, "hash:%u\n", hashval % DICTSIZE);
    return hashval % DICTSIZE;
}

// iterate over the list at the hash of s and then strcmp
// and attempt to find s
entry_t* lookup(dictionary_t dict, char* s) {
    entry_t* e;
    for (e = dict[hash(s)]; e != NULL; e = e->next) {
        if (strcmp(s, e->key) == 0) {
            return e;
        }
    }
    // If not found
    fprintf(stderr, "Key: %s not found\n", s);

    return NULL;
}

int insert(dictionary_t dict, char* key, void* data) {
    fprintf(stderr, "Inserting key: %s\n", key);
    entry_t* e;
    unsigned hashval;
    // not found, create entry
    if ((e = lookup(dict, key)) == NULL) {
        e = (entry_t*)malloc(sizeof(entry_t));
        // dictionary error
        if (e == NULL || (e->key = strdup(key)) == NULL) {
            fprintf(stderr, "Problem adding to dictionary\n");
            return 1;
        }
        hashval = hash(key);
        e->next = dict[hashval];
        if (e->next != NULL) {
            e->next->prev = e;    
        }
        e->value = data;
        e->prev = NULL;
        dict[hashval] = e;
    }
    // key found
    else {
        fprintf(stderr, "key found\n");
        return -1;
    }
    return 0;
}

int delete(dictionary_t dict, char* key) {
    fprintf(stderr, "deleting: %s\n", key);
    entry_t *e = lookup(dict, key);
    if (e == NULL) {
        fprintf(stderr, "key not found\n");
        return -1;
    }   
    unsigned h = hash(key);
    if (dict[h] == e) {
        dict[h] = e->next;
    }
    else {// find previous node, if it exists // if(e->prev != NULL) {
        e->prev->next = e->next;
    }

    if(e->next != NULL) {
        e->next->prev = e->prev;
    }
    // User is responsible for freeing their own data!
    free(e);
    return 1;
}

void dictionary_print(dictionary_t dict) {
    int e;
    entry_t* p;
    for (e = 0; e < DICTSIZE; e++) {
        for (p = dict[e]; p != NULL; p = p->next) {
            fprintf(stderr, "%s -> %s\n", p->key, (char*)p->value);
        }
    }
}

void test_dictionary() {
    dictionary_t dict = create_dictionary();
    insert(dict, "key1", "value1");
    insert(dict, "key2", "value2");
    insert(dict, "key3", "value3");
    dictionary_print(dict);
}
