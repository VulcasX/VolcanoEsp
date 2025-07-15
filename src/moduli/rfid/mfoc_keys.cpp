/**
 * MFOC - Mifare Classic Offline Cracker
 * Implementazione della funzionalità di caricamento/salvataggio chiavi
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <Adafruit_SSD1306.h>
#include "mfoc.h"
#include "mfcuk_utils.h"
#include "mfoc_keys.h"
#include "../../core/common/common.h"
#include "../../core/littlefs/littlefs.h"
#include "../../core/common/virtualkeyboard.h"
#include <input.h>
#include <Arduino.h>
// Riferimento al display OLED
extern Adafruit_SSD1306 display;

// Prototipi delle funzioni definite in questo file
void mfoc_view_keys(MfocCard* card);
void mfoc_save_keys_ui(MfocCard* card);
void mfoc_load_keys_ui(MfocCard* card);
/**
 * Visualizza le chiavi memorizzate nella carta
 */
void mfoc_view_keys(MfocCard* card) {
    int currentSector = 0;
    bool needRedraw = true;
    int totalSectors = card->num_sectors;
    
    while(true) {
        if (needRedraw) {
            display.clearDisplay();
            //common::println("Chiavi Settore " + String(currentSector), 0, 0, 1, SSD1306_WHITE);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 0);
            display.setTextSize(1);
            display.println("Settore: " + String(currentSector));
            Serial.printf("Settore: %d\n", currentSector);
            // Visualizza la chiave A
            display.setCursor(0, 12);
            display.print("A: ");
            if (card->sectors[currentSector].foundKeyA) {
                char keyHex[MIFARE_KEY_SIZE * 2 + 1];
                bytes_to_hex(card->sectors[currentSector].KeyA.bytes, keyHex, MIFARE_KEY_SIZE);
                display.print(keyHex);
            } else {
                display.print("non trovata");
            }
            
            // Visualizza la chiave B
            display.setCursor(0, 24);
            display.print("B: ");
            if (card->sectors[currentSector].foundKeyB) {
                char keyHex[MIFARE_KEY_SIZE * 2 + 1];
                bytes_to_hex(card->sectors[currentSector].KeyB.bytes, keyHex, MIFARE_KEY_SIZE);
                display.print(keyHex);
            } else {
                display.print("non trovata");
            }
            
            display.setCursor(0, 48);
            display.print("UP/DWN settore, RST esci");
            display.display();
            
            needRedraw = false;
        }
        
        // Naviga tra i settori
        if (digitalRead(buttonPin_UP) == LOW) {
            currentSector = (currentSector - 1 + totalSectors) % totalSectors;
            common::debounceButton(buttonPin_UP, 120);
            needRedraw = true;
        }
        
        if (digitalRead(buttonPin_DWN) == LOW) {
            currentSector = (currentSector + 1) % totalSectors;
            common::debounceButton(buttonPin_DWN, 120);
            needRedraw = true;
        }
        
        if (digitalRead(buttonPin_RST) == LOW) {
            common::debounceButton(buttonPin_RST, 50);
            return;
        }
        
        delay(10);
    }
}

/**
 * Interfaccia per salvare le chiavi
 */
void mfoc_save_keys_ui(MfocCard* card) {
    display.clearDisplay();
    common::println("Salva chiavi", 0, 0, 1, SSD1306_WHITE);
    common::println("Inserisci nome file:", 0, 12, 1, SSD1306_WHITE);
    display.display();
    
    // Chiedi il nome del file
    String filename = getKeyboardInput("", 20, "Nome file:");
    
    if (filename.length() > 0) {
        // Aggiungi estensione .txt se non presente
        if (!filename.endsWith(".txt")) {
            filename += ".txt";
        }
        
        // Aggiungi '/' all'inizio se non presente
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        
        display.clearDisplay();
        common::println("Salvataggio...", 0, 0, 1, SSD1306_WHITE);
        display.display();
        
        if (mfoc_save_keys(filename.c_str(), card)) {
            display.clearDisplay();
            common::println("Chiavi salvate in:", 0, 0, 1, SSD1306_WHITE);
            common::println(filename.c_str(), 0, 12, 1, SSD1306_WHITE);
            display.display();
        } else {
            display.clearDisplay();
            common::println("Errore salvataggio", 0, 0, 1, SSD1306_WHITE);
            display.display();
        }
        
        delay(2000);
    }
}

/**
 * Interfaccia per caricare le chiavi
 */
void mfoc_load_keys_ui(MfocCard* card) {
    display.clearDisplay();
    common::println("Carica chiavi", 0, 0, 1, SSD1306_WHITE);
    common::println("Seleziona file:", 0, 12, 1, SSD1306_WHITE);
    display.display();
    
    // Utilizziamo la funzione browse_files per selezionare un file
    String filename = browse_files("/");
    
    if (filename.length() > 0) {
        display.clearDisplay();
        common::println("Caricamento...", 0, 0, 1, SSD1306_WHITE);
        display.display();
        
        if (mfoc_load_keys_file(filename.c_str(), card)) {
            display.clearDisplay();
            common::println("Chiavi caricate da:", 0, 0, 1, SSD1306_WHITE);
            common::println(filename.c_str(), 0, 12, 1, SSD1306_WHITE);
            display.display();
        } else {
            display.clearDisplay();
            common::println("Errore caricamento", 0, 0, 1, SSD1306_WHITE);
            common::println("o file vuoto", 0, 12, 1, SSD1306_WHITE);
            display.display();
        }
        
        delay(2000);
    }
}

