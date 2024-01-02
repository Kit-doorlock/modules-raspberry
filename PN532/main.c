#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

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
        scanf("%s", command);

        if (strcmp(command, "add") == 0) {
            addCard();
        } else if (strcmp(command, "verify") == 0) {
            printf("%10s\n", existCard() == true ? "Exist" : "Not Exist");
        } else if (strcmp(command, "delete") == 0) {
            deleteCard();
        } else if (strcmp(command, "exit") == 0 || strcmp(command, "e") == 0) {
            break;
        }
    }
}