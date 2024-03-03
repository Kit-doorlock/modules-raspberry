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


#include "as608_module.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <wiringPi.h>
#include <wiringSerial.h>


/*******************************BEGIN**********************************
 * 전역 변수(정의)
*/
AS608 g_as608;
int   g_fd;          // 전역 변수, 파일 디스크립터, open() 함수에서 반환된 값
int   g_verbose;     // 전역 변수, 출력 정보의 상세 수준
char  g_error_desc[128]; // 전역 변수, 오류 코드 설명
uchar g_error_code;      // 전역 변수, 모듈에서 반환된 확인 코드, 함수가 true를 반환하지 않는 경우 읽을 수 있음

uchar g_order[64] = { 0 }; // 모듈로 보내는 명령 패킷
uchar g_reply[64] = { 0 }; // 모듈의 응답 패킷 

/*
**********************************END********************************/



/******************************************************************************
 *
 * 첫 번째 섹션: 보조 함수 영역
 *   이 섹션의 함수는 이 파일 내에서만 유효하며, as608.h에 선언되어 있지 않습니다!!!
 *
******************************************************************************/

/*
 * 보조 함수
 * 부호 없는 정수를 여러 개의 바이트로 분할합니다.
 *    예: num=0xa0b1c2d3, 분할=0xa0, 0xb1, 0xc2, 0xd3
*/
void Split(uint num, uchar* buf, int count) {
  for (int i = 0; i < count; ++i) {
    *buf++ = (num & 0xff << 8*(count-i-1)) >> 8*(count-i-1);
  }
}

/*
 * 보조 함수
 * 여러 개의 바이트(최대 4개)를 하나의 unsigned int 정수로 병합합니다.
 *    예: 0xa0, 0xb1, 0xc2, 0xd3을 병합하여 0xa0b1c2d3을 만듭니다.
 * 매개변수: num(포인터, 변환 결과를 저장할 곳)
 *       startAddr(포인터, 배열의 특정 요소를 가리키는 포인터, (배열 이름 + 오프셋) 형식으로 사용할 수 있음)
 *       count(startAddr에서 시작하여 count개의 숫자를 합쳐 num에 할당)
*/
bool Merge(uint* num, const uchar* startAddr, int count) {
  *num = 0;
  for (int i = 0; i < count; ++i)
    *num += (int)(startAddr[i]) << (8*(count-i-1)); 

  return true;
}

/* 
 *  보조 함수, 디버그용
 *  16진수 데이터를 출력합니다.
*/
void PrintBuf(const uchar* buf, int size) {
  for (int i = 0; i < size; ++i) {
    printf("%02X ", buf[i]);
  }
  printf("\n");
}

/*
 * 보조 함수
 * 체크섬을 계산합니다. (buf의 7번 바이트부터 시작하여 뒤에서 3번째 바이트까지의 모든 바이트를 계산)
**/
int Calibrate(const uchar* buf, int size) {
  int count = 0;
  for (int i = 6; i < size - 2; ++i) {
    count += buf[i];
  }

  return count;
}

/*
 * 보조 함수
 * 확인 및 체크섬을 확인합니다.
 * 매개변수: buf, 모듈의 응답 패킷 데이터
 *     size, 응답 패킷의 유효한 바이트 수
*/
bool Check(const uchar* buf, int size) {
  int count_ = 0;       // 모듈로부터 받은 체크섬 
  Merge(&count_, buf+size-2, 2); 

  // 직접 계산한 체크섬
  int count = Calibrate(buf, size);

  return (buf[9] == 0x00 && 
          count_ == count && 
          buf[0] != 0x00);   // 0x00만 포함되는 경우 방지
}

/* 
 *  보조 함수
 *  명령 패킷을 전송합니다.
 *  매개변수, size(실제로 전송할 유효한 문자, '\0'을 포함하지 않음.  ！！！)
*/
int SendOrder(const uchar* order, int size) {
  // 자세한 정보 출력
  if (g_verbose == 1) {
    printf("sent: ");
    PrintBuf(order, size);
  }
  int ret = write(g_fd, order, size);
  return ret;
}

/* 
 *  보조 함수
 *  응답 패킷을 수신합니다.
 *  매개변수: size(수신할 데이터의 실제 크기, 헤더, 패킷 길이, 데이터 영역, 체크섬 등을 포함)
 */
bool RecvReply(uchar* hex, int size) {
  int availCount = 0;
  int timeCount  = 0;

  int avail = 0;

  while (true) {
    avail = serialDataAvail(g_fd);
    if (avail) {
      hex[availCount] = serialGetchar(g_fd);
      availCount++;
      if (availCount >= size) {
        break;
      }
    }
    usleep(10); // 10 마이크로초 대기
    timeCount++;
    if (timeCount > 300000) {   // 최대 3초까지 대기
      break;
    }
  }

  // 자세한 정보 출력
  if (g_verbose == 1) {
    printf("recv: ");
    PrintBuf(hex, availCount);
  }

  // 최대 블록 시간 내에 지정된 크기의 데이터를 수신하지 못하면 false 반환
  if (availCount < size) {
    g_error_code = 0xff;
    return false;
  }

  g_error_code = hex[9];
  return true;
}

/*
 * 진행 상황 표시
 * 매개변수: done(완료된 양)
 *          all(총 양)
*/
void PrintProcess(int done, int all) {
  // 프로그레스 바, 0~100%, 50개 문자
  char process[64] = { 0 };
  double stepSize = (double)all / 100;
  int doneStep = done / stepSize;
  
  // 단계가 짝수이면 프로그레스 바 업데이트
  // 100단계의 진행을 50개 문자로 표시
  for (int i = 0; i < doneStep / 2 - 1; ++i)
    process[i] = '=';
  process[doneStep/2 - 1] = '>';

  printf("\rProcess:[%-50s]%d%% ", process, doneStep);
  if (done < all)
    fflush(stdout);
  else
    printf("\n");
}

