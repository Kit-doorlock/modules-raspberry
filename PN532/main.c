#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

int main() {
    char command[20];
    initPN532();

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