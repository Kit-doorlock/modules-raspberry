/*
  Copyright (c) 2019  Leopard-C

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*/

#ifndef __AS608_H__
#define __AS608_H__

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef unsigned char uchar;
typedef unsigned int uint;

typedef struct AS608_Module_Info {
  uint status;      // 상태 레지스터 0
  uint model;       // 센서 유형 0-15
  uint capacity;    // 지문 용량, 300
  uint secure_level;    // 보안 레벨 1/2/3/4/5, 기본값은 3
  uint packet_size;     // 데이터 패킷 크기 32/64/128/256 바이트, 기본값은 128
  uint baud_rate;       // 보레이트 계수 
  uint chip_addr;       // 장치(칩) 주소                  
  uint password;        // 통신 암호
  char product_sn[12];        // 제품 모델
  char software_version[12];  // 소프트웨어 버전
  char manufacture[12];       // 제조사 이름
  char sensor_name[12];       // 센서 이름

  uint detect_pin;      // AS608의 WAK 핀이 연결된 라즈베리 파이 GPIO 핀 번호
  uint has_password;    // 암호가 있는지 여부
} AS608;


/*******************************BEGIN**********************************
 * 전역 변수
*/
extern AS608 g_as608;
extern int   g_fd;          // 파일 디스크립터, 즉 open() 함수로 열린 시리얼의 반환 값
extern int   g_verbose;     // 상세한 정보 출력
extern char  g_error_desc[128]; // 오류 코드의 의미
extern uchar g_error_code;      // 모듈 반환 확인 코드, 함수 반환 값이 true가 아닌 경우 이 변수를 읽음
/*
**********************************END********************************/


#ifdef __cplusplus
extern "C" {
#endif

extern bool PS_Setup(uint chipAddr, uint password);       // 0x00000000 ~ 0xffffffff

extern bool PS_GetImage();
extern bool PS_GenChar(uchar bufferID);
extern bool PS_Match(int* pScore);
extern bool PS_Search(uchar bufferID, int startPageID, int count, int* pPageID, int* pScore);
extern bool PS_RegModel();
extern bool PS_StoreChar(uchar bufferID, int pageID);
extern bool PS_LoadChar(uchar bufferID, int pageID);
extern bool PS_UpChar(uchar bufferID, const char* filename);
extern bool PS_DownChar(uchar bufferID, const char* filename);
extern bool PS_UpImage(const char* filename);
extern bool PS_DownImage(const char* filename);
extern bool PS_DeleteChar(int startpageID, int count);
extern bool PS_Empty();
extern bool PS_WriteReg(int regID, int value);
extern bool PS_ReadSysPara();
extern bool PS_Enroll(int* pPageID);
extern bool PS_Identify(int* pPageID, int* pScore);
extern bool PS_SetPwd(uint passwd);   // 4 바이트 부호 없는 정수
extern bool PS_VfyPwd(uint passwd);   // 4 바이트 부호 없는 정수
extern bool PS_GetRandomCode(uint* pRandom);
extern bool PS_SetChipAddr(uint newAddr);
extern bool PS_ReadINFpage(uchar* pInfo, int size/*>=512*/);
extern bool PS_WriteNotepad(int notePageID, uchar* pContent, int contentSize);
extern bool PS_ReadNotepad(int notePageID, uchar* pContent, int contentSize);
extern bool PS_HighSpeedSearch(uchar bufferID, int startPageID, int count, int* pPageID, int* pScore);
extern bool PS_ValidTempleteNum(int* pValidN);
extern bool PS_ReadIndexTable(int* indexList, int size);

// 래퍼 함수
extern bool PS_DetectFinger();
extern bool PS_SetBaudRate(int value);
extern bool PS_SetSecureLevel(int level);
extern bool PS_SetPacketSize(int size);
extern bool PS_GetAllInfo();
extern bool PS_Flush();

// 오류 코드 g_error_code의 의미를 얻고 g_error_desc에 할당하고 반환
extern char* PS_GetErrorDesc(); 

#ifdef __cplusplus
}
#endif

#endif // __AS608_H__