/* 
 *  보조 함수
 *  패킷을 수신합니다. 확인 코드 0x02는 데이터 패킷 및 후속 패킷이 있음을 의미하며, 0x08은 마지막 패킷임을 의미합니다.
 *  매개변수：
 *     validDataSize: 유효한 데이터 크기, 헤더, 길이 패킷, 데이터 영역, 체크섬 등을 포함
*/
bool RecvPacket(uchar* pData, int validDataSize) {
  if (g_as608.packet_size <= 0)
    return false;
  int realPacketSize = 11 + g_as608.packet_size; // 각 데이터 패킷의 실제 크기
  int realDataSize = validDataSize * realPacketSize / g_as608.packet_size;  // 총 수신해야 하는 데이터 크기

  uchar readBufTmp[8] = { 0 };  // 한 번에 최대 8바이트까지 읽고 readBuf에 추가
  uchar* readBuf = (uchar*)malloc(realPacketSize); // realPacketSize 바이트만큼 읽을 때 완전한 데이터 패킷을 받았다고 판단하고 pData에 추가

  int availSize      = 0;
  int readSize       = 0;
  int readCount      = 0;
  int readBufTmpSize = 0;
  int readBufSize    = 0;
  int offset         = 0;
  int timeCount      = 0;
  
  while (true) {
    if ((availSize = serialDataAvail(g_fd)) > 0) {
      timeCount = 0;
      if (availSize > 8) {
        availSize = 8;
      }
      if (readBufSize + availSize > realPacketSize) {
        availSize = realPacketSize - readBufSize;
      }

      memset(readBufTmp, 0, 8);
      readSize = read(g_fd, readBufTmp, availSize);
      memcpy(readBuf+readBufSize, readBufTmp, readSize);

      readBufSize += readSize;
      readCount   += readSize;

      // 자세한 정보를 출력할지 여부
      if (g_verbose == 1) {
        printf("%2d%% RecvData: %d  count=%4d/%-4d  ", 
            (int)((double)readCount/realDataSize*100), readSize, readCount, realDataSize);
        PrintBuf(readBufTmp, readSize);
      }
      else if (g_verbose == 0){ // 기본적으로 진행 표시
        PrintProcess(readCount, realDataSize);
      }
      else {
        // show nothing
      }

      // 완전한 데이터 패킷(139 바이트)을 수신한 경우
      if (readBufSize >= realPacketSize) {
        int count_ = 0;
        Merge(&count_, readBuf+realPacketSize-2, 2);
        if (Calibrate(readBuf, realPacketSize) != count_) {
          free(readBuf);
          g_error_code = 0x01;
          return false;
        }

        memcpy(pData+offset, readBuf+9, g_as608.packet_size);
        offset += g_as608.packet_size;
        readBufSize = 0;

        // 종료 패킷을 받음
        if (readBuf[6] == 0x08) {
          break;
        }
      }

      // validDataSize 바이트의 유효한 데이터를 수신하였지만 종료 패킷을 아직 수신하지 않은 경우
      if (readCount >= realDataSize) {
        free(readBuf);
        g_error_code = 0xC4;
        return false;
      }
    } // end outer if

    usleep(10); // 10 마이크로초 대기
    timeCount++;
    if (timeCount > 300000) {   // 최대 3초까지 대기
      break;
    }
  } // end while

  free(readBuf);

  // 최대 블록 시간 내에 지정된 크기의 데이터를 수신하지 못하면 false 반환
  if (readCount < realDataSize) {
    g_error_code = 0xC3;
    return false;
  }
  
  g_error_code = 0x00;
  return true; 
}


/* 
 *  보조 함수
 *  데이터 패킷을 보냅니다. 확인 코드 0x02는 데이터 패킷이며 후속 패킷이 있음을 나타내고, 0x08은 마지막 데이터 패킷임을 나타냅니다.
 *  매개변수:
 *     pData: 유효한 데이터 크기를 나타냅니다. 헤더, 체크섬 및 일부를 제외한 유효한 데이터 크기입니다.
 *     validDataSize: 실제 데이터 크기입니다. 패킷 헤더와 함께 계산됩니다.
*/
bool SendPacket(uchar* pData, int validDataSize) {
  if (g_as608.packet_size <= 0)
    return false;
  if (validDataSize % g_as608.packet_size != 0) {
    g_error_code = 0xC8;
    return false;
  }
  int realPacketSize = 11 + g_as608.packet_size; // 각 데이터 패킷의 실제 크기
  int realDataSize = validDataSize * realPacketSize / g_as608.packet_size;  // 전체 보내야 하는 데이터 크기

  // 데이터 패킷 생성
  uchar* writeBuf = (uchar*)malloc(realPacketSize);
  writeBuf[0] = 0xef;  // 패킷 헤더
  writeBuf[1] = 0x01;  // 패킷 헤더
  Split(g_as608.chip_addr, writeBuf+2, 4);  // 칩 주소
  Split(g_as608.packet_size+2, writeBuf+7, 2);  // 패킷 길이

  int offset     = 0;  // 전송된 유효한 데이터
  int writeCount = 0;  // 실제로 전송된 데이터 크기

  while (true) {
    // 데이터 영역 채우기
    memcpy(writeBuf+9, pData+offset, g_as608.packet_size);

    // 데이터 패킷 플래그
    if (offset + g_as608.packet_size < validDataSize)
      writeBuf[6] = 0x02;  // 종료 패킷(마지막 데이터 패킷)
    else
      writeBuf[6] = 0x08;  // 일반 데이터 패킷

    // 체크섬
    Split(Calibrate(writeBuf, realPacketSize), writeBuf+realPacketSize-2, 2); 

    // 데이터 패킷 전송
    write(g_fd, writeBuf, realPacketSize);

    offset     += g_as608.packet_size;
    writeCount += realPacketSize;

    // 자세한 정보 출력 여부 확인
    if (g_verbose == 1) {
      printf("%2d%% SentData: %d  count=%4d/%-4d  ", 
          (int)((double)writeCount/realDataSize*100), realPacketSize, writeCount, realDataSize);
      PrintBuf(writeBuf, realPacketSize);
    }
    else if (g_verbose == 0) {
      // 진행률 표시
      PrintProcess(writeCount, realDataSize);
    }
    else {
      // show nothing
    }

    if (offset >= validDataSize)
      break;
  } // end while

  free(writeBuf);
  g_error_code = 0x00;
  return true; 
}

