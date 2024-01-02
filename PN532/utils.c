#include "utils.h"

uint8_t buff[255];
uint8_t uid[MIFARE_UID_MAX_LENGTH];
PN532 pn532;

uint32_t makePrefix(int uid_len);
firmware getPN532Version();
uint32_t getCardUid();

void waitUntilNotDetectCard() {
    while(PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000) != PN532_STATUS_ERROR);
}

void waitUntilDetectCard() {
    while(PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000) == PN532_STATUS_ERROR);
}

int initPN532(uchar_t serial) {
    firmware f;

    switch (serial) {
        case UART:
            PN532_UART_Init(&pn532);
            break;
        case SPI:
            PN532_SPI_Init(&pn532);
            break;
        case I2C:
            PN532_I2C_Init(&pn532);
            break;
        default:
            return -1;
            break;    
    }

    f = getPN532Version();
    if (f.integer == 255U) return -2;

    PN532_SamConfiguration(&pn532);

    printf("Init Complete!\n");
    return 0;
}

firmware getPN532Version() {
    firmware f = {-1, -1};
    if (PN532_GetFirmwareVersion(&pn532, buff) == PN532_STATUS_OK) {
        f.integer = buff[1];
        f.decimal = buff[2];
    }
    return f;
}

uint32_t makePrefix(int uid_len) { // uid_len은 대부분 4
    uint32_t val = 0;
    for (uint8_t i = 0; i < uid_len; i++) {
        printf("%02x ", uid[i]);
        val |= uid[i];
        val <<= 8;
    }
    return val;
}

uint32_t getCardUid() {
    int uid_len;
    uint32_t key;
    int wait_time = MAX_WAIT;
    printf("Waiting for RFID/NFC card...\r\n");
    while (1)
    {
        // Check if a card is available to read
        uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);
        if (uid_len == PN532_STATUS_ERROR) {
            printf(".");
            fflush(stdout);

            wait_time--;
            if (wait_time <= 0) {
                printf("\r\n");
                return 0x0;
            }
        } else {
            printf("Found card with UID: ");
            key = makePrefix(uid_len);
            printf("\r\n");
            return key;
        }
    }
}

void addCard() {
    uint32_t key = getCardUid();
    if (key == 0x0) return;
    appendElement(key);
}

bool existCard() {
    uint32_t key = getCardUid();
    if (key == 0) false;
    return checkExist(key) == true;
}

void deleteCard() {
    uint32_t key = getCardUid();
    if (key == 0x0) return;
    deleteElement(key);
}