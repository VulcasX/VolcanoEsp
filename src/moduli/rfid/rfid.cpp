#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include "rfid.h"
#include "input.h"
#include "core/config/config.h"
#include "core/common/common.h"
#include "core/common/virtualkeyboard.h"

// Definizioni per il display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CHAR_HEIGHT 8
#define LINE_SPACING 1
#define LINES_ON_SCREEN ((SCREEN_HEIGHT) / (CHAR_HEIGHT + LINE_SPACING))

// PIN ESP32 I2C
#define SDA_PIN 21
#define SCL_PIN 22

Extended_PN532 nfc(SDA_PIN, SCL_PIN);

void rfid() {
  // Attendi che il pulsante SET sia rilasciato prima di mostrare il menu
  common::debounce(50); // Debounce per evitare rimbalzi del pulsante
  display.clearDisplay(); // Pulisce il display OLED

  while(true) {
    display.clearDisplay();
    common::println("Menu RFID", 0, 0, 1, SSD1306_WHITE);
    common::println("1. Read UID", 0, 10, 1, SSD1306_WHITE);
    common::println("2. Dump Card", 0, 20, 1, SSD1306_WHITE);
    common::println("3. MFCUK Tools", 0, 30, 1, SSD1306_WHITE);
    common::println("RST: Exit", 0, 50, 1, SSD1306_WHITE);
    display.display();

    if(digitalRead(buttonPin_UP) == LOW) {
      common::debounceButton(buttonPin_UP, 50);
      rfid_read();
    }
    else if(digitalRead(buttonPin_DWN) == LOW) {
      common::debounceButton(buttonPin_DWN, 50);
      dump();
    }
    else if(digitalRead(buttonPin_SET) == LOW) {
      common::debounceButton(buttonPin_SET, 50);
      rfid_advanced_menu(); // Corretto da mfcuk_menu a rfid_advanced_menu
    }
    else if(digitalRead(buttonPin_RST) == LOW) {
      while(digitalRead(buttonPin_RST) == LOW) delay(10);
      common::debounceButton(buttonPin_RST, 50);
      return;
    }
    delay(10);
  }
}
/*
void dump() {
  // Attendi che il pulsante SET sia rilasciato prima di mostrare il menu
  common::debounce(50); // Debounce per evitare rimbalzi del pulsante
  const int linesOnScreen = 5; // Numero di righe visibili sul display (adatta se necessario)
  // PIN ESP32 I2C
  char buffer[64];
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
              sprintf(buffer, "%02X ", data[i]);
              file.print(buffer); Serial.print(buffer);
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
*/
/*void dump() {
    common::debounce(50);
    char buffer[64];
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
    display.clearDisplay();
    common::println("Avvicina il tag...", 0, 0, 1, SSD1306_WHITE);

    while (true) {
        if (digitalRead(buttonPin_RST) == LOW) {
            return;
        }
        uint8_t uid[7];
        uint8_t uidLength;

        if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
            Serial.println("Tag trovato!");
            display.clearDisplay();
            common::println("Tag trovato!", 0, 0, 1, SSD1306_WHITE);

            // Buffer per visualizzare il dump sul display
            String dumpScreen = "";

            for (uint8_t sector = 0; sector < 16; sector++) {
                for (uint8_t block = 0; block < 4; block++) {
                    uint8_t blockNumber = sector * 4 + block;
                    uint8_t data[16];
                    uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

                    String line = "";
                    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keyA)) {
                        if (nfc.mifareclassic_ReadDataBlock(blockNumber, data)) {
                            line += "S";
                            line += sector;
                            line += " B";
                            line += block;
                            line += ": ";
                            for (int i = 0; i < 16; i++) {
                                char hex[4];
                                sprintf(hex, "%02X ", data[i]);
                                line += hex;
                            }
                        } else {
                            line += "Err lettura B";
                            line += blockNumber;
                        }
                    } else {
                        line += "Auth fallita B";
                        line += blockNumber;
                    }
                    dumpScreen += line + "\n";
                }
            }

            // Mostra il dump sul display, scorrendo le righe
            display.clearDisplay();
            // Suddividi dumpScreen in righe
            String dumpLines[64]; // max 64 righe (16 settori x 4 blocchi)
            int dumpCount = 0;
            int start = 0;
            int end = dumpScreen.indexOf('\n');
            while (end != -1 && dumpCount < 64) {
                dumpLines[dumpCount++] = dumpScreen.substring(start, end);
                start = end + 1;
                end = dumpScreen.indexOf('\n', start);
            }
            if (start < dumpScreen.length() && dumpCount < 64) {
                dumpLines[dumpCount++] = dumpScreen.substring(start);
            }

            int scroll = 0;
            const int linesPerScreen = 6;
            while (true) {
                display.clearDisplay();
                int y = 0;
                for (int i = 0; i < linesPerScreen; i++) {
                    int idx = scroll + i;
                    if (idx < dumpCount) {
                        common::println(dumpLines[idx].c_str(), 0, y, 1, SSD1306_WHITE);
                        y += 10;
                    }
                }
                common::println("UP/DOWN: scroll", 0, 56, 1, SSD1306_WHITE);
                common::println("SET: salva", 0, 46, 1, SSD1306_WHITE);
                display.display();

                // Gestione pulsanti
                if (digitalRead(buttonPin_UP) == LOW) {
                    if (scroll > 0) scroll--;
                    delay(150);
                    while (digitalRead(buttonPin_UP) == LOW) delay(10);
                }
                if (digitalRead(buttonPin_DWN) == LOW) {
                    if (scroll < dumpCount - linesPerScreen) scroll++;
                    delay(150);
                    while (digitalRead(buttonPin_DWN) == LOW) delay(10);
                }
                if (digitalRead(buttonPin_SET) == LOW) {
                    common::debounceButton(buttonPin_SET, 50);
                    break;
                }
                if (digitalRead(buttonPin_RST) == LOW) {
                    return;
                }
                delay(10);
            }

            // Salva il dump su file
            File file = LittleFS.open("/dump.txt", "w");
            if (!file) {
                Serial.println("Errore apertura file");
                return;
            }
            for (int i = 0; i < dumpCount; i++) {
                file.println(dumpLines[i]);
            }
            file.close();
            Serial.println("Dump salvato su /dump.txt");

            display.clearDisplay();
            common::println("Dump completato!", 0, 0, 1, SSD1306_WHITE);
            common::println("Premi SET per nuovo dump", 0, 16, 1, SSD1306_WHITE);
            display.display();

            while (digitalRead(buttonPin_SET) == LOW) {
                delay(10);
            }
            common::debounceButton(buttonPin_SET, 50);
        }
        delay(500);
    }
}*/
//ok
/*
void dump() {
  common::debounce(50);
  char buffer[128];

  Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

  if (!LittleFS.begin()) {
    Serial.println("Errore inizializzazione LittleFS");
    return;
  }

  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 non trovato!");
    return;
  }

  nfc.SAMConfig();
  Serial.println("Avvicina il tag...");
  display.clearDisplay();
  common::println("Avvicina il tag...", 0, 0, 1, SSD1306_WHITE);

  while (true) {
    if (digitalRead(buttonPin_RST) == LOW) return;

    uint8_t uid[7];
    uint8_t uidLength;

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("Tag trovato!");

      // Costruisci nome file dinamico
      sprintf(buffer, "/dump_%02X%02X%02X.txt", uid[0], uid[1], uid[2]);
      File file = LittleFS.open(buffer, "w");
      if (!file) {
        Serial.println("Errore apertura file");
        return;
      }

      for (uint8_t sector = 0; sector < 16; sector++) {
        for (uint8_t block = 0; block < 4; block++) {
          uint8_t blockNumber = sector * 4 + block;
          uint8_t data[16];
          uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

          if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keyA)) {
            if (nfc.mifareclassic_ReadDataBlock(blockNumber, data)) {
              sprintf(buffer, "Block %02d: ", blockNumber);
              file.print(buffer); Serial.print(buffer);
              for (int i = 0; i < 16; i++) {
                sprintf(buffer, "%02X ", data[i]);
                file.print(buffer); Serial.print(buffer);
              }
              file.println(); Serial.println();
            } else {
              sprintf(buffer, "Errore lettura blocco %d", blockNumber);
              file.println(buffer); Serial.println(buffer);
            }
          } else {
            sprintf(buffer, "Auth fallita blocco %d", blockNumber);
            file.println(buffer); Serial.println(buffer);
          }
        }
      }

      file.close();
      Serial.println("Dump completato!");
      display.clearDisplay();
      common::println("Dump completato!", 0, 0, 1, SSD1306_WHITE);
      delay(5000);
    }

    delay(300);
  }
}
*/
/*
void dump() {
  common::debounce(50);
  char buffer[64];
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
  display.clearDisplay();
  common::println("Avvicina il tag...", 0, 0, 1, SSD1306_WHITE);
  display.display();

  while (true) {
    if (digitalRead(buttonPin_RST) == LOW) return;

    uint8_t uid[7];
    uint8_t uidLength;

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("Tag trovato!");
      display.clearDisplay();
      common::println("Tag trovato!", 0, 0, 1, SSD1306_WHITE);
      display.display();

      String dumpLines[64];
      int dumpCount = 0;

      for (uint8_t sector = 0; sector < 16; sector++) {
        for (uint8_t block = 0; block < 4; block++) {
          uint8_t blockNumber = sector * 4 + block;
          uint8_t data[16];
          uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

          String line = "";
          if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keyA)) {
            if (nfc.mifareclassic_ReadDataBlock(blockNumber, data)) {
              line += "S"; line += sector;
              line += " B"; line += block;
              line += ": ";
              for (int i = 0; i < 16; i++) {
                char hex[4];
                sprintf(hex, "%02X ", data[i]);
                line += hex;
              }
            } else {
              line += "Err lettura B"; line += blockNumber;
            }
          } else {
            line += "Auth fallita B"; line += blockNumber;
          }
          dumpLines[dumpCount++] = line;
          Serial.println(line);
        }
      }

      int scrollIndex = 0;
      while (true) {
        display.clearDisplay();
        for (int i = 0; i < 6; i++) {
          if ((scrollIndex + i) < dumpCount) {
            common::println(dumpLines[scrollIndex + i].c_str(), 0, i * 10, 1, SSD1306_WHITE);
          }
        }
        common::println("UP/DOWN: scroll", 0, 56, 1, SSD1306_WHITE);
        common::println("SET: salva", 0, 46, 1, SSD1306_WHITE);
        display.display();

        if (digitalRead(buttonPin_UP) == LOW && scrollIndex > 0) {
          scrollIndex--;
          common::debounceButton(buttonPin_UP, 100);
        }
        if (digitalRead(buttonPin_DWN) == LOW && scrollIndex + 6 < dumpCount) {
          scrollIndex++;
          common::debounceButton(buttonPin_DWN, 100);
        }
        if (digitalRead(buttonPin_SET) == LOW) {
          common::debounceButton(buttonPin_SET, 100);

          File file = LittleFS.open("/dump.txt", "w");
          if (!file) {
            Serial.println("Errore apertura file");
            return;
          }
          for (int i = 0; i < dumpCount; i++) {
            file.println(dumpLines[i]);
          }
          file.close();
          Serial.println("Dump salvato su /dump.txt");

          display.clearDisplay();
          common::println("Salvataggio completato!", 0, 0, 1, SSD1306_WHITE);
          common::println("Premi SET per nuovo dump", 0, 16, 1, SSD1306_WHITE);
          display.display();

          while (digitalRead(buttonPin_SET) == HIGH);
          common::debounceButton(buttonPin_SET, 100);
          break;
        }
        delay(50);
      }
    }
    delay(200);
  }
}
*/
//ok
void dump() {
  common::debounce(50); // Debounce per evitare rimbalzi del pulsante
  char buffer[64];
  unsigned long rstPressStart = 0;
  bool needRedraw = true;
  uint8_t uid[7];
  uint8_t uidLength;
  unsigned long lastScanTime = 0;

  if (!LittleFS.begin()) {
    Serial.println("Errore inizializzazione LittleFS");
    display.clearDisplay();
    common::println("Errore!", 0, 0, 1, SSD1306_WHITE);
    common::println("File system non", 0, 12, 1, SSD1306_WHITE);
    common::println("inizializzato", 0, 24, 1, SSD1306_WHITE);
    display.display();
    delay(2000);
    return;
  }

  // Inizializzazione esplicita del modulo NFC
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 non trovato!");
    display.clearDisplay();
    common::println("ERROR!", 0, 0, 1, SSD1306_WHITE);
    common::println("PN532 non trovato", 0, 12, 1, SSD1306_WHITE);
    display.display();
    delay(2000);
    return;
  }
  nfc.SAMConfig();
  
  Serial.println("Avvicina il tag...");
  
  while (true) {
    // Controllo per uscita con RST
    if(digitalRead(buttonPin_RST) == LOW) {
      if(rstPressStart == 0) rstPressStart = millis();
      // Uscita dopo 100ms per una risposta veloce
      if(millis() - rstPressStart > 100) {
        common::debounceButton(buttonPin_RST, 50);
        return;
      }
    } else {
      rstPressStart = 0;
    }
    
    // Ridisegna solo quando necessario per evitare flickering
    if (needRedraw) {
      display.clearDisplay();
      common::println("Avvicina il tag...", 0, 0, 1, SSD1306_WHITE);
      common::println("", 0, 24, 1, SSD1306_WHITE); 
      common::println("RST: Esci", 0, 48, 1, SSD1306_WHITE);
      display.display();
      needRedraw = false;
    }
    
    // Limitare la frequenza di scansione per ridurre il carico CPU
    if (millis() - lastScanTime > 100) {
      lastScanTime = millis();
      uint8_t uid[7];
      uint8_t uidLength;

    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      Serial.println("Tag trovato!");
      display.clearDisplay();
      common::println("Tag trovato!", 0, 0, 1, SSD1306_WHITE);

      // Costruisci nome file con UUID e gestisci duplicati con tastiera
      char uuid[32] = {0};
      for (uint8_t i = 0; i < uidLength; i++) {
        sprintf(uuid + strlen(uuid), "%02X", uid[i]);
      }
      String baseName = String("dump_") + String(uuid) + ".mfd";
      String suggested = baseName;
      int copyIdx = 1;
      while (LittleFS.exists( ("/" + suggested).c_str() )) {
        // Proponi nome incrementale
        int dot = baseName.lastIndexOf('.');
        if (dot > 0) {
          suggested = baseName.substring(0, dot) + "(" + String(copyIdx) + ")" + baseName.substring(dot);
        } else {
          suggested = baseName + "(" + String(copyIdx) + ")";
        }
        copyIdx++;
        // Chiedi nuovo nome tramite tastiera
        String result = getKeyboardInput(suggested, 32, "Nome file dump:");
        if (result.length() > 0) suggested = result;
      }
      String filename = "/" + suggested;

      // Bufferizza il dump per visualizzazione
      #define MAX_MFD_LINES 128
      String lines[MAX_MFD_LINES];
      int lineCount = 0;

      File file = LittleFS.open(filename.c_str(), "w");
      if (!file) {
        Serial.println("Errore apertura file");
        return;
      }

      for (uint8_t sector = 0; sector < 16; sector++) {
        for (uint8_t block = 0; block < 4; block++) {
          uint8_t blockNumber = sector * 4 + block;
          uint8_t data[16];
          uint8_t keyA[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
          if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keyA)) {
            if (nfc.mifareclassic_ReadDataBlock(blockNumber, data)) {
              file.write(data, 16);
              String line = "S";
              if (sector < 10) line += "0";
              line += String(sector);
              line += " B";
              if (block < 10) line += "0";
              line += String(block);
              line += ": ";
              for (int i = 0; i < 16; i++) {
                char hex[4];
                sprintf(hex, "%02X ", data[i]);
                line += hex;
              }
              // Suddividi la riga lunga in più righe se necessario
              int start = 0;
              while (start < line.length() && lineCount < MAX_MFD_LINES) {
                lines[lineCount++] = line.substring(start, start + 32); // 32 caratteri per compatibilità
                start += 32;
              }
            } else {
              String err = "Err lettura B" + String(blockNumber);
              lines[lineCount++] = err;
            }
          } else {
            String err = "Auth fallita B" + String(blockNumber);
            lines[lineCount++] = err;
          }
        }
      }
      file.close();
      Serial.print("Dump salvato su "); Serial.println(filename);

      // Visualizzazione stile littlefs
      int scroll = 0;
      int hScroll = 0;
      bool redraw = true;
      int maxLineLen = 0;
      for (int i = scroll; i < scroll + LINES_ON_SCREEN && i < lineCount; i++) {
        if (lines[i].length() > maxLineLen) maxLineLen = lines[i].length();
      }
      int charsPerLine = 20; // Adatta a font e display
      int maxHScroll = (maxLineLen > charsPerLine) ? (maxLineLen - charsPerLine) : 0;
      while (true) {
        if (digitalRead(buttonPin_RST) == LOW) {
          while (digitalRead(buttonPin_RST) == LOW) delay(10);
          common::debounceButton(buttonPin_RST, 50);
          return;
        }
        if (redraw) {
          display.clearDisplay();
          for (int i = 0; i < LINES_ON_SCREEN; i++) {
            int idx = scroll + i;
            if (idx < lineCount) {
              display.setCursor(0, i * 10);
              display.setTextColor(SSD1306_WHITE);
              display.setTextSize(1);
              String toShow = lines[idx];
              if (hScroll < toShow.length())
                toShow = toShow.substring(hScroll);
              else
                toShow = "";
              if (toShow.length() > charsPerLine)
                toShow = toShow.substring(0, charsPerLine);
              display.write(toShow.c_str());
            }
          }
          // Barra verticale
          int barX = 120; // Adatta se serve
          int barY = 0;
          int barH = 56;
          int barW = 8;
          display.drawRect(barX, barY, barW, barH, SSD1306_WHITE);
          if (lineCount > LINES_ON_SCREEN) {
            int scrollH = (LINES_ON_SCREEN * barH) / lineCount;
            if (scrollH < 8) scrollH = 8;
            int scrollY = (scroll * (barH - scrollH)) / (lineCount - LINES_ON_SCREEN);
            display.fillRect(barX + 1, barY + 1 + scrollY, barW - 2, scrollH - 2, SSD1306_WHITE);
          }
          // Barra orizzontale (logica identica a quella verticale)
          int hBarY = 56;
          int hBarX = 0;
          int hBarW = 120;
          int hBarH = 8;
          display.drawRect(hBarX, hBarY, hBarW, hBarH, SSD1306_WHITE);
          if (maxLineLen > charsPerLine) {
            int hScrollW = (charsPerLine * hBarW) / maxLineLen;
            if (hScrollW < 8) hScrollW = 8;
            int hScrollX = (hScroll * (hBarW - hScrollW)) / (maxLineLen - charsPerLine);
            display.fillRect(hBarX + 1 + hScrollX, hBarY + 1, hScrollW - 2, hBarH - 2, SSD1306_WHITE);
          } else {
            display.fillRect(hBarX + 1, hBarY + 1, hBarW - 2, hBarH - 2, SSD1306_WHITE);
          }
          display.display();
          redraw = false;
        }
        if (digitalRead(buttonPin_UP) == LOW && scroll > 0) {
          scroll--;
          redraw = true;
          delay(150);
        }
        if (digitalRead(buttonPin_DWN) == LOW && scroll + LINES_ON_SCREEN < lineCount) {
          scroll++;
          redraw = true;
          delay(150);
        }
        if (digitalRead(buttonPin_LFT) == LOW && hScroll > 0) {
          hScroll--;
          redraw = true;
          delay(150);
        }
        if (digitalRead(buttonPin_RHT) == LOW && hScroll < maxHScroll) {
          hScroll++;
          redraw = true;
          delay(150);
        }
        if (digitalRead(buttonPin_SET) == LOW) {
          while (digitalRead(buttonPin_SET) == LOW) delay(10);
          common::debounceButton(buttonPin_SET, 50);
          break; // esci dal ciclo visualizzazione e riparti con nuovo dump
        }
        delay(10);
      }
      // Se arrivi qui, riparti con nuovo dump
    }
    delay(300);
  }
  }
}

