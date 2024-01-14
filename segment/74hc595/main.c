#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>

#define CLK 20
#define DIO 21
#define LCH 16
#define BIT_8 8

void writeFND(unsigned int data);
void writeFNDByte(unsigned char data);
void writeFNDBit(unsigned char data);

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
    pinMode(DIO, OUTPUT);
    pinMode(LCH, OUTPUT);

    digitalWrite(CLK, LOW);
    digitalWrite(DIO, LOW);
    digitalWrite(LCH, LOW);
}

void write8digitFND(const unsigned int datas[]) {
    unsigned int data = 0;
    for (int i = 0; i < 8; i++) {
        data = (unsigned int)datas[i] << 8;
        data |= 1 << (7 - i);
        writeFND(data);
    }
}

void writeFND(unsigned int data) {
    writeFNDByte( data & 0xff ); // position
    writeFNDByte( (data >> 8 ) & 0xff ); // data

    digitalWrite(LCH, HIGH);
    delay(2);
    digitalWrite(LCH, LOW);
}

void print(unsigned char data) {
    for (int i = 7; i >= 0; i--) printf("%d", (data >> i) & 1);
    printf(" ");
}

void writeFNDBit(unsigned char bit) {
    digitalWrite(DIO, (bit % 2) == 1 ? HIGH : LOW);
    digitalWrite(CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(CLK, LOW);
}

void writeFNDByte(unsigned char data) {
    // print(data);
    for (int i = 0; i < BIT_8; i++) {
        writeFNDBit(data & 0x01);
        data >>= 1;
    }
}

// 동작
// LCH LOW -> { CLK HIGH -> DATA -> CLK LOW } * 16 -> LCH HIGH
// 상위 1byte - 데이터

// 하위 1byte - 위치
// {1}{2}{3}{4}{5}{6}{7}{8} 각 위치가 1bit
// e.g.) 0xff - 모든 위치에 8 띄움

int main() {
    unsigned int datas[8];

    init();

    // writeFND(0b0001101 << 8 | 0x80);
    for (int i = 0; i < 8; i++) {
        datas[i] = FND_CHAR_MAP[i+1];
    }
    while(1) {
        write8digitFND(datas);
    }

    
}
