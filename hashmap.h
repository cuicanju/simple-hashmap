#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stdint.h>


#define INT_KEY(key) ((void*)(intptr_t)&(key)) 

#define INT_HASHMAP_INSERT(hashmap, key, value) \
    do { \
        int int_key = (int)(intptr_t)(key); \
        hashmap_insert(hashmap, INT_KEY(int_key), sizeof(int), value); \
    } while (0)

#define INT_HASHMAP_REMOVE(hashmap, key) \
    do { \
        int int_key = (int)(intptr_t)(key); \
        hashmap_remove(hashmap, INT_KEY(int_key), sizeof(int)); \
    } while (0)

#define INT_HASHMAP_GET(hashmap, key) \
    ({ \
        int int_key = (int)(intptr_t)(key); \
        (int*)hashmap_get(hashmap, INT_KEY(int_key), sizeof(int)); \
    });

#define ROTL32(x, r) ((x << r) | (x >> (32 - r)))

typedef struct HashMapNode {
    void* key;
    void* value;
    struct HashMapNode* next;
} HashMapNode;


typedef struct {
    size_t size;
    size_t capacity;
    HashMapNode** nodes;
} HashMap;

HashMap* hashmap_create(size_t capacity);

uint32_t hashmap_hashcode(const void *key, size_t len,size_t capacity);

int hashmap_insert(HashMap* hashmap, void* key,size_t len, void* value);

void* hashmap_get(HashMap* hashmap, void* key,size_t len);

void hashmap_remove(HashMap* hashmap, void* key,size_t len);

void hashmap_destroy(HashMap* hashmap);

uint32_t hashmap_hashcode(const void *key, size_t len,size_t capacity) {
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 4;

    uint32_t h1 = 0;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    for (int i = 0; i < nblocks; i++) {
        uint32_t k1;
        memcpy(&k1, data + i * 4, 4);

#ifdef __BIG_ENDIAN__
        k1 = __builtin_bswap32(k1);
#endif

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t *tail = data + nblocks * 4;
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
            k1 *= c1;
            k1 = ROTL32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    }

    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1 % capacity;
}


int hashmap_insert(HashMap* hashmap, void* key,size_t len, void* value) {
    size_t index = hashmap_hashcode(key,len, hashmap->capacity);

    HashMapNode* curr = hashmap->nodes[index];

    while (curr != NULL) {
        if (memcmp(curr->key,key,len) == 0) {
            curr->value = value;
            return 0;
        }
        curr = curr->next;
    }

    HashMapNode* node = (HashMapNode*)malloc(sizeof(HashMapNode));
    if(node == NULL) return -1;
    void* nodekey = malloc(len);
    if(nodekey == NULL) return -1;

    memcpy(nodekey,key,len);
    node->key = nodekey;
    node->value = value;
    node->next = hashmap->nodes[index];
    hashmap->nodes[index] = node;

    hashmap->size++;

    if (hashmap->size > hashmap->capacity * 0.75) {
        size_t new_capacity = hashmap->capacity * 2;
        HashMapNode** new_nodes = (HashMapNode**)calloc(new_capacity, sizeof(HashMapNode*));
        if(new_nodes == NULL) return -1;

        for (size_t i = 0; i < hashmap->capacity; i++) {
            curr = hashmap->nodes[i];
            while (curr != NULL) {
                HashMapNode* next = curr->next;
                size_t new_index = hashmap_hashcode(curr->key,len, new_capacity);
                curr->next = new_nodes[new_index];
                new_nodes[new_index] = curr;
                curr = next;
            }
        }

        free(hashmap->nodes);

        hashmap->nodes = new_nodes;
        hashmap->capacity = new_capacity;
    }

    return 0;
}

void* hashmap_get(HashMap* hashmap, void* key,size_t len) {
    size_t index = hashmap_hashcode(key,len, hashmap->capacity);

    HashMapNode* curr = hashmap->nodes[index];

    while (curr != NULL) {
        if (memcmp(curr->key,key,len) == 0) {
            void* value = curr->value;
            return value;
        }
        curr = curr->next;
    }

    return NULL;
}

void hashmap_remove(HashMap* hashmap, void* key,size_t len) {
    size_t index = hashmap_hashcode(key,len, hashmap->capacity);

    HashMapNode* prev = NULL;
    HashMapNode* curr = hashmap->nodes[index];

    while (curr != NULL) {
        if (memcmp(curr->key,key,len) == 0) {
            if (prev == NULL) {
                hashmap->nodes[index] = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr->key);
            free(curr);
            hashmap->size--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }

}

void hashmap_destroy(HashMap* hashmap) {
    for (size_t i = 0; i < hashmap->capacity; i++) {
        HashMapNode* curr = hashmap->nodes[i];
        while (curr != NULL) {
            HashMapNode* next = curr->next;
            free(curr->key);
            free(curr);
            curr = next;
        }
    }
    free(hashmap->nodes);
    free(hashmap);
}

HashMap* hashmap_create(size_t capacity) {
    HashMap* hashmap = (HashMap*)malloc(sizeof(HashMap));
    if(hashmap == NULL) return (HashMap *)-1;
    hashmap->size = 0;
    hashmap->capacity = capacity;
    hashmap->nodes = (HashMapNode**)calloc(capacity, sizeof(HashMapNode*));
    return hashmap;
}


