#include <wiringPi.h>
#include <stdio.h>

#define SCL_PIN 20
#define SDO_PIN 21

unsigned char readKeypad()
{
    unsigned char count;
    unsigned char state = 0;

    for (count = 1; count <= 16; count++) // 16개의 키 입력 대기
    {
        digitalWrite(SCL_PIN, LOW); // SCL 핀 LOW

        if (!digitalRead(SDO_PIN))  // SDO 핀이 LOW면 
            state = count; // Key_State에 Count 저장

        digitalWrite(SCL_PIN, HIGH); // SCL 핀 HIGH
    }

    return state;
}

int main() {
    unsigned char res;
    wiringPiSetupGpio();

    pinMode(SCL_PIN, OUTPUT);
    pinMode(SDO_PIN, INPUT);

    while (1) {
        res = readKeypad();
        if (res) printf("%d\n", res);
        delay(100);
    }
}

