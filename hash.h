#ifndef HASH_H
#define HASH_H
#include <stdio.h>

struct HashPair {
    void* key;
    void* element;
};

struct HashList {
    struct HashList* next;
    struct HashPair* pair;
};

#define HASHSIZE 101

struct HashTable {
    struct HashList** hashListArr;
    int (*cmp)(void* a, void* b);
    unsigned int (*hash)(void* key);
    int (*onRemove)(struct HashPair*);
};

struct HashTable* HashTable_Init(struct HashTable* hashTable, int (*cmp)(void* a, void* b), unsigned int (*hash)(void* key), int (*onRemove)(struct HashPair*));
void HashTable_Destroy(struct HashTable* hashTable);
void* HashTable_Has(struct HashTable* hashTable, void* key);
void* HashTable_At(struct HashTable* hashTable, void* key);
int HashTable_Ready(struct HashTable* hashTable);

//*** impl

struct HashTable* HashTable_Init(struct HashTable* hashTable, int (*cmp)(void* a, void* b), unsigned int (*hash)(void* key), int (*onRemove)(struct HashPair*)) {
    hashTable->cmp = cmp;
    hashTable->hash = hash;
    hashTable->onRemove = onRemove;
    hashTable->hashListArr = calloc(HASHSIZE, sizeof(struct HashList*));
    return hashTable;
};

void HashTable_Destroy(struct HashTable* hashTable) {
    if(hashTable && HashTable_Ready(hashTable)) {
        for(int i = 0; i < HASHSIZE; i++) {
            struct HashList* l = hashTable->hashListArr[i];
            while(l) {
                struct HashList* next = l->next;
                if(hashTable->onRemove) {
                    hashTable->onRemove(l->pair);
                }
                free(l->pair);
                free(l);
                l = next;
            }
        }
        free(hashTable->hashListArr);
        hashTable->hash = 0;
    }
}

///version of At without malloc. will return null if key not found.
void* HashTable_Has(struct HashTable* hashTable, void* key) {
    unsigned int idx = hashTable->hash(key);
    struct HashList** list = &(hashTable->hashListArr[idx % HASHSIZE]);
    while(*list && hashTable->cmp(key, (*list)->pair->key)) {
        list = &((*list)->next);
    }
    return (list && *list) ? &((*list)->pair->element) : 0;
}

void* HashTable_At(struct HashTable* hashTable, void* key) {
    unsigned int idx = hashTable->hash(key);
    struct HashList** list = 0;
    struct HashList** next = &(hashTable->hashListArr[idx % HASHSIZE]);
    do{
        list = next;
        if(!(*list)) {
            struct HashList* l = malloc(sizeof(struct HashList));
            struct HashPair* p = malloc(sizeof(struct HashPair));
            p->key = key;
            p->element = 0;
            l->next = 0;
            l->pair = p;
            *list = l;
        }
        next = &((*list)->next);
    } while(hashTable->cmp(key, (*list)->pair->key));
    return &((*list)->pair->element);
}

int HashTable_Ready(struct HashTable* hashTable) {
    return hashTable && hashTable->hash;
}
#endif //HASH_H