/*
 * 보조 함수
 * 명령 패킷을 생성하고 결과를 전역 변수 g_order에 할당합니다.
 * 매개변수:
 *   orderCode: 명령 코드, 예: 0x01, 0x02, 0x1a...
 *   fmt: 매개변수 설명, 예: 명령 패킷에 2개의 매개변수가 있으며 하나는 uchar 형식이고 1바이트이고 다른 하나는 uchar* 형식이고 32바이트인 경우 fmt은 "%1d%32s"여야 합니다.
 *      (숫자는 해당 매개변수의 바이트 수를 나타내며 1이면 생략 가능하며 문자는 형식을 나타냅니다. %d, %u 및 %s만 지원됩니다.)
*/
int GenOrder(uchar orderCode, const char* fmt, ...) {
  g_order[0] = 0xef;        // 패킷 헤더, 0xef
  g_order[1] = 0x01;
  Split(g_as608.chip_addr, g_order+2, 4);    // 칩 주소, PS_Setup()에서 초기 설정해야 함
  g_order[6] = 0x01;        // 패킷 플래그, 0x01은 명령 패킷, 0x02는 데이터 패킷, 0x08은 종료 패킷(마지막 데이터 패킷)
  g_order[9] = orderCode;   // 명령

  // 전체 매개변수 수 계산
  int count = 0;
  for (const char* p = fmt; *p; ++p) {
    if (*p == '%')
      count++;
  }

  // fmt==""
  if (count == 0) { 
    Split(0x03, g_order+7,  2);  // 패킷 길이
    Split(Calibrate(g_order, 0x0c), g_order+10, 2);  // 체크섬(매개변수가 없는 경우 명령 패킷 길이는 12이므로 0x0c)
    return 0x0c;
  }
  else {
    va_list ap;
    va_start(ap, fmt);

    uint  uintVal;
    uchar ucharVal;
    uchar* strVal;

    int offset = 10;  // g_order 포인터 오프셋
    int width = 1;    // fmt에서 너비 지정자, 예: %4d, %32s

    // 가변 매개변수 처리
    for (; *fmt; ++fmt) {
      width = 1;
      if (*fmt == '%') {
        const char* tmp = fmt+1;

        // 너비 가져오기, 예: %4u, %32s
        if (*tmp >= '0' && *tmp <= '9') {
          width = 0;
          do {
            width = (*tmp - '0') + width * 10;
            tmp++;
          } while(*tmp >= '0' && *tmp <= '9');
        }

        switch (*tmp) {
          case 'u':
          case 'd':
            if (width > 4)
              return 0;
            uintVal = va_arg(ap, int);
            Split(uintVal, g_order+offset, width);
            break;
          case 'c': // "%d"와 동일
            if (width > 1)
              return 0;
            ucharVal = va_arg(ap, int);
            g_order[offset] = ucharVal;
            break;
          case 's':
            strVal = va_arg(ap, char*);
            memcpy(g_order+offset, strVal, width);
            break;
          default:
            return 0;
        } // end switch 

        offset += width;
      } // end if (*p == '%')
    } // end for 

    Split(offset+2-9, g_order+7, 2);  // 패킷 길이
    Split(Calibrate(g_order, offset+2), g_order+offset, 2); // 체크섬
    
    va_end(ap);
    return offset + 2;
  } // end else (count != 0)
}

/***************************************************************************
 *
 * 두 번째 부분:
 *   as608.h에서 선언된 함수
 *
***************************************************************************/

/*
 * 초기 설정
 * 칩 주소와 통신 비밀번호를 설정합니다. 기본값은 각각 0xffffffff와 0x00000000으로 칩 주소와 비밀번호가 없음을 의미합니다.
 *   칩 주소를 변경하지는 않습니다.
*/
bool PS_Setup(uint chipAddr, uint password) {
  g_as608.chip_addr = chipAddr;
  g_as608.password  = password;

  if (g_verbose == 1)
    printf("-------------------------Initializing-------------------------\n");
  // 비밀번호 확인
  if (g_as608.has_password) {
    if (!PS_VfyPwd(password))
     return false;
  }

  // 데이터 패킷 크기, 보레이트 등을 가져옴
  if (PS_ReadSysPara() && g_as608.packet_size > 0) {
    if (g_verbose == 1)
      printf("-----------------------------Done-----------------------------\n");
    return true;
  }

  if (g_verbose == 1)
    printf("-----------------------------Done-----------------------------\n");
  g_error_code = 0xC7;
  return false;
}

/*
 * 함수명: PS_GetImage
 * 기능: 손가락을 감지하고 감지되면 지문 이미지를 ImageBuffer에 저장합니다. 결과는 성공 또는 손가락 없음을 나타내는 확인 코드로 반환됩니다.
 * 매개변수: 없음
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 이미지 캡처 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=02H는 센서 상에 손가락이 없음을 나타냅니다.
 *   확인 코드=03H는 이미지 캡처 실패를 나타냅니다.
*/
bool PS_GetImage() {
  int size = GenOrder(0x01, "");

  // 손가락 감지 여부 확인
  //for (int i = 0; i < 100000; ++i) {
  //  if (digitalRead(g_as608.detect_pin) == HIGH) {
  //    delay(1000);
  //    printf("Sending order.. \n");
  
  // 명령 패킷 전송
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && Check(g_reply, 12));

  //}
  // 100*100000=10e6 마이크로초, 즉 10초
  // 10초 동안 지문을 감지하지 못하면 false를 반환합니다.
  //  usleep(100);
  // }

  //g_error_code = 0x02;  // 센서 상에 손가락이 없음
  //return false;
}


/*
 * 함수명: PS_GenChar
 * 기능: ImageBuffer의 원시 이미지를 사용하여 지문 특징 파일을 CharBuffer1 또는 CharBuffer2에 생성합니다.
 * 매개변수: bufferID(버퍼 ID)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 특징 생성 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=06H는 지문 이미지가 지저분하여 특징 생성에 실패했음을 나타냅니다.
*/
bool PS_GenChar(uchar bufferID) {
  int size = GenOrder(0x02, "%d", bufferID);
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && Check(g_reply, 12));
}


/*
 * 함수명: PS_Match
 * 기능: CharBuffer1 또는 CharBuffer2의 특징 파일을 정확하게 비교합니다.
 * 매개변수: score(점수 비교 결과를 가리키는 포인터)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 지문 일치를 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=08H는 지문이 일치하지 않음을 나타냅니다.
*/
bool PS_Match(int* pScore) {
  int size = GenOrder(0x03, "");
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 14) && 
          Check(g_reply, 14) &&
          Merge(pScore, g_reply+10, 2));
}


