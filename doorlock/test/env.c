#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSWORD_ENV "DOORLOCK_PASSWORD"

typedef unsigned char byte;

typedef struct password_data {
    char password[100];
    byte len; 
} password;

void setPasswordEnv(const char* password) {
    char buf[100];
    sprintf(buf, "%s,%d", password, strlen(password));
    setenv(PASSWORD_ENV, buf, 1);
}

password getPasswordEnv() {
    char* pass = getenv(PASSWORD_ENV);
    password ret_value;
    strcpy(ret_value.password, strtok(pass, ","));
    ret_value.len = atoi(strtok(NULL, " "));
    return ret_value;
}

int main() {
    const char* val = "010234f1529f";
    password pass;
    setPasswordEnv(val);

    pass = getPasswordEnv();
    if (pass.len > 0) {
        printf("%s %d\n", pass.password, pass.len);
    }

}