/**
 * Salva le chiavi trovate durante l'attacco
 */
bool mfoc_save_keys(const char* filename, MfocCard* card) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("Errore apertura file per salvataggio");
        return false;
    }
    
    file.println("# MFOC Keys - VolcanoEsp");
    file.println("# Formato: <settore>;<tipo>;<chiave>");
    file.println();
    
    for (int s = 0; s < card->num_sectors; s++) {
        if (card->sectors[s].foundKeyA) {
            char keyHex[MIFARE_KEY_SIZE * 2 + 1];
            bytes_to_hex(card->sectors[s].KeyA.bytes, keyHex, MIFARE_KEY_SIZE);
            
            file.print(s);
            file.print(";A;");
            file.println(keyHex);
        }
        
        if (card->sectors[s].foundKeyB) {
            char keyHex[MIFARE_KEY_SIZE * 2 + 1];
            bytes_to_hex(card->sectors[s].KeyB.bytes, keyHex, MIFARE_KEY_SIZE);
            
            file.print(s);
            file.print(";B;");
            file.println(keyHex);
        }
    }
    
    file.close();
    return true;
}

/**
 * Carica un file di chiavi in formato standard
 * Formato: <settore>;<tipo>;<chiave>
 * dove:
 * - settore è il numero del settore (0-15 per Mifare 1K)
 * - tipo è 'A' o 'B'
 * - chiave è la chiave in formato esadecimale (12 caratteri)
 */
bool mfoc_load_keys_file(const char* filename, MfocCard* card) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Errore apertura file chiavi");
        return false;
    }
    
    int keysLoaded = 0;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        // Ignora righe vuote e commenti
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }
        
        // Analizza la riga: <settore>;<tipo>;<chiave>
        int firstSep = line.indexOf(';');
        if (firstSep <= 0) continue;
        
        int secondSep = line.indexOf(';', firstSep + 1);
        if (secondSep <= firstSep + 1) continue;
        
        // Estrae settore, tipo e chiave
        int sector = line.substring(0, firstSep).toInt();
        char keyType = line.substring(firstSep + 1, secondSep)[0];
        String keyHex = line.substring(secondSep + 1);
        
        // Verifica che settore e tipo siano validi
        if (sector < 0 || sector >= card->num_sectors) continue;
        if (keyType != 'A' && keyType != 'B') continue;
        if (keyHex.length() != 12) continue;
        
        // Converte la chiave da hex a bytes
        uint8_t keyBytes[MIFARE_KEY_SIZE];
        hex_to_bytes(keyHex.c_str(), keyBytes, MIFARE_KEY_SIZE);
        
        // Salva la chiave nella struttura della carta
        if (keyType == 'A') {
            memcpy(card->sectors[sector].KeyA.bytes, keyBytes, MIFARE_KEY_SIZE);
            card->sectors[sector].foundKeyA = true;
        } else {
            memcpy(card->sectors[sector].KeyB.bytes, keyBytes, MIFARE_KEY_SIZE);
            card->sectors[sector].foundKeyB = true;
        }
        
        keysLoaded++;
    }
    
    file.close();
    return (keysLoaded > 0);
}

/**
 * Visualizza il gestore delle chiavi trovate
 */
void mfoc_key_manager() {
    const char* voci[] = {"Visualizza chiavi", "Salva chiavi", "Carica chiavi", "Esci"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    MfocCard card;
    
    // Inizializza la carta
    card.num_sectors = 16; // Mifare 1K
    memset(card.sectors, 0, sizeof(card.sectors));
    
    while(true) {
        if (needRedraw) {
            // Disegna l'interfaccia del menu
            display.clearDisplay();
            common::println("Gestione Chiavi", 0, 0, 1, SSD1306_WHITE);
            
            for(int i=0; i<vociCount; i++) {
                if(i == selezione) {
                    display.fillRect(0, 10+i*12, 128, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                display.setCursor(2, 12+i*12);
                display.print(voci[i]);
            }
            
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 10+vociCount*12);
            display.print("RST >2s per uscire");
            display.display();
            needRedraw = false;
        }
        
        // Navigazione - pulsante UP
        if(digitalRead(buttonPin_UP) == LOW) {
            selezione--;
            if(selezione < 0) selezione = vociCount-1;
            common::debounceButton(buttonPin_UP, 120);
            needRedraw = true;
        }
        
        // Navigazione - pulsante DOWN
        if(digitalRead(buttonPin_DWN) == LOW) {
            selezione = (selezione+1)%vociCount;
            common::debounceButton(buttonPin_DWN, 120);
            needRedraw = true;
        }
        
        // Selezione - pulsante SET
        if(digitalRead(buttonPin_SET) == LOW) {
            common::debounceButton(buttonPin_SET, 120);
            switch(selezione) {
                case 0: mfoc_view_keys(&card); 
                        break;    // Visualizza chiavi
                case 1: mfoc_save_keys_ui(&card); 
                        break; // Salva chiavi
                case 2: mfoc_load_keys_ui(&card); 
                        break; // Carica chiavi
                case 3: return;                          // Esci
            }
            needRedraw = true;
        }
        
        // Uscita con RST premuto >2s
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            if(millis() - rstPressStart > 2000) return;
        } else {
            rstPressStart = 0;
        }
        
        delay(10);
    }
}