/* 
 * 함수명: PS_Search
 * 기능: CharBuffer1 또는 CharBuffer2의 특징 파일을 전체 또는 일부 지문 데이터베이스에서 검색합니다.
 * 매개변수: bufferID(버퍼 ID)
 *           pageID(검색 결과를 가리키는 포인터)
 *           score(검색 결과에 해당하는 점수를 가리키는 포인터)
 *           startPageID(시작 페이지 ID)
 *           count(페이지 수)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 검색 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=09H는 검색 결과를 찾지 못했음을 나타냅니다. 이때 페이지 ID 및 점수는 0입니다.
*/
bool PS_Search(uchar bufferID, int startPageID, int count, int* pPageID, int* pScore) {
  int size = GenOrder(0x04, "%d%2d%2d", bufferID, startPageID, count);
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 16) && 
           Check(g_reply, 16) && 
           (Merge(pPageID, g_reply+10, 2)) &&  // 페이지 ID에 값을 할당하고 true 반환
           (Merge(pScore,  g_reply+12, 2))     // 점수에 값을 할당하고 true 반환
        );
}

/*
 * 함수명: PS_RegModel
 * 기능: CharBuffer1 또는 CharBuffer2의 특징 파일을 병합하여 템플릿을 생성하고 CharBuffer1 및 CharBuffer2에 저장합니다.
 * 매개변수: 없음
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 병합 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=0aH는 병합 실패(두 개의 지문이 동일한 손가락에 속하지 않음)를 나타냅니다.
*/
bool PS_RegModel() {
  int size = GenOrder(0x05, "");
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) &&
      Check(g_reply, 12));
}


/*
 * 함수명: PS_StoreChar
 * 기능: CharBuffer1 또는 CharBuffer2의 특징 파일을 지정한 PageID 위치의 플래시 데이터베이스에 저장합니다.
 * 매개변수: BufferID(버퍼 ID)
 *          PageID(지문 데이터베이스 위치)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 저장 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=0bH는 PageID가 지문 데이터베이스 범위를 벗어남을 나타냅니다.
 *   확인 코드=18H는 플래시 쓰기 오류를 나타냅니다.
*/
bool PS_StoreChar(uchar bufferID, int pageID) {
  int size = GenOrder(0x06, "%d%2d", bufferID, pageID);
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && 
        Check(g_reply, 12));
}


/*
 * 함수명: PS_LoadChar
 * 기능: 플래시 데이터베이스에서 지정한 ID 번호의 지문 템플릿을 읽어서 CharBuffer1 또는 CharBuffer2에로드합니다.
 * 매개변수: BufferID(버퍼 ID)
 *          PageID(지문 데이터베이스 템플릿 번호)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 읽기 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=0cH는 읽기 오류 또는 무효한 템플릿을 나타냅니다.
 *   확인 코드=0BH는 PageID가 지문 데이터베이스 범위를 벗어남을 나타냅니다.
*/
bool PS_LoadChar(uchar bufferID, int pageID) {
  int size = GenOrder(0x07, "%d%2d", bufferID, pageID);
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) &&
         Check(g_reply, 12));
}

/*
 * 함수명: PS_UpChar
 * 기능: 특징 버퍼의 특징 파일을 상위 장치에 업로드합니다 (지문 인식 모듈에서 라즈베리 파이로 다운로드).
 * 매개변수: bufferID(버퍼 ID)
 *          filename(파일 이름)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 데이터 패킷을 뒤따르도록 합니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=0dH는 명령 실행 실패를 나타냅니다.
*/
bool PS_UpChar(uchar bufferID, const char* filename) {
  int size = GenOrder(0x08, "%d", bufferID);
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  if (!(RecvReply(g_reply, 12) && Check(g_reply, 12))) {
    return false;
  }

  // 데이터 패킷을 수신하고 유효한 데이터를 pData에 저장
  uchar pData[768] = { 0 };
  if (!RecvPacket(pData, 768)) {
    return false;
  }

  // 파일 작성
  FILE* fp = fopen(filename, "w+");
  if (!fp) { 
    g_error_code = 0xC2;
    return false;
  }

  fwrite(pData, 1, 768, fp);
  fclose(fp);

  return true;
}


/*
 * 함수명: PS_DownChar
 * 기능: 상위 장치에서 특징 파일을 모듈의 특징 버퍼에 다운로드합니다 (라즈베리 파이에서 지문 인식 모듈로 업로드).
 * 매개변수: bufferID(버퍼 ID)
 *          filename(파일 이름)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 후속 데이터 패킷을 수신합니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=0eH는 후속 데이터 패킷을 수신할 수 없습니다.
*/
bool PS_DownChar(uchar bufferID, const char* filename) {
  // 명령을 전송
  int size = GenOrder(0x09, "%d", bufferID);
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드를 확인하여 후속 데이터 패킷을 전송할 수 있는지 확인
  if ( !(RecvReply(g_reply, 12) && Check(g_reply, 12)) )
    return false;

  // 로컬 파일 열기
  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    g_error_code = 0xC2;
    return false;
  }

  // 파일 크기 가져오기
  int fileSize = 0;
  fseek(fp, 0, SEEK_END);
  fileSize = ftell(fp);
  rewind(fp);
  if (fileSize != 768) {
    g_error_code = 0x09;
    fclose(fp);
    return false;
  }

  // 파일 내용 읽기
  uchar charBuf[768] = { 0 };
  fread(charBuf, 1, 768, fp);

  fclose(fp);

  // 데이터 패킷 전송
  return SendPacket(charBuf, 768);
}


