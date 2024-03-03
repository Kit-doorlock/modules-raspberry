#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>

// AS608 - 0
// PN532 - 1
// TTP229 - 2



int main() {
    int thread_ret_val[3], threadpool[3];

    if (wiringPiSetupGpio() == -1) return 1;


}