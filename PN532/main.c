#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

#define ADD "add"
#define VERIFY "verify"
#define DELETE "delete"
#define EXIT "exit"

void showCommandList() {
    printf(" ========= command ========= \n");
    printf("%10s %10s\n", ADD, VERIFY);
    printf("%10s %10s\n", DELETE, EXIT);
    printf(" =========================== \nenter command: ");
}

int main(int argc, char* argv[]) {
    char command[20];
    int init_res = 0;

    if (argc == 1 || argc > 2) {
        printf("usage: ./main <MODE>\nmode - UART:0, SPI:1, I2C:2\n");
        return 1;
    }

    init_res = initPN532(atoi(argv[1]));
    if (init_res == -1) return 2;
    else if (init_res == -2) return 3;

    while (1) {
        showCommandList();
        scanf("%s", command);

        if (strcmp(command, ADD) == 0) {
            addCard();
        } else if (strcmp(command, VERIFY) == 0) {
            printf("%10s\n", existCard() == true ? "Exist" : "Not Exist");
        } else if (strcmp(command, DELETE) == 0) {
            deleteCard();
        } else if (strcmp(command, EXIT) == 0 || strcmp(command, "e") == 0) {
            break;
        }
    }
}