/*
 * 함수명: PS_UpImage
 * 기능: 이미지 버퍼의 데이터를 라즈베리 파이에 업로드합니다 (이미지 다운로드).
 * 매개변수: filename(저장할 파일 이름, .bmp 형식)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *  확인 코드=00H는 후속 데이터 패킷을 전송합니다.
 *  확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *  확인 코드=0fH는 후속 데이터 패킷을 전송할 수 없습니다.
*/
bool PS_UpImage(const char* filename) {
  int size = GenOrder(0x0a, "");
  SendOrder(g_order, size);

  // 응답 패킷 수신 및 확인 코드 및 체크섬 확인
  if (!(RecvReply(g_reply, 12) && Check(g_reply, 12))) {
    return false;
  }

  // 데이터 패킷 수신 및 pData에 유효한 데이터 저장
  // 이미지 크기 128*288 = 36864
  uchar* pData = (uchar*)malloc(36864);
  if (!RecvPacket(pData, 36864)) {
    return false;
  }

  // pData를 파일에 쓰기
  FILE* fp = fopen(filename, "w+");
  if (!fp) {
    g_error_code = 0xC2;
    return false;
  }

  // BMP 헤더 작성 (모듈에 대해 고정된 값)
  uchar header[54] = "\x42\x4d\x00\x00\x00\x00\x00\x00\x00\x00\x36\x04\x00\x00\x28\x00\x00\x00\x00\x01\x00\x00\x20\x01\x00\x00\x01\x00\x08";
  for (int i = 29; i < 54; ++i)
    header[i] = 0x00;
  fwrite(header, 1, 54, fp);

  // 팔레트
  uchar palette[1024] = { 0 };
  for (int i = 0; i < 256; ++i) {
    palette[4*i]   = i;
    palette[4*i+1] = i;
    palette[4*i+2] = i;
    palette[4*i+3] = 0;
  }
  fwrite(palette, 1, 1024, fp);

  // BMP 픽셀 데이터
  uchar* pBody = (uchar*)malloc(73728);
  for (int i = 0; i < 73728; i += 2) {
    pBody[i] = pData[i/2] & 0xf0;  
  }
  for (int i = 1; i < 73728; i += 2) {
    pBody[i] = (pData[i/2] & 0x0f) << 4;
  }

  fwrite(pBody, 1, 73728, fp);

  free(pBody);
  free(pData);
  fclose(fp);

  return true; 
}


/*
 * 함수명: PS_DownImage
 * 기능: 상위 장치에서 이미지 데이터를 모듈에 다운로드합니다 (AS608 모듈에 이미지 업로드).
 * 매개변수: 로컬 지문 이미지 파일 이름
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 후속 데이터 패킷을 수신합니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=0eH는 후속 데이터 패킷을 수신할 수 없습니다.
*/
bool PS_DownImage(const char* filename) {
  int size = GenOrder(0x0b, "");
  SendOrder(g_order, size);

  if (!RecvReply(g_reply, 12) && Check(g_reply, 12))
    return false;

  // 지문 이미지 파일 크기: 748069 바이트
  uchar imageBuf[74806] = { 0 };

  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    g_error_code = 0xC2;
    return false;
  }

  // 이미지 파일 크기 가져오기
  int imageSize = 0;
  fseek(fp, 0, SEEK_END);
  imageSize = ftell(fp);
  rewind(fp);
  if (imageSize != 74806) { // 지문 이미지 크기는 74806KB여야 합니다.
    g_error_code = 0xC9;
    fclose(fp);
    return false;
  }

  // 파일 읽기
  if (fread(imageBuf, 1, 74806, fp) != 74806) {
    g_error_code = 0xCA;
    fclose(fp);
    return false;
  }
  fclose(fp);

  //FILE* fpx = fopen("temp.bmp", "wb");
  //if (!fpx) {
  //  printf("Error\n");
  //  return false;
  //}
  //fwrite(imageBuf, 1, 74806, fpx);
  //fclose(fpx);
  //return true;

  //uchar dataBuf[128*288] = { 0 };
  //for (uint i = 0, size=128*288; i < size; ++i) {
  //  dataBuf[i] = imageBuf[1078 + i*2] + (imageBuf[1078 + i*2 + 1] >> 4);
  //}

  // 이미지의 픽셀 데이터를 보내기 위해 오프셋 54+1024=1078 사용, 크기 128*256*2=73728
  //return SendPacket(dataBuf, 128*288);
  return SendPacket(imageBuf+1078, 73728);
}


/*
 * 함수명: PS_DeleteChar
 * 기능: 플래시 데이터베이스에서 지정된 ID부터 N개의 지문 템플릿을 삭제합니다.
 * 매개변수: startPageID(지문 라이브러리 템플릿 시작 번호)
 *          count(삭제할 템플릿 개수)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 템플릿 삭제 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=10H는 템플릿 삭제 실패를 나타냅니다.
*/
bool PS_DeleteChar(int startPageID, int count) {
  int size = GenOrder(0x0c, "%2d%2d", startPageID, count);
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) &&
         Check(g_reply, 12));
}

/*
 * 함수명: PS_Empty
 * 기능: 플래시 데이터베이스에서 모든 지문 템플릿을 삭제합니다.
 * 매개변수: 없음
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 삭제 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=11H는 삭제 실패를 나타냅니다.
*/
bool PS_Empty() {
  int size = GenOrder(0x0d, "");
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && Check(g_reply, 12));
}

/*
 * 함수명: PS_WriteReg
 * 기능: 모듈 레지스터에 쓰기 작업을 수행합니다.
 * 매개변수: 없음(매개변수는 g_as608.chip_addr, g_as608.packet_size, PS_BPS 등의 시스템 변수에 저장됩니다.)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 OK를 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=1aH는 레지스터 번호가 잘못되었음을 나타냅니다.
*/
bool PS_WriteReg(int regID, int value) {
  if (regID != 4 && regID != 5 && regID != 6) {
    g_error_code = 0x1a;
    return false;
  }

  int size = GenOrder(0x0e, "%d%d", regID, value);
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && Check(g_reply, 12));
}

/*
 * 함수명: PS_ReadSysPara
 * 기능: 모듈의 기본 매개변수 (통신 속도, 패킷 크기 등)를 읽습니다.
 * 매개변수: 없음(매개변수는 g_as608.chip_addr, g_as608.packet_size, PS_BPS 등의 시스템 변수에 저장됩니다.)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 OK를 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
*/
bool PS_ReadSysPara() {
  int size = GenOrder(0x0f, "");
  SendOrder(g_order, size);
  
  return (RecvReply(g_reply, 28) &&
          Check(g_reply, 28) &&
          Merge(&g_as608.status,       g_reply+10, 2) &&
          Merge(&g_as608.model,        g_reply+12, 2) && 
          Merge(&g_as608.capacity,     g_reply+14, 2) &&
          Merge(&g_as608.secure_level, g_reply+16, 2) &&
          Merge(&g_as608.chip_addr,    g_reply+18, 4) &&
          Merge(&g_as608.packet_size,  g_reply+22, 2) &&
          Merge(&g_as608.baud_rate,    g_reply+24, 2) &&
          (g_as608.packet_size = 32 * (int)pow(2, g_as608.packet_size)) &&
          (g_as608.baud_rate *= 9600)
         );
}

