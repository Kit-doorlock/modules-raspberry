#include <wiringPi.h>
#include <stdio.h>

#define CLK 20
#define DIO 21
#define LCH 16

void writeBit(unsigned char bit) {
    digitalWrite(SER, bit);
    digitalWrite(CLK, HIGH);
    delayMicroseconds(1);
    digitalWrite(CLK, LOW);
}

int main() {
    unsigned int temp = 0x40ff, t;
    wiringPiSetupGpio();

    pinMode(CLK, OUTPUT);
    pinMode(SER, OUTPUT);
    pinMode(LCH, OUTPUT);

    digitalWrite(CLK, LOW);
    digitalWrite(SER, LOW);
    digitalWrite(LCH, LOW);

        t = temp;
        for (int j = 0; j < 16; j++) {
            writeBit(t & 1);
            t >>= 1;
        }
        digitalWrite(LCH, HIGH);
        delay(1000);
        digitalWrite(LCH, LOW);
}