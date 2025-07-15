#ifndef _EXTENDED_PN532_H_
#define _EXTENDED_PN532_H_

#include <Wire.h>
#include <Adafruit_PN532.h>

// Comandi Mifare
#define MIFARE_CMD_AUTH_A         0x60
#define MIFARE_CMD_AUTH_B         0x61
#define MIFARE_CMD_READ           0x30
#define MIFARE_CMD_WRITE          0xA0

class Extended_PN532 : public Adafruit_PN532 {
    friend class PN532;  // Per accedere ai membri privati di Adafruit_PN532
public:
    Extended_PN532(uint8_t irq, uint8_t reset) : Adafruit_PN532(irq, reset) {
        Wire.begin();
    }
    
    // Funzioni estese per l'attacco MFCUK
    bool mifareClassicGetNT(uint8_t* nt);
    bool mifareClassicGetAR(uint8_t* nr, uint8_t* ar);
    bool readResponse(uint8_t* buffer, uint8_t length, uint16_t timeout = 1000);
    
    // Funzione di reset hardware
    bool resetPN532();
    
    // Funzioni resilienti per l'attacco MFCUK
    bool robustMifareClassicGetNT(uint8_t* nt, uint8_t maxRetries = 5);
    bool robustMifareClassicGetAR(uint8_t* nr, uint8_t* ar, uint8_t maxRetries = 5);
    
private:
    uint8_t pn532_packetbuffer[64];
    bool sendRawCommand(uint8_t* cmd, uint8_t cmdlen, uint8_t* response, uint8_t* responseLength);
    bool resetI2CBus();
};

#endif
