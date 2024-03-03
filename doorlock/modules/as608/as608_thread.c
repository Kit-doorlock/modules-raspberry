#include "as608.h"
#include "utils.h"
#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdint.h>

typedef struct as608_thread_data {
    char serial[20];
    uint32_t baudrate;

    uint32_t address;
    uint32_t password;

    uchar detect_pin;
    uchar has_password;
    uchar verbose;
} as608_data;

#define MAX_BUFFER 21

#define WAIT "wait"
#define ADD "add"
#define ENROLL "enroll"
#define DELETEALL "deleteall"
#define DELETE "delete"
#define COUNT "count"
#define SEARCH "search"
#define LIST "list"
#define EXIT "exit"
#define E "e"

#define HSEARCH 1
#define NSEARCH 0

void controller();

void* as608_thread(void* param) {
    int* ret_value = malloc(sizeof(int));
    as608_data dat = *(as608_data*)param;

    g_as608.detect_pin = dat.detect_pin; 
    g_as608.has_password = dat.has_password;  // 비밀번호 없음
    g_verbose = dat.verbose;       // 출력정보를 최소화 합니다
    
    // g_detect_pin 핀을 입력 모드로 설정합니다
    pinMode(g_as608.detect_pin, INPUT);
    
    // 시리얼 포트 열기
    if ((g_fd = serialOpen(dat.serial, dat.baudrate)) < 0) {
        *ret_value = 2;
        pthread_exit(ret_value);
    }
        
    // AS608 모듈 초기화
    if (PS_Setup(dat.address, dat.password) == 0) {
        serialClose(g_fd);
        *ret_value = 3;
        pthread_exit(ret_value);
    }

    // ========================================================

    controller();


    // ========================================================

    serialClose(g_fd);
    *ret_value = 0;
    pthread_exit(ret_value);
}

void controller() {
    char buffer[MAX_BUFFER];
    int fp_id; // 다이얼로 조절하도록 수정 예정
    while (1) {
		printf("==== command list ====\n");
		printf("%8s %8s %8s\n", ADD, ENROLL, DELETEALL);
		printf("%8s %8s %8s\n", DELETE, COUNT, SEARCH);
		printf("%8s %8s(%8s)\n", LIST, EXIT, E);

		scanf("%s", buffer);

		if (strcmp(buffer, ADD) == 0) {
			printf("입력할 id 입력(-1 : 취소) - ");
			scanf("%d", &fp_id);
			if (fp_id == -1) continue;
			printf("pageID=%3d\n", add_fp(fp_id));
		} else if (strcmp(buffer, ENROLL) == 0) {
			// printf("pageID=%3d\n", enroll_fp());
			printf("enroll not work...\n");
		} else if (strcmp(buffer, DELETEALL) == 0) {
			deleteall_fp();
		} else if (strcmp(buffer, DELETE) == 0) {
			printf("삭제할 id 입력(-1 : 취소) - ");
			scanf("%d", &fp_id);
			if (fp_id == -1) continue;
			delete_fp(fp_id);
		} else if (strcmp(buffer, COUNT) == 0) {
			printf("stored fingerprint=%3d\n", count_fp());
		} else if (strcmp(buffer, SEARCH) == 0) {
			ret value = search_fp(HSEARCH);
			printf("pageID=%3d score=%3d\n", value.pageID, value.score);
		} else if (strcmp(buffer, LIST) == 0) {
			list_fp();	
		} else if (strcmp(buffer, EXIT) == 0 || strcmp(buffer, E) == 0) {
			break;
		}
	}
}