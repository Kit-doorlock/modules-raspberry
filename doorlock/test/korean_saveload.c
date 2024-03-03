#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "datas.txt"
#define CHECKSUM(x) ((x[0]+x[1]+x[2]) == 0)

FILE* fp;
char* buffer;

void removeNewline(char* str);

void saveDictionary() {
    char datas[500][50] = {'\0'};

    // ===================== TODO =========================
    // * 딕셔너리에서 버퍼에 값을 넣는 과정 추가


    // ===================== ==== =========================
    FILE* fp = fopen(PATH, "w");

    for (int i = 0; i < 500; i++) {
        if (CHECKSUM(datas[i])) break;
        fwrite(datas[i], sizeof(char), strlen(datas[i]), fp);
    }

    fclose(fp);
}

void loadDictionary() {
    char datas[500][50];
    int count = 0;
    FILE* fp = fopen(PATH, "r");

    if (fp == NULL) {
        printf("No File\n");
        return;
    }

    while (fgets(datas[count], (sizeof(datas[count]) / sizeof(char)), fp)) {
        removeNewline(datas[count]);
        if (datas[count][0] != '\0') count++;
    }

    fclose(fp);

    // ===================== TODO =========================
    // * 버퍼에서 딕셔너리에 값을 넣는 과정 추가


    // ===================== ==== =========================
}

void removeNewline(char* buffer) {
    if (buffer[strlen(buffer)-2] == '\r')
        buffer[strlen(buffer)-2] = '\0';
    if (buffer[strlen(buffer)-1] == '\n')
        buffer[strlen(buffer)-1] = '\0';
}

int main() {
    loadDictionary();
    saveDictionary();
}