/*
 * 함수명: PS_Enroll
 * 기능: 한 번의 지문 등록 템플릿을 취합하고 지문 라이브러리에서 빈 공간을 찾아 저장하며 저장된 ID를 반환합니다.
 * 매개변수: pPageID(저장된 ID 반환)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 등록 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=1eH는 등록 실패를 나타냅니다.
*/
bool PS_Enroll(int* pPageID) {
  int size = GenOrder(0x10, "");
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 14) &&
          Check(g_reply, 14) &&
          Merge(pPageID, g_reply+10, 2)
         );
}

/*
 * 함수명: PS_Identify
 * 기능: 1. 자동 지문 취합 후 지문 라이브러리에서 대상 템플릿을 찾아 검색 결과를 반환합니다.
 *      2. 대상 템플릿이 현재 취합된 지문과 매치 점수가 가장 높으며 대상 템플릿이
 *         불완전한 경우 취합된 지문의 빈 공간을 업데이트합니다.
 * 매개변수: pPageID(저장된 ID 반환)
 *          pScore(매치 점수 반환)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 검색 결과가 있음을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=09H는 검색 결과가 없음을 나타냅니다. 이때 페이지 번호와 점수는 0입니다.
*/
bool PS_Identify(int* pPageID, int* pScore) { 
  int size = GenOrder(0x11, "");
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 16) &&
          Check(g_reply, 16) &&
          Merge(pPageID, g_reply+10, 2) &&
          Merge(pScore,  g_reply+12, 2)
         );
}

/*
 * 함수명: PS_SetPwd
 * 기능: 모듈의 핸드셰이크 암호를 설정합니다.
 * 매개변수: 암호(pwd)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 OK를 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
*/
bool PS_SetPwd(uint pwd) {   // 0x00 ~ 0xffffffff
  int size  = GenOrder(0x12, "%4d", pwd);
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && 
          Check(g_reply, 12) &&
          (g_as608.has_password = 1) &&
          ((g_as608.password = pwd) || true)); // pwd=0x00 방지
}


/*
 * 함수명: PS_VfyPwd
 * 기능: 모듈의 핸드셰이크 암호를 검증합니다.
 * 매개변수: 암호(pwd) 0x00 ~ 0xffffffff
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 암호 확인이 올바름을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=13H는 암호가 올바르지 않음을 나타냅니다.
*/
bool PS_VfyPwd(uint pwd) { 
  int size = GenOrder(0x13, "%4d", pwd); 
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && Check(g_reply, 12));
}

/*
 * 함수명: PS_GetRandomCode
 * 기능: 칩에 무작위 숫자를 생성하도록 합니다.
 * 매개변수: 없음
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 생성 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
*/
bool PS_GetRandomCode(uint* pRandom) {
  int size = GenOrder(0x14, "");
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 16) &&
          Check(g_reply, 16) &&
          Merge(pRandom, g_reply+10, 4)
         );
}

/*
 * 함수명: PS_SetChipAddr
 * 기능: 칩의 주소를 설정합니다.
 * 매개변수: 주소(addr) - 새로운 주소
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 주소 생성 성공을 나타냅니다.
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 * 비고:
 *   상위 장치에서 명령 패킷을 아래와 같이 보낼 때 칩의 주소는 기본 주소인 0xffffffff를 사용하며 응답 패킷의 주소 필드는 새로 생성된 주소로 설정됩니다.
 *   이 명령을 실행한 후 칩의 주소는 고정되며 변경되지 않습니다. FLASH를 지우면 주소를 변경할 수 있습니다.
 *   이 명령을 실행한 후 모든 데이터 패킷은 새로 생성된 주소를 사용해야 합니다.
*/
bool PS_SetChipAddr(uint addr) {
  int size = GenOrder(0x15, "%4d", addr);
  SendOrder(g_order, size);

  // 데이터 수신 및 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 12) && 
          Check(g_reply, 12) && 
          ((g_as608.chip_addr = addr) || true)); // addr=0x00 방지
}

/*
 * 함수명: PS_ReadINFpage
 * 기능: FLASH 정보 페이지에 저장된 정보(512바이트)를 읽어옵니다.
 * 매개변수: pInfo
 *          pInfoSize(배열 크기)
 * 반환 값: true(성공), false(오류 발생), 확인 코드는 g_error_code에 할당됩니다.
 *    확인 코드=00H는 데이터 패킷을 뒤따르도록 나타냅니다.
 *    확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *    확인 코드=0dH는 명령 실행 실패를 나타냅니다.
*/
bool PS_ReadINFpage(uchar* pInfo, int pInfoSize/*>=512*/) {
  if (pInfoSize < 512) {
    g_error_code = 0xC1;
    return false;
  }

  int size = GenOrder(0x16, "");
  SendOrder(g_order, size);

  // 응답 패킷 수신
  if (!(RecvReply(g_reply, 12) && Check(g_reply, 12))) 
    return false;

  // 데이터 패킷 수신
  if (!RecvPacket(pInfo, 512))
    return false;
  
  memcpy(g_as608.product_sn,       pInfo+28, 8);
  memcpy(g_as608.software_version, pInfo+36, 8);
  memcpy(g_as608.manufacture,      pInfo+44, 8);
  memcpy(g_as608.sensor_name,      pInfo+52, 8);

  return true;
}

