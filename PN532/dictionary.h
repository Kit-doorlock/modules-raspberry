#ifndef __DICT__
#define __DICT__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_HASH 26
#define MAX_BUFFER 201

#define DIR_IN 0
#define DIR_OUI 1

typedef unsigned char uchar_t;
typedef unsigned int uint32_t;

struct __dict_chain {
    uint32_t key;
    struct __dict_chain* next;
};

struct __dict_head {
    uint32_t count;
    struct __dict_chain* next;
};

struct dictionary {
    struct __dict_head head[MAX_HASH];
};

extern struct dictionary dict;

uint32_t appendElement(const uint32_t key);
uint32_t deleteElement(const uint32_t key);

bool checkExist(const uint32_t key);

void initDictionary();
void clearDictionary();

uchar_t getHash(const uint32_t key);

void showAllDictionary();
void showDictionary(int hash);

#endif