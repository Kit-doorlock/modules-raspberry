#include "dictionary.h"

#define COMPARE(x, y) ((x) == (y))
// #define COMPARE(x, y) (strcmp((x), (y)) == 0)

#define APPEND(dest, val)((dest) = (val))
/*#define APPEND(dest, val) {          \
    ALLOC(dest);                        \
    strcpy(dest, val);                  \
} */

#define GETHASH(x) ((x) % (MAX_HASH))
/*#define GETHASH(x) {                 \
    int len = strlen(x);                \
    for (int i = 0; i < len; i++)       \
        total += x[i] % MAX_HASH;       \
    total %= MAX_HASH;                  \
} */

#define ALLOC(x)
// #define ALLOC(x) ((x) = (char*)malloc(sizeof(char) * (strlen(key)+1)))

#define FREE(x)
// #define FREE(x) free(x)

struct dictionary dict;

uint32_t appendElement(const uint32_t key) {
    struct __dict_chain *point;
    uchar_t hash;

    hash = getHash(key);

    // move to last element
    if (dict.head[hash].next == NULL) {
        // if no element in head << create new chain
        point = (struct __dict_chain*)malloc(sizeof(struct __dict_chain));
        dict.head[hash].next = point;
    }
    else {
        point = dict.head[hash].next;
        while (point->next != NULL) {
            if (COMPARE(point->key, key)) {
                return dict.head[hash].count;
            }
            point = point->next;
        }
        if (COMPARE(point->key, key) == 0) {
            return dict.head[hash].count;
        }
        point->next = (struct __dict_chain *)malloc(sizeof(struct __dict_chain));
        point = point->next;
    }

    // set key and value
    point->next = NULL;
    APPEND(point->key, key);

    dict.head[hash].count++;
    return dict.head[hash].count;
}

uint32_t deleteElement(const uint32_t key) {
    struct __dict_chain *point, *prev;
    uchar_t hash;

    hash = getHash(key);

    // unable delete element << no element in dictionary
    if (dict.head[hash].count == 0) return -1;

    // search key is ${key} element
    point = dict.head[hash].next;
    while (!COMPARE(key, point->key)) {
        prev = point;
        point = point->next;

        // if point is pointing NULL, it is error
        if (point == NULL) return -1;
    }

    // modify chain
    if (point == dict.head[hash].next)
        dict.head[hash].next = point->next;
    else
        prev->next = point->next;

    free(point);


    dict.head[hash].count--;
    return dict.head[hash].count;
}

bool checkExist(const uint32_t key) {
    struct __dict_chain *point;
    uchar_t hash;

    hash = GETHASH(key);

    if (dict.head[hash].count == 0) return false;

    point = dict.head[hash].next;
    while (point != NULL) {
        if (COMPARE(point->key, key)) {
            return true;
        }
        point = point->next;
    }
    return false;
}

void initDictionary() {
    int i;
    for (i = 0; i < MAX_HASH; i++) {
        dict.head[i].count = 0;
        dict.head[i].next = NULL;
    }
}

void clearDictionary() {
    int i, j, count;
    struct __dict_chain *point, *next;

    for (i = 0; i < MAX_HASH; i++) {
        point = dict.head[i].next;
        dict.head[i].next = NULL;

        count = dict.head[i].count;

        for (j = 0; j < count; j++) {
            next = point->next;
            FREE(point->key);
            free(point);
            point = next;
        }

        dict.head[i].count = 0;
    }
}

uchar_t getHash(const uint32_t key) {
    uint32_t hash;
    hash = GETHASH(key);
    return hash;
}

void showAllDictionary() {
    int i;

    for (i = 0; i < MAX_HASH; i++)
        if (dict.head[i].count > 0) showDictionary(i);

    return;
}

void showDictionary(int hash) {
    struct __dict_chain *point;

    if (hash < 0 || hash >= MAX_HASH) return;
    printf("datas of hash - %2d\n", hash);
    point = dict.head[hash].next;

    while (point != NULL) {
        printf("\t%u\n", point->key);
        point = point->next;
    }
    printf("=====================\n");
    return;
}