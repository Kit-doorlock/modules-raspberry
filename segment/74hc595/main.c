#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>

#define LCH 16
#define CLK 20
#define SER 21

#define BIT_8 8
#define ONE_SEC 62

void writeFND(unsigned int data);
void writeFNDByte(unsigned char data);
void writeFNDBit(unsigned char data);
void displayFND();

const unsigned char FND_CHAR_MAP[10] = {
    0b0000001, // 0
    0b1001111, // 1
    0b0010010, // 2
    0b0000110, // 3
    0b1001100, // 4
    0b0100100, // 5
    0b0100000, // 6
    0b0001101, // 7
    0b0000000, // 8
    0b0000100, // 9
};

void init() {
    if (wiringPiSetupGpio() == -1) exit(-1);

    pinMode(CLK, OUTPUT);
    pinMode(SER, OUTPUT);
    pinMode(LCH, OUTPUT);

    digitalWrite(CLK, LOW);
    digitalWrite(SER, LOW);
    digitalWrite(LCH, LOW);
}

void write8digitFND(const unsigned char datas[]) {
    unsigned int data = 0;
    for (int i = 0; i < 8; i++) {
        data = (unsigned int)datas[i] << 8;
        data |= 1 << (7 - i);
        writeFND(data);
    }
}

void writeFND(unsigned int data) {
    writeFNDByte( data & 0xff ); // position
    writeFNDByte( data >> 8 ); // data

    displayFND();
}

void displayFND() {
    digitalWrite(LCH, HIGH);
    delay(2);
    digitalWrite(LCH, LOW);
}

void writeFNDByte(unsigned char data) {
    for (int i = 0; i < BIT_8; i++) {
        writeFNDBit(data & 0x01);
        data >>= 1;
    }
}

void writeFNDBit(unsigned char bit) {
    digitalWrite(SER, (bit % 2) == 1 ? HIGH : LOW);
    digitalWrite(CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(CLK, LOW);
}

// 동작
// LCH LOW -> { CLK HIGH -> DATA -> CLK LOW } * 16 -> LCH HIGH
// 상위 1byte - 데이터

// 하위 1byte - 위치
// {1}{2}{3}{4}{5}{6}{7}{8} 각 위치가 1bit
// e.g.) 0xff - 모든 위치에 8 띄움

int main() {
    unsigned char datas[8];
    int i = 0;
    int cnt = 0;

    init();

    // writeFND(0b0001101 << 8 | 0x80);
    while(1) {
        for (int j = 0; j < 8; j++)
            datas[j] = FND_CHAR_MAP[(i+j*3) % 10] | 0x8000;

        while ((++cnt) < ONE_SEC)
            write8digitFND(datas);

        i++; cnt = 0;
    }

    
}
