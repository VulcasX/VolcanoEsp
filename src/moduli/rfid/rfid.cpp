#include <Wire.h>
#include <Adafruit_PN532.h>
#include <LittleFS.h> // Include LittleFS per la gestione dei file
#include "rfid.h"
#include "input.h" // Include il file di input per i pulsanti

void rfid() {
   
    // PIN ESP32 I2C
#define SDA_PIN 21
#define SCL_PIN 22
// Instanzia il PN532 su I2C
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

  Serial.begin(115200);
  Serial.println("Inizializzazione PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 non trovato!");
    while (1); // blocca
  }

  // Mostra info firmware
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // Configura in modalità lettura passiva
  nfc.SAMConfig();
  Serial.println("In attesa di tag NFC...");
while(true) {
  if (digitalRead(buttonPin_RST) == LOW) {
    return;
  }
  boolean success;
  uint8_t uid[7];    // buffer UID
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) {
    Serial.print("UID trovato: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX); Serial.print(" ");
    }
    Serial.println();
    delay(1000);
  }
}
return; // Esci dalla funzione rfid
}

void dump() {
  // PIN ESP32 I2C
  #define SDA_PIN 21
  #define SCL_PIN 22
  // Instanzia il PN532 su I2C
  Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

  if (!LittleFS.begin()) {
    Serial.println("Errore inizializzazione LittleFS");
    while (true);
  }

  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 non trovato!");
    while (1);
  }

  nfc.SAMConfig();
  Serial.println("Avvicina il tag...");


while(true) {
  if (digitalRead(buttonPin_RST) == LOW) {
    return;
  }
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.println("Tag trovato!");

    File file = LittleFS.open("/dump.txt", "w");
    if (!file) {
      Serial.println("Errore apertura file");
      return;
    }

    for (uint8_t sector = 0; sector < 16; sector++) {
      for (uint8_t block = 0; block < 4; block++) {
        uint8_t blockNumber = sector * 4 + block;
        uint8_t data[16];

        // Default key A: FF FF FF FF FF FF
        uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

        if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keyA)) {
          if (nfc.mifareclassic_ReadDataBlock(blockNumber, data)) {
            char line[64];
            sprintf(line, "Block %02d: ", blockNumber);
            file.print(line); Serial.print(line);
            for (int i = 0; i < 16; i++) {
              sprintf(line, "%02X ", data[i]);
              file.print(line); Serial.print(line);
            }
            file.println(); Serial.println();
          } else {
            Serial.printf("Errore lettura blocco %d\n", blockNumber);
          }
        } else {
          Serial.printf("Autenticazione fallita su blocco %d\n", blockNumber);
        }
      }
    }
    file.close();
    Serial.println("Dump salvato su /dump.txt");
    delay(5000);
  }

  delay(500);
}
return; // Esci dalla funzione dump
}