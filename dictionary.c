#include "dictionary.h"

dictionary_t create_dictionary() {
    return malloc(DICTSIZE * sizeof(entry_t*));
}

unsigned hash(char *s) {
    fprintf(stderr, "hashing: %s\n", s);
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++) {
        hashval = *s + 31 * hashval;
    }
    fprintf(stderr, "%u\n", hashval % DICTSIZE);
    return hashval % DICTSIZE;
}

// iterate over the list at the hash of s and then strcmp
// and attempt to find s
entry_t* lookup(dictionary_t dict, char* s) {
    fprintf(stderr, "here\n");
    unsigned h = hash(s);
    fprintf(stderr, "here12\n");
    entry_t* e = dict[h];
    fprintf(stderr, "here24\n");
    fprintf(stderr, "foosdfsf");
    for (e = dict[hash(s)]; e != NULL; e = e->next) {
        if (strcmp(s, e->key) == 0) {
            return e;
        }
    }
    // If not found
    fprintf(stderr, "hmm");

    return NULL;
}

int insert(dictionary_t dict, char* key, void* data) {
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
        e->value = data;
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
    fprintf(stderr, "ERROR: not implemented yet\n");
    exit(1);
}

void dictionary_print(dictionary_t dict) {
    int e;
    entry_t* p;
    for (e = 0; e < DICTSIZE; e++) {
        for (p = dict[e]; p != NULL; p = p->next) {
            printf("%s -> %s\n", p->key, p->value);
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
