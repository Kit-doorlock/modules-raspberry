#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include "as608.h"			// 헤더파일 포함
#include "utils.h"

// 전역변수를 선언합니다.【as608.c에서 정의됨】
extern AS608 g_as608;
extern int g_fd;
extern int g_verbose;
extern char  g_error_desc[];
extern uchar g_error_code;

// ============= file로 내보낼 값들 =============
#define FINGER_DETECT_PIN 1 // wPi pinmap 기준
#define BAUD_RATE 57600
#define SERIAL "/dev/ttyAMA1"
#define ADDRESS 0xffffffff
#define PASSWORD 0x00000000
#define VERBOSE 0
#define HAS_PASSWORD 0
// =============================================

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

int main(int argc, char* argv[]) {
	int i, fp_id;
	char buffer[MAX_BUFFER];

    // 전역변수에 값 할당
    g_as608.detect_pin = FINGER_DETECT_PIN; 
    g_as608.has_password = HAS_PASSWORD;  // 비밀번호 없음
    g_verbose = VERBOSE;       // 출력정보를 최소화 합니다
    
    // wiringPi 초기화
    if (wiringPiSetup() == -1) return 1;
    
    // g_detect_pin 핀을 입력 모드로 설정합니다
    pinMode(g_as608.detect_pin, INPUT);
    
    // 시리얼 포트 열기
    if ((g_fd = serialOpen(SERIAL, BAUD_RATE)) < 0) return 2;
        
    // AS608 모듈 초기화
    if (PS_Setup(ADDRESS, PASSWORD) == 0) return 3;
    
    /****************************************/
    // do something
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
    /****************************************/
    
    // 시리얼 포트 닫기
    serialClose(g_fd);
    
    return 0;
}