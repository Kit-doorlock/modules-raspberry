#include "utils.h"
#include "as608.h"

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

// fp_id 번호에 지문을 추가합니다.
int add_fp(const int fp_id) {
    printf("Please put your finger on the module.\n");
    if (waitUntilDetectFinger(5000)) {
        delay(500);
        PS_GetImage() || PS_Exit();
        PS_GenChar(1) || PS_Exit();
    } else {
        printf("Error: Didn't detect finger!\n");
        exit(1);
    }

    // 손가락을 뗐는지 확인
    printf("Ok.\nPlease raise your finger!\n");
    if (waitUntilNotDetectFinger(5000)) {
        delay(100);
        printf("Ok.\nPlease put your finger again!\n");
        // 지문 재입력
        if (waitUntilDetectFinger(5000)) {
            delay(500);
            PS_GetImage() || PS_Exit();
            PS_GenChar(2) || PS_Exit();
        } else {
            printf("Error: Didn't detect finger!\n");
            exit(1);
        }
    } else {
        printf("Error! Didn't raise your finger\n");
        exit(1);
    }

    int score = 0;
    if (PS_Match(&score)) {
        printf("Matched! score=%d\n", score);
    } else {
        printf("Not matched, raise your finger and put it on again.\n");
        exit(1);
    }

    if (g_error_code != 0x00)
        PS_Exit();

    // 특성 파일을 병합합니다.
    PS_RegModel() || PS_Exit();
    PS_StoreChar(2, fp_id) || PS_Exit();

    printf("OK! New fingerprint saved to pageID=%d\n", fp_id);

    return fp_id;
}

// 빈 자리를 찾아서 지문을 추가합니다.
int enroll_fp() {
    int count = 0;
    printf("Please put your finger on the moudle\n");
    while (digitalRead(g_as608.detect_pin) == LOW) {
        delay(1);
        if ((count++) > 5000) {
            printf("Not detected the finger!\n");
            exit(2);
        }
    }

    int pageID = 0;
    PS_Enroll(&pageID) || PS_Exit();

    printf("OK! New fingerprint saved to pageID=%d\n", pageID);

    return pageID;
}

// 모든 데이터를 제거합니다.
void deleteall_fp() {
    PS_Empty() || PS_Exit();
    printf("OK!\n");
}

// 무조건 하나만 지우도록 합니다.
void delete_fp(const int fp_id) {
    int count = 1;

    PS_DeleteChar(fp_id, count) || PS_Exit();
    printf("OK!\n");
}

// 전체 등록된 개수를 가져옵니다.
// len(FingerprintList)
int count_fp() {
    int count = 0;
    PS_ValidTempleteNum(&count) || PS_Exit();
    printf("%d\n", count);
    return count;
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

// 사용중인 리스트 반환
void list_fp() {
    int indexList[512] = {0};
    PS_ReadIndexTable(indexList, 512) || PS_Exit();

    int i = 0;
    for (i = 0; i < 300; ++i) {
        if (indexList[i] == -1)
            break;
        printf("%3d ", indexList[i]);
        if ((i+1) % 10 == 0)
            printf("\n");
    }
    if (i > 0)
        printf("\n");
    else if (i == 0)
        printf("The database is empty!\n");
}