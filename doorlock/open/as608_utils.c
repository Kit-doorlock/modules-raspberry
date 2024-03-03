#include "as608_utils.h"
#include "as608_module.h"

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#define WAIT_DETECT 1
#define WAIT_NDETECT 0

bool waitUntilDetectFinger(int wait_time);
bool waitUntilNotDetectFinger(int wait_time);

ret nsearch_fp();
ret hsearch_fp();

bool PS_Exit() {
    printf("ERROR! code=%02X, desc=%s\n", g_error_code, PS_GetErrorDesc());
    exit(2);
    return true;
}

// wait_time : 대기시간
// flag :   waitUntilDetectFinger - 1
//          waitUntilNotDetectFinger - 0
bool waitFinger_fp(const int wait_time, const int flag) {
    if (flag)
        return waitUntilDetectFinger(wait_time);
    return waitUntilNotDetectFinger(wait_time);
}

// 최대 wait_time만큼 손가락이 감지될때까지 대기합니다.
bool waitUntilDetectFinger(int wait_time) {
    while (true) {
        if (PS_DetectFinger())
            return true;
        else {
            delay(100);
            wait_time -= 100;
            if (wait_time < 0)
                return false;
        }
    }
}

// 최대 wait_time만큼 손가락이 감지되지 않을때까지 대기합니다.
bool waitUntilNotDetectFinger(int wait_time) {
    while (true) {
        if (!PS_DetectFinger())
            return true;
        else {
            delay(100);
            wait_time -= 100;
            if (wait_time < 0)
                return false;
        }
    }
}

// 일반 검색 - 0
// 고속 검색 - 1
ret search_fp(const int flag) {
    printf("Please put your finger on the module.\n");

    waitFinger_fp(MAX_WAIT, WAIT_DETECT);

    PS_GetImage() || PS_Exit();
    PS_GenChar(1) || PS_Exit();

    if (!flag)
        return nsearch_fp();
    return hsearch_fp();
}

// 지문 일반 검색
ret nsearch_fp() {
    ret value = {-1, -1};
    PS_Search(1, 0, 300, &(value.pageID), &(value.score));
    // if (!PS_Search(1, 0, 300, &(value.pageID), &(value.score)))
    //     PS_Exit();
    // else
    //     printf("Matched! pageID=%d score=%d\n", value.pageID, value.score);
    return value;
}

// 지문 빠른 검색
ret hsearch_fp() {
    ret value = {-1, -1};
    PS_HighSpeedSearch(1, 0, 300, &(value.pageID), &(value.score));
    // if (!PS_HighSpeedSearch(1, 0, 300, &(value.pageID), &(value.score)))
    //     PS_Exit();
    // else
    //     printf("Matched! pageID=%d score=%d\n", value.pageID, value.score);
    return value;
}