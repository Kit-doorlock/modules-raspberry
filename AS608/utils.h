#include <stdbool.h>

#define MAX_WAIT 10000 // ms

typedef struct fingerprint_return_value {
    int pageID;
    int score;
} ret;

// 지문 검출때까지 대기
bool waitFinger(const int wait_time, const int flag);

// 지문 추가
int add_fp(const int fp_id);
int enroll_fp();

// 전체 데이터 삭제
void deleteall_fp();

// 특정 데이터 삭제
void delete_fp(const int fp_id);

// 전체 데이터 개수 반환
int count_fp();

// 지문 검색
ret search_fp(const int flag);

bool PS_Exit();

void list_fp();