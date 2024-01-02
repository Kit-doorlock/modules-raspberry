#include "pn532.h"
#include "pn532_rpi.h"
#include "dictionary.h"

#define MAX_WAIT 10 // sec

#define UART 0
#define SPI 1
#define I2C 2 // IIC

extern uint8_t buff[255];
extern uint8_t uid[MIFARE_UID_MAX_LENGTH];
extern PN532 pn532;

typedef struct pn532_firmware_version {
    uint8_t integer; // 정수부
    uint8_t decimal; // 소숫점 이하
} firmware;

void waitUntilNotDetectCard();
void waitUntilDetectCard();
int initPN532(uchar_t serial);

void addCard();
void deleteCard();
bool existCard();