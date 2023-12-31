#include "pn532.h"
#include "pn532_rpi.h"
#include "dictionary.h"

#define MAX_WAIT 10 // sec

extern uint8_t buff[255];
extern uint8_t uid[MIFARE_UID_MAX_LENGTH];
extern PN532 pn532;

typedef struct pn532_firmware_version {
    uint8_t integer; // 정수부
    uint8_t decimal; // 소숫점 이하
} firmware;

void waitUntilNotDetectCard();
void waitUntilDetectCard();
void initPN532();

void addCard();
void deleteCard();
bool existCard();