/*
 * 함수명: PS_WriteNotepad
 * 설명: 사용자 메모장에 32바이트 이하의 데이터를 쓰기 위해 사용됩니다.
 * 매개변수: notePageID(페이지 번호)
 *          pContent(데이터를 저장할 내용)
 *          contentSize(32바이트 이하)
 * 반환값: true(성공), false(오류 발생), 오류 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 성공을 나타내며;
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 */
bool PS_WriteNotepad(int notePageID, uchar* pContent, int contentSize) {
  if (contentSize > 32) {
    g_error_code = 0xC6;
    return false;
  }

  pContent[32] = 0; // 문자열 종료 문자
  int size = GenOrder(0x18, "%d%32s", notePageID, pContent);
  SendOrder(g_order, size);

  return (RecvReply(g_reply, 12) && Check(g_reply, 12));
}

/*
 * 함수명: PS_ReadNotepad
 * 설명: FLASH 사용자 영역에서 128바이트 데이터를 읽기 위해 사용됩니다.
 * 매개변수: notePageID(페이지 번호)
 *          pContent(읽어온 내용을 저장할 버퍼)
 *          contentSize(32바이트 이상)
 * 반환값: true(성공), false(오류 발생), 오류 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 성공을 나타내며;
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 */
bool PS_ReadNotepad(int notePageID, uchar* pContent, int contentSize) {
  if (contentSize < 32) {
    g_error_code = 0xC1;
    return false;
  }

  int size = GenOrder(0x19, "%d", notePageID);
  SendOrder(g_order, size);

  // 응답 패킷 수신, 확인 코드 및 체크섬 확인
  if (!(RecvReply(g_reply, 44) && Check(g_reply, 44)))
    return false;

  memcpy(pContent, g_reply+10, 32);
  return true;
}

/*
 * 함수명: PS_HighSpeedSearch
 * 설명: CharBuffer1 또는 CharBuffer2에서 특징 파일을 사용하여 전체 또는 일부 지문 라이브러리를 빠르게 검색합니다. 검색되면 페이지 번호를 반환합니다.
 *       이 명령은 지문 라이브러리에 실제로 존재하며 등록 시에 품질이 좋은 지문에 대해서는 빠른 결과를 제공합니다.
 * 매개변수: bufferID(특징 버퍼 ID)
 *          startPageID(시작 페이지 번호)
 *          count(페이지 수)
 *          pPageID(결과 페이지 번호를 저장할 포인터)
 *          pScore(결과에 해당하는 점수를 저장할 포인터)
 * 반환값: true(성공), false(오류 발생), 오류 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 검색 성공을 나타내며;
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 *   확인 코드=09H는 검색 실패를 나타내며, 이때 페이지 번호와 점수는 0입니다.
 */
bool PS_HighSpeedSearch(uchar bufferID, int startPageID, int count, int* pPageID, int* pScore) {
  int size = GenOrder(0x1b, "%d%2d%2d", bufferID, startPageID, count);
  SendOrder(g_order, size);

  // 데이터 수신, 확인 코드 및 체크섬 확인
  return ( RecvReply(g_reply, 16) && 
           Check(g_reply, 16) && 
           (Merge(pPageID, g_reply+10, 2)) &&  // 페이지 ID 할당, true 반환
           (Merge(pScore,  g_reply+12, 2))     // 점수 할당, true 반환
        );
}

/*
 * 함수명: PS_ValidTempleteNum
 * 설명: 유효한 템플릿 개수를 읽어옵니다.
 * 매개변수: pValidN(유효한 템플릿 개수를 저장할 포인터)
 * 반환값: true(성공), false(오류 발생), 오류 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 성공적으로 읽었음을 나타내며;
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 */
bool PS_ValidTempleteNum(int* pValidN) {
  int size = GenOrder(0x1d, "");
  SendOrder(g_order, size);

  // 데이터 수신, 확인 코드 및 체크섬 확인
  return (RecvReply(g_reply, 14) &&
          Check(g_reply, 14) &&
          Merge(pValidN, g_reply+10, 2)
         );
}

/*
 * 함수명: PS_ReadIndexTable
 * 설명: 등록된 템플릿의 인덱스 테이블을 읽어옵니다.
 * 매개변수: indexList(인덱스를 저장할 배열), size(배열 크기)
 * 반환값: true(성공), false(오류 발생), 오류 코드는 g_error_code에 할당됩니다.
 *   확인 코드=00H는 성공을 나타내며;
 *   확인 코드=01H는 패킷 수신 오류를 나타냅니다.
 */
bool PS_ReadIndexTable(int* indexList, int size) {
  // indexList의 모든 요소를 -1로 초기화
  for (int i = 0; i < size; ++i)
    indexList[i] = -1;

  int nIndex = 0;

  for (int page = 0; page < 2; ++page) {
    // 데이터 전송 (두 번 전송, 각 페이지에 256개의 지문 템플릿이 있으므로 두 페이지를 요청해야 함)
    int size = GenOrder(0x1f, "%d", page);
    SendOrder(g_order, size);

    // 데이터 수신, 확인 코드 및 체크섬 확인
    if (!(RecvReply(g_reply, 44) && Check(g_reply, 44)))
      return false;

    for (int i = 0; i < 32; ++i) {
      for (int j = 0; j < 8; ++j) {
        if ( ( (g_reply[10+i] & (0x01 << j) ) >> j) == 1 ) {
          if (nIndex > size) {
            g_error_code = 0xC1;    // 배열이 너무 작음
            return false;
          }
          indexList[nIndex++] = page*256 + 8 * i + j;
        } // end if

      } // end inner for

    } // end middle for

  }// end outer for

  return true;
}

/******************************************************************
*
* 세 번째 부분:
*    as608.h에서 선언된 캡슐화된 함수
*
******************************************************************/

// 지문의 존재 여부를 검사합니다.
// '상태'가 '높음'이면 모듈에 지문이 있는 경우 true를 반환하고, 지문이 없는 경우 false를 반환합니다.
// '상태'가 '낮음'이면 모듈에 지문이 있는 경우 false를 반환하고, 지문이 없는 경우 true를 반환합니다.
bool PS_DetectFinger() {
  return digitalRead(g_as608.detect_pin) == HIGH;
}

bool PS_SetBaudRate(int value) {
  return PS_WriteReg(4, value / 9600);
}

bool PS_SetSecureLevel(int level) {
  return PS_WriteReg(5, level);
}

