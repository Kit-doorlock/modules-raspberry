#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 한국어는 3byte - 유니코드 적용?

int hashingString(const char* buffer) {
    int i, checksum = 0;
    for (i = 0; i < strlen(buffer); i++) {
        checksum += buffer[i];
        checksum %= 26;
    }
    return checksum;
}

int main() {
    char asdf[100], buf[100];
    scanf("%s", asdf);

    int checksum = 0;

    checksum = hashingString(asdf);
    printf("%d\n", checksum);
}