void rfid_read() {
    // Attendi che il pulsante SET sia rilasciato prima di mostrare il menu
    common::debounce(50); // Debounce per evitare rimbalzi del pulsante
    char buffer[16];
    bool needRedraw = true;
    bool cardFound = false;
    uint8_t uid[7];    // buffer UID
    uint8_t uidLength;
    unsigned long lastScanTime = 0;
    unsigned long rstPressStart = 0;
    
    // Inizializzazione esplicita del modulo NFC
    nfc.begin();
    if (!nfc.getFirmwareVersion()) {
        display.clearDisplay();
        common::println("ERROR!", 0, 0, 1, SSD1306_WHITE);
        common::println("PN532 non trovato", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return;
    }
    nfc.SAMConfig();
    while(true) {
        // Controllo per uscita con RST
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            // Uscita dopo 100ms per una risposta veloce
            if(millis() - rstPressStart > 100) {
                common::debounceButton(buttonPin_RST, 50);
                return;
            }
        } else {
            rstPressStart = 0;
        }

        // Ridisegna solo quando necessario per evitare flickering
        if (needRedraw) {
            display.clearDisplay();
            if (cardFound) {
                common::print("UID trovato: ", 0, 0, 1, SSD1306_WHITE);

                int base_x = common::getSpacing("UID trovato: ", 1);
                int byte_width = common::getSpacing("FF", 1);

                for (uint8_t i = 0; i < uidLength; i++) {
                    sprintf(buffer, "%02X", uid[i]);
                    int x = base_x + i * byte_width;
                    common::print(buffer, x, 0, 1, SSD1306_WHITE);
                }
                
                common::println("Avvicina altra carta", 0, 24, 1, SSD1306_WHITE);
                common::println("o premi RST per", 0, 36, 1, SSD1306_WHITE);
                common::println("uscire", 0, 48, 1, SSD1306_WHITE);
            } else {
                common::println("Avvicina il tag...", 0, 0, 1, SSD1306_WHITE);
                common::println("", 0, 24, 1, SSD1306_WHITE);
                common::println("RST: Esci", 0, 48, 1, SSD1306_WHITE);
            }
            display.display();
            needRedraw = false;
        }
        
        // Limitare la frequenza di scansione per ridurre il carico CPU
        if (millis() - lastScanTime > 100) {
            lastScanTime = millis();
            if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
                cardFound = true;
                needRedraw = true;
                // Piccolo feedback sonoro o visivo potrebbe essere aggiunto qui
            }
        }
        
        delay(10);
    }
}