bool PS_SetPacketSize(int size) {
  int value = 0;
  printf("size=%d\n", size);
  switch (size) {
  default: 
    g_error_code = 0xC5; 
    return false;
  case 32:  value = 0; break;
  case 64:  value = 1; break;
  case 128: value = 2; break;
  case 256: value = 3; break;
  }

  return PS_WriteReg(6, value);
}

/*
 * 모듈의 자세한 정보를 가져와서 해당 정보를 전역 변수인 g_as608.packet_size, PS_LEVEL 등에 할당합니다.
*/
bool PS_GetAllInfo() {
  uchar buf[512] = { 0 };
  if (PS_ReadSysPara() && g_as608.packet_size > 0 && PS_ReadINFpage(buf, 512)) {
    return true;
  }
  else {
    g_error_code = 0xC7;
    return false;
  }
}

/*
 * 버퍼를 비웁니다.
 * 데이터 수신 중에 프로그램이 예기치 않게 종료되는 경우 데이터가 완전히 수신되지 않았거나 처리되지 않았을 때 실행할 수 있습니다.
**/
bool PS_Flush() {
  int num = 0;
  for (int i = 0; i < 3; ++i) {
    if (PS_ValidTempleteNum(&num)) {
      return true;
    }
    sleep(1);
  }
  return false;
}

/*
 * 오류 코드의 설명을 가져옵니다.
 * 전역 변수 g_error_desc에 할당하고 g_error_desc를 반환합니다.
*/
char* PS_GetErrorDesc() {
  switch (g_error_code) {
  default:   strcpy(g_error_desc, "Undefined error"); break;
  case 0x00: strcpy(g_error_desc, "OK"); break;
  case 0x01: strcpy(g_error_desc, "Recive packer error"); break;
  case 0x02: strcpy(g_error_desc, "No finger on the sensor"); break;
  case 0x03: strcpy(g_error_desc, "Failed to input fingerprint image"); break;
  case 0x04: strcpy(g_error_desc, "Fingerprint images are too dry and bland to be characteristic"); break;
  case 0x05: strcpy(g_error_desc, "Fingerprint images are too wet and mushy to produce features"); break;
  case 0x06: strcpy(g_error_desc, "Fingerprint images are too messy to be characteristic"); break;
  case 0x07: strcpy(g_error_desc, "The fingerprint image is normal, but there are too few feature points (or too small area) to produce a feature"); break;
  case 0x08: strcpy(g_error_desc, "Fingerprint mismatch"); break;
  case 0x09: strcpy(g_error_desc, "Not found in fingerprint libary"); break;
  case 0x0A: strcpy(g_error_desc, "Feature merge failed"); break;
  case 0x0B: strcpy(g_error_desc, "The address serial number is out of the range of fingerprint database when accessing fingerprint database"); break;
  case 0x0C: strcpy(g_error_desc, "Error or invalid reading template from fingerprint database"); break;
  case 0x0D: strcpy(g_error_desc, "Upload feature failed"); break;
  case 0x0E: strcpy(g_error_desc, "The module cannot accept subsequent packets"); break;
  case 0x0F: strcpy(g_error_desc, "Failed to upload image"); break;
  case 0x10: strcpy(g_error_desc, "Failed to delete template"); break;
  case 0x11: strcpy(g_error_desc, "Failed to clear the fingerprint database"); break;
  case 0x12: strcpy(g_error_desc, "Cannot enter low power consumption state"); break;
  case 0x13: strcpy(g_error_desc, "Incorrect password"); break;
  case 0x14: strcpy(g_error_desc, "System reset failure"); break;
  case 0x15: strcpy(g_error_desc, "An image cannot be generated without a valid original image in the buffer"); break;
  case 0x16: strcpy(g_error_desc, "Online upgrade failed"); break;
  case 0x17: strcpy(g_error_desc, "There was no movement of the finger between the two collections"); break;
  case 0x18: strcpy(g_error_desc, "FLASH reading or writing error"); break;
  case 0x19: strcpy(g_error_desc, "Undefined error"); break;
  case 0x1A: strcpy(g_error_desc, "Invalid register number"); break;
  case 0x1B: strcpy(g_error_desc, "Register setting error"); break;
  case 0x1C: strcpy(g_error_desc, "Notepad page number specified incorrectly"); break;
  case 0x1D: strcpy(g_error_desc, "Port operation failed"); break;
  case 0x1E: strcpy(g_error_desc, "Automatic enrollment failed"); break;
  case 0xFF: strcpy(g_error_desc, "Fingerprint is full"); break;
  case 0x20: strcpy(g_error_desc, "Reserved. Wrong address or wrong password"); break;
  case 0xF0: strcpy(g_error_desc, "There are instructions for subsequent packets, and reply with 0xf0 after correct reception"); break;
  case 0xF1: strcpy(g_error_desc, "There are instructions for subsequent packets, and the command packet replies with 0xf1"); break;
  case 0xF2: strcpy(g_error_desc, "Checksum error while burning internal FLASH"); break;
  case 0xF3: strcpy(g_error_desc, "Package identification error while burning internal FLASH"); break;
  case 0xF4: strcpy(g_error_desc, "Packet length error while burning internal FLASH"); break;
  case 0xF5: strcpy(g_error_desc, "Code length is too long to burn internal FLASH"); break;
  case 0xF6: strcpy(g_error_desc, "Burning internal FLASH failed"); break;
  case 0xC1: strcpy(g_error_desc, "Array is too smalll to store all the data"); break;
  case 0xC2: strcpy(g_error_desc, "Open local file failed!"); break;
  case 0xC3: strcpy(g_error_desc, "Packet loss"); break;
  case 0xC4: strcpy(g_error_desc, "No end packet received, please flush the buffer(PS_Flush)"); break;
  case 0xC5: strcpy(g_error_desc, "Packet size not in 32, 64, 128 or 256"); break;
  case 0xC6: strcpy(g_error_desc, "Array size is to big");break;
  case 0xC7: strcpy(g_error_desc, "Setup failed! Please retry again later"); break;
  case 0xC8: strcpy(g_error_desc, "The size of the data to send must be an integral multiple of the g_as608.packet_size"); break;
  case 0xC9: strcpy(g_error_desc, "The size of the fingerprint image is not 74806bytes(about73.1kb)");break;
  case 0xCA: strcpy(g_error_desc, "Error while reading local fingerprint imgae"); break;
  
  }

  return g_error_desc;
}
