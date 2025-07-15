/**
 * MFOC - Mifare Classic Offline Cracker
 * Implementazione unificata per ESP32 con display OLED
 * 
 * Basato su:
 * - mfoc originale (https://github.com/nfc-tools/mfoc)
 * - Adattato per funzionare con la nostra implementazione MFCUK
 * - Versione migliorata con funzionalità estese
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <Adafruit_SSD1306.h>
#include "mfoc.h"
#include "mfcuk_types.h"
#include "mfcuk_utils.h"
#include "mfcuk_crypto.h"
#include "rfid.h"
#include "../../lib/input/input.h"
#include "../../core/common/virtualkeyboard.h"
#include "../../core/common/common.h"
#include "../../core/littlefs/littlefs.h"
#include "../../core/config/config.h"
/*
// Strutture dati per MFOC
typedef struct {
    uint32_t* distances;
    uint32_t num_distances;
    uint32_t tolerance;
    uint32_t median;
} mfoc_denonce;

typedef struct {
    uint64_t* possibleKeys;
    uint32_t size;
} mfoc_pKeys;

typedef struct {
    uint64_t* brokenKeys;
    uint32_t size;
} mfoc_bKeys;

typedef struct {
    uint64_t key;
    uint32_t count;
} mfoc_countKeys;
/*/
// Riferimento al display OLED
extern Adafruit_SSD1306 display;

// Configurazione globale per MFOC
MfocConfig gMfocConfig;

// Array con chiavi Mifare Classic predefinite
uint8_t mfoc_default_keys[][6] = {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // Chiave predefinita
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // NFCForum MAD key
    {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // NFCForum content key
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Chiave vuota
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5},
    {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd},
    {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a},
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
    {0x71, 0x4c, 0x5c, 0x88, 0x6e, 0x97},
    {0x58, 0x7e, 0xe5, 0xf9, 0x35, 0x0f},
    {0xa0, 0x47, 0x8c, 0xc3, 0x90, 0x91},
    {0x53, 0x3c, 0xb6, 0xc7, 0x23, 0xf6},
    {0x8f, 0xd0, 0xa4, 0xf2, 0x56, 0xe9}
};

// Numero di chiavi predefinite
const int mfoc_default_keys_count = sizeof(mfoc_default_keys)/sizeof(mfoc_default_keys[0]);

// Definizione della costante NUM_DEFAULT_KEYS per retrocompatibilità
#define NUM_DEFAULT_KEYS mfoc_default_keys_count

/**
 * Converte un array di byte in un numero a 64 bit
 */
uint64_t bytes_to_num(uint8_t* src, uint32_t len) {
    uint64_t num = 0;
    while (len--) {
        num = (num << 8) | (*src);
        src++;
    }
    return num;
}

/**
 * Menu principale MFOC
 * Gestisce la navigazione e la selezione delle opzioni
 */
void mfoc_menu() {
    const char* voci[] = {"Configura", "Esegui", "Carica config", "Salva config", "Esci"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    
    // Imposta la configurazione predefinita
    gMfocConfig.known_key_type = KEY_A;
    gMfocConfig.target_key_type = KEY_A;
    gMfocConfig.target_sector = 0;
    gMfocConfig.max_iterations = 1000;
    gMfocConfig.num_probes = DEFAULT_PROBES_NR;
    gMfocConfig.sets = DEFAULT_SETS_NR;
    gMfocConfig.tolerance = DEFAULT_TOLERANCE;
    gMfocConfig.load_keys_from_file = false;
    memset(gMfocConfig.keys_file, 0, sizeof(gMfocConfig.keys_file));
    
    // Loop principale del menu
    while(true) {
        if (needRedraw) {
            // Disegna l'interfaccia del menu
            display.clearDisplay();
            common::println("MFOC Menu", 0, 0, 1, SSD1306_WHITE);
            
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
                case 0: mfoc_config(); break;  // Menu configurazione
                case 1: mfoc_run(&gMfocConfig); break; // Esecuzione attacco
                case 2: /* Caricamento config - da implementare */ break;
                case 3: /* Salvataggio config - da implementare */ break;
                case 4: return; // Uscita
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
        
        delay(10);  // Piccolo ritardo per evitare consumo CPU
    }
}

/**
 * Menu di configurazione MFOC
 * Permette di impostare tutti i parametri dell'attacco
 */
void mfoc_config() {
    const char* voci[] = {"Chiave nota", "Tipo chiave nota", "Target", "File chiavi", "Iterazioni", "Esci"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    int top = 0;
    int visibili = 4; // Ridotto a 4 voci visibili contemporaneamente per lasciare spazio
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    
    while(true) {
        if (needRedraw) {
            display.clearDisplay();
            common::println("MFOC Config", 0, 0, 1, SSD1306_WHITE);
            
            // Calcola lo spazio disponibile per la descrizione
            const int menuHeight = 46; // Altezza riservata al menu
            const int infoHeight = SCREEN_HEIGHT - menuHeight - 10; // Altezza per info
            const int itemHeight = 10; // Altezza voce del menu
            
            // Visualizza le voci del menu con scrolling
            for(int i=0; i<visibili; i++) {
                int idx = top+i;
                if(idx >= vociCount) break;
                
                if(idx == selezione) {
                    display.fillRect(0, 10+i*itemHeight, 120, itemHeight, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                
                display.setCursor(2, 10+i*itemHeight+2);
                display.print(voci[idx]);
            }
            
            // Barra di scorrimento laterale
            int barHeight = (visibili * menuHeight) / vociCount;
            int barPos = (top * menuHeight) / vociCount;
            display.fillRect(124, 10+barPos, 4, barHeight, SSD1306_WHITE);
            
            // Linea di separazione tra menu e info
            display.drawLine(0, menuHeight, 128, menuHeight, SSD1306_WHITE);
            
            // Area info con scrolling orizzontale per testo lungo
            display.setTextColor(SSD1306_WHITE);
            
            // Ottiene info contestuale
            String info = "";
            switch(selezione) {
                case 0: info = "Imposta chiave nota"; break;
                case 1: info = "Tipo chiave: ";
                        info += gMfocConfig.known_key_type == KEY_B ? "B" : "A"; break;
                case 2: info = "Settore target: ";
                        info += String(gMfocConfig.target_sector); break;
                case 3: 
                    if(gMfocConfig.load_keys_from_file) {
                        info = "File: ";
                        info += gMfocConfig.keys_file;
                    } else {
                        info = "Usa chiavi default";
                    }
                    break;
                case 4: info = "Iterazioni: ";
                        info += String(gMfocConfig.max_iterations); break;
                case 5: info = "Torna al menu MFOC"; break;
            }
            
            // Dividi l'info in più righe se necessario
            int maxCharsPerLine = 21;
            if (info.length() > maxCharsPerLine) {
                display.setCursor(0, menuHeight + 2);
                display.print(info.substring(0, maxCharsPerLine));
                display.setCursor(0, menuHeight + 12);
                display.print(info.substring(maxCharsPerLine, min(info.length(), (unsigned int)maxCharsPerLine*2)));
            } else {
                display.setCursor(0, menuHeight + 2);
                display.print(info);
            }
            
            // Istruzioni controlli sempre visibili
            display.setCursor(0, SCREEN_HEIGHT - 10);
            display.print("RST: Esci");
            display.setCursor(68, SCREEN_HEIGHT - 10); 
            display.print("SET: OK");
            
            display.display();
            needRedraw = false;
        }
        
        // Navigazione - pulsante UP
        if(digitalRead(buttonPin_UP) == LOW) {
            selezione--;
            if(selezione < 0) selezione = vociCount-1;
            
            // Aggiorna la vista se necessario
            if(selezione < top) top = selezione;
            if(selezione >= top+visibili) top = selezione-visibili+1;
            
            common::debounceButton(buttonPin_UP, 120);
            needRedraw = true;
        }
        
        // Navigazione - pulsante DOWN
        if(digitalRead(buttonPin_DWN) == LOW) {
            selezione = (selezione+1)%vociCount;
            
            // Aggiorna la vista se necessario
            if(selezione < top) top = selezione;
            if(selezione >= top+visibili) top = selezione-visibili+1;
            
            common::debounceButton(buttonPin_DWN, 120);
            needRedraw = true;
        }
        
        // Selezione - pulsante SET
        if(digitalRead(buttonPin_SET) == LOW) {
            common::debounceButton(buttonPin_SET, 120);
            
            switch(selezione) {
                case 0: mfoc_set_known_key(); break;           // Imposta chiave nota
                case 1: mfoc_set_known_key_type(); break;      // Imposta tipo chiave nota
                case 2: mfoc_set_target(); break;              // Imposta target
                case 3: mfoc_set_key_file(); break;            // Imposta file chiavi
                case 4: mfoc_set_iterations(); break;          // Imposta iterazioni
                case 5: return;  // Esci
            }
            
            needRedraw = true;
        }
        
        // Uscita con RST premuto - risposta più veloce
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            if(millis() - rstPressStart > 300) { // Solo 300ms invece di 2s
                common::debounceButton(buttonPin_RST, 50);
                return;
            }
        } else {
            rstPressStart = 0;
        }
        
        delay(10);
    }
}

/**
 * Imposta la chiave nota utilizzando la tastiera virtuale
 */
void mfoc_set_known_key() {
    char keyHex[MIFARE_KEY_SIZE * 2 + 1] = {0};
    bytes_to_hex(gMfocConfig.known_key.bytes, keyHex, MIFARE_KEY_SIZE);
    
    String keyStr = getKeyboardInput(String(keyHex), 12, "Inserisci chiave nota (HEX 12 cifre):");
    
    if(keyStr.length() == 12) {
        hex_to_bytes(keyStr.c_str(), gMfocConfig.known_key.bytes, MIFARE_KEY_SIZE);
    }
}

/**
 * Imposta il tipo di chiave nota (A/B)
 */
void mfoc_set_known_key_type() {
    bool needRedraw = true;
    unsigned long rstPressStart = 0;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Tipo chiave nota:", 0, 0, 1, SSD1306_WHITE);
            common::println(gMfocConfig.known_key_type == KEY_B ? "B" : "A", 0, 10, 1, SSD1306_WHITE);
            
            // Istruzioni di controllo in fondo allo schermo
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, SCREEN_HEIGHT - 10);
            display.print("RST: Esci");
            display.setCursor(68, SCREEN_HEIGHT - 10);
            display.print("SET: Toggle A/B");
            
            display.display();
            needRedraw = false;
        }
        
        if(digitalRead(buttonPin_SET) == LOW) {
            common::debounceButton(buttonPin_SET, 120);
            gMfocConfig.known_key_type = (gMfocConfig.known_key_type == KEY_A) ? KEY_B : KEY_A;
            needRedraw = true;
        }
        
        // Uscita con RST premuto - risposta veloce
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            if(millis() - rstPressStart > 100) {
                common::debounceButton(buttonPin_RST, 50);
                return;
            }
        } else {
            rstPressStart = 0;
        }
        
        delay(10);
    }
}

/**
 * Imposta il settore target
 */
void mfoc_set_target() {
    int s = gMfocConfig.target_sector;
    bool needRedraw = true;
    unsigned long rstPressStart = 0;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Target Sector", 0, 0, 1, SSD1306_WHITE);
            
            // Visualizza in grande il settore selezionato
            display.setTextSize(2);
            char tmp[4];
            sprintf(tmp, "%02d", s);
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(tmp, 0, 0, &x1, &y1, &w, &h);
            display.setCursor((SCREEN_WIDTH - w) / 2, 20);
            display.print(tmp);
            display.setTextSize(1);
            
            // Istruzioni di controllo in fondo allo schermo
            display.setCursor(0, SCREEN_HEIGHT - 10);
            display.print("RST: OK");
            display.setCursor(50, SCREEN_HEIGHT - 10);
            display.print("UP/DWN: Cambia");
            
            display.display();
            needRedraw = false;
        }
        
        // Incrementa il settore
        if(digitalRead(buttonPin_UP) == LOW) {
            common::debounceButton(buttonPin_UP, 120);
            s = (s+1)%16;  // Massimo 16 settori (0-15) per Mifare 1K
            needRedraw = true;
        }
        
        // Decrementa il settore
        if(digitalRead(buttonPin_DWN) == LOW) {
            common::debounceButton(buttonPin_DWN, 120);
            s = (s-1+16)%16;
            needRedraw = true;
        }
        
        // Salva ed esci
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            if(millis() - rstPressStart > 100) {
                common::debounceButton(buttonPin_RST, 50);
                gMfocConfig.target_sector = s;
                return;
            }
        } else {
            rstPressStart = 0;
        }
        
        delay(10);
    }
}

/**
 * Imposta il file delle chiavi
 */
void mfoc_set_key_file() {
    bool needRedraw = true;
    bool toggle_mode = false;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("File Chiavi", 0, 0, 1, SSD1306_WHITE);
            
            if(toggle_mode) {
                if(gMfocConfig.load_keys_from_file) {
                    common::println("Attualmente: File", 0, 12, 1, SSD1306_WHITE);
                    display.setCursor(0, 24);
                    if(strlen(gMfocConfig.keys_file) > 0) {
                        display.print(gMfocConfig.keys_file);
                    } else {
                        display.print("(Nessun file selezionato)");
                    }
                } else {
                    common::println("Attualmente: Default", 0, 12, 1, SSD1306_WHITE);
                }
                
                display.setCursor(0, 36);
                display.print("SET per toggle, RST per OK");
            } else {
                common::println("Fonte chiavi:", 0, 12, 1, SSD1306_WHITE);
                display.setCursor(0, 24);
                
                if(gMfocConfig.load_keys_from_file) {
                    display.print("1) Da file");
                    display.setCursor(0, 36);
                    display.print("2) Seleziona file");
                } else {
                    display.print("1) Default");
                    display.setCursor(0, 36);
                    display.print("2) Da file");
                }
                
                display.setCursor(0, 48);
                display.print("UP/DWN seleziona, SET conferma");
            }
            
            display.display();
            needRedraw = false;
        }
        
        if(toggle_mode) {
            // In modalità toggle, cambiamo tra file e default
            if(digitalRead(buttonPin_SET) == LOW) {
                common::debounceButton(buttonPin_SET, 120);
                gMfocConfig.load_keys_from_file = !gMfocConfig.load_keys_from_file;
                needRedraw = true;
            }
            
            if(digitalRead(buttonPin_RST) == LOW) {
                common::debounceButton(buttonPin_RST, 50);
                return;
            }
        } else {
            // In modalità selezione
            int option = 0;
            
            // Navigazione - pulsante UP
            if(digitalRead(buttonPin_UP) == LOW) {
                common::debounceButton(buttonPin_UP, 120);
                option = 0; // Opzione 1
                display.fillRect(0, 24, 3, 3, SSD1306_WHITE);
                display.fillRect(0, 36, 3, 3, SSD1306_BLACK);
                display.display();
            }
            
            // Navigazione - pulsante DOWN
            if(digitalRead(buttonPin_DWN) == LOW) {
                common::debounceButton(buttonPin_DWN, 120);
                option = 1; // Opzione 2
                display.fillRect(0, 24, 3, 3, SSD1306_BLACK);
                display.fillRect(0, 36, 3, 3, SSD1306_WHITE);
                display.display();
            }
            
            // Selezione - pulsante SET
            if(digitalRead(buttonPin_SET) == LOW) {
                common::debounceButton(buttonPin_SET, 120);
                
                if(gMfocConfig.load_keys_from_file) {
                    if(option == 0) {
                        // Manteniamo la modalità file, ma usciamo
                        return;
                    } else {
                        // Selezioniamo un nuovo file
                        if(mfoc_select_key_file(gMfocConfig.keys_file, sizeof(gMfocConfig.keys_file))) {
                            display.clearDisplay();
                            common::println("File selezionato:", 0, 0, 1, SSD1306_WHITE);
                            common::println(gMfocConfig.keys_file, 0, 12, 1, SSD1306_WHITE);
                            display.display();
                            delay(1000);
                        }
                        needRedraw = true;
                    }
                } else {
                    if(option == 0) {
                        // Manteniamo la modalità default, ma usciamo
                        return;
                    } else {
                        // Passiamo alla modalità file e selezioniamo un file
                        gMfocConfig.load_keys_from_file = true;
                        if(mfoc_select_key_file(gMfocConfig.keys_file, sizeof(gMfocConfig.keys_file))) {
                            display.clearDisplay();
                            common::println("File selezionato:", 0, 0, 1, SSD1306_WHITE);
                            common::println(gMfocConfig.keys_file, 0, 12, 1, SSD1306_WHITE);
                            display.display();
                            delay(1000);
                        }
                        needRedraw = true;
                    }
                }
            }
            
            if(digitalRead(buttonPin_RST) == LOW) {
                common::debounceButton(buttonPin_RST, 50);
                toggle_mode = true;
                needRedraw = true;
            }
        }
        
        delay(10);
    }
}

/**
 * Seleziona un file delle chiavi dal file system
 */
bool mfoc_select_key_file(char* filename, size_t max_len) {
    // Utilizziamo la nostra funzione per navigare il filesystem locale
    String selectedFile = browse_files("/");
    
    if(selectedFile.length() > 0) {
        strncpy(filename, selectedFile.c_str(), max_len - 1);
        filename[max_len - 1] = '\0';
        return true;
    }
    
    return false;
}

/**
 * Funzione per navigare nel filesystem e selezionare un file
 */
String browse_files(const char* directory) {
    File root = LittleFS.open(directory);
    if (!root || !root.isDirectory()) {
        return "";
    }

    const int MAX_FILES = 32;
    const int MAX_FILENAME = 32;
    
    struct FileEntry {
        char name[MAX_FILENAME];
        bool isDir;
    } fileList[MAX_FILES];
    
    int fileCount = 0;
    
    // Elenca i file nella directory
    File file = root.openNextFile();
    while (file && fileCount < MAX_FILES) {
        // Salva il nome del file
        strncpy(fileList[fileCount].name, file.name(), MAX_FILENAME - 1);
        fileList[fileCount].name[MAX_FILENAME - 1] = 0;
        fileList[fileCount].isDir = file.isDirectory();
        fileCount++;
        file = root.openNextFile();
    }
    
    // Se non ci sono file
    if (fileCount == 0) {
        display.clearDisplay();
        common::println("Nessun file", 0, 0, 1, SSD1306_WHITE);
        common::println("trovato", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return "";
    }
    
    // Menu di selezione del file
    int selected = 0;
    bool needRedraw = true;
    
    while (true) {
        if (needRedraw) {
            display.clearDisplay();
            common::println("Seleziona file:", 0, 0, 1, SSD1306_WHITE);
            
            int startIdx = max(0, min(selected - 2, fileCount - 5));
            for (int i = 0; i < min(5, fileCount); i++) {
                int idx = startIdx + i;
                if (idx < fileCount) {
                    if (idx == selected) {
                        display.fillRect(0, 12+i*10, 128, 10, SSD1306_WHITE);
                        display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                    } else {
                        display.setTextColor(SSD1306_WHITE);
                    }
                    
                    display.setCursor(2, 14+i*10);
                    if (fileList[idx].isDir) {
                        display.print("[DIR] ");
                    }
                    display.print(fileList[idx].name);
                }
            }
            
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 54);
            display.print("SET seleziona, RST annulla");
            display.display();
            needRedraw = false;
        }
        
        // Navigazione - pulsante UP
        if (digitalRead(buttonPin_UP) == LOW) {
            if (selected > 0) {
                selected--;
                needRedraw = true;
            }
            common::debounceButton(buttonPin_UP, 120);
        }
        
        // Navigazione - pulsante DOWN
        if (digitalRead(buttonPin_DWN) == LOW) {
            if (selected < fileCount - 1) {
                selected++;
                needRedraw = true;
            }
            common::debounceButton(buttonPin_DWN, 120);
        }
        
        // Selezione - pulsante SET
        if (digitalRead(buttonPin_SET) == LOW) {
            common::debounceButton(buttonPin_SET, 120);
            
            if (fileList[selected].isDir) {
                // Entra nella directory
                String newDir = String(directory);
                if (!newDir.endsWith("/")) {
                    newDir += "/";
                }
                newDir += fileList[selected].name;
                
                String result = browse_files(newDir.c_str());
                if (result.length() > 0) {
                    return result;
                }
                
                // Se non è stato selezionato un file, rimani nella directory corrente
                needRedraw = true;
            } else {
                // Seleziona il file
                String result = String(directory);
                if (!result.endsWith("/")) {
                    result += "/";
                }
                result += fileList[selected].name;
                return result;
            }
        }
        
        // Annulla - pulsante RST
        if (digitalRead(buttonPin_RST) == LOW) {
            common::debounceButton(buttonPin_RST, 50);
            return "";
        }
        
        delay(10);
    }
    
    return "";
}

/**
 * Imposta il numero di iterazioni
 */
void mfoc_set_iterations() {
    int iter = gMfocConfig.max_iterations;
    bool needRedraw = true;
    unsigned long rstPressStart = 0;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Iterazioni", 0, 0, 1, SSD1306_WHITE);
            
            // Visualizza in grande il valore selezionato
            display.setTextSize(2);
            char tmp[8];
            sprintf(tmp, "%d", iter);
            int16_t x1, y1;
            uint16_t w, h;
            display.getTextBounds(tmp, 0, 0, &x1, &y1, &w, &h);
            display.setCursor((SCREEN_WIDTH - w) / 2, 20);
            display.print(tmp);
            display.setTextSize(1);
            
            // Istruzioni di controllo in fondo allo schermo
            display.setCursor(0, SCREEN_HEIGHT - 10);
            display.print("RST: OK");
            display.setCursor(50, SCREEN_HEIGHT - 10);
            display.print("UP/DWN: +/-100");
            
            display.display();
            needRedraw = false;
        }
        
        // Incrementa il valore
        if(digitalRead(buttonPin_UP) == LOW) {
            common::debounceButton(buttonPin_UP, 120);
            iter += 100;
            needRedraw = true;
        }
        
        // Decrementa il valore
        if(digitalRead(buttonPin_DWN) == LOW) {
            common::debounceButton(buttonPin_DWN, 120);
            iter = (iter-100>100) ? iter-100 : 100; // Minimo 100 iterazioni
            needRedraw = true;
        }
        
        // Salva ed esci
        if(digitalRead(buttonPin_RST) == LOW) {
            if(rstPressStart == 0) rstPressStart = millis();
            if(millis() - rstPressStart > 100) {
                common::debounceButton(buttonPin_RST, 50);
                gMfocConfig.max_iterations = iter;
                return;
            }
        } else {
            rstPressStart = 0;
        }
        
        delay(10);
    }
}

/**
 * Aggiorna la barra di progresso e lo stato dell'attacco sul display
 */
void mfoc_update_progress(int progress, const char* status) {
    display.clearDisplay();
    common::println(status, 0, 0, 1, SSD1306_WHITE);
    
    // Barra progresso
    int barWidth = (progress * 128) / 100;
    display.fillRect(0, 20, barWidth, 8, SSD1306_WHITE);
    display.drawRect(0, 20, 128, 8, SSD1306_WHITE);
    
    // Percentuale
    char percent[8];
    sprintf(percent, "%d%%", progress);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(percent, 0, 0, &x1, &y1, &w, &h);
    common::println(percent, (128-w)/2, 40, 1, SSD1306_WHITE);
    
    display.display();
}

/**
 * Esegue l'attacco MFOC
 */
bool mfoc_run(MfocConfig* config, MfocCard* card) {
    // Inizializzazione esplicita del modulo NFC
    nfc.begin();
    if (!nfc.getFirmwareVersion()) {
        Serial.println("[ERROR] PN532 non trovato!");
        display.clearDisplay();
        common::println("ERROR!", 0, 0, 1, SSD1306_WHITE);
        common::println("PN532 non trovato", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return false;
    }
    nfc.SAMConfig(); // Configura il modulo NFC
    Serial.println("[INFO] Modulo NFC inizializzato correttamente");

    bool success = false;
    MfocCard localCard;
    mfoc_denonce denonce;
    mfoc_pKeys possibleKeys;
    mfoc_bKeys brokenKeys;
    
    // Se non abbiamo ricevuto una scheda, usiamo una locale
    if (card == nullptr) {
        card = &localCard;
        memset(card, 0, sizeof(MfocCard));
    }
    
    display.clearDisplay();
    common::println("MFOC Attack", 0, 0, 1, SSD1306_WHITE);
    common::println("Attendere carta...", 0, 12, 1, SSD1306_WHITE);
    display.display();
    
    // Preparazione per l'attacco
    mfoc_update_progress(0, "Inizializzazione...");
    
    // Attesa carta - timeout aumentato a 10 secondi
    Serial.println("[DEBUG] Attesa della carta RFID...");
    bool cardDetected = rfid_wait_for_tag(10000);  // 10 secondi di timeout
    Serial.println(cardDetected ? "[DEBUG] Carta rilevata!" : "[DEBUG] Timeout attesa carta!");
    
    if (!cardDetected) {
        display.clearDisplay();
        common::println("Nessuna carta", 0, 0, 1, SSD1306_WHITE);
        common::println("rilevata", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return false;
    }
    
    // Inizializza le strutture dati
    denonce.distances = (uint32_t*)malloc(DEFAULT_DIST_NR * sizeof(uint32_t));
    denonce.num_distances = DEFAULT_DIST_NR;
    denonce.tolerance = config->tolerance;
    
    possibleKeys.possibleKeys = NULL;
    possibleKeys.size = 0;
    
    brokenKeys.brokenKeys = NULL;
    brokenKeys.size = 0;
    
    // Inizializzazione della scheda con la chiave nota
    mfoc_update_progress(10, "Autenticazione...");
    card->sectors[config->target_sector].trailerBlock = get_block_number_by_sector(config->target_sector, 3);
    
    if (config->known_key_type == KEY_A) {
        memcpy(card->sectors[config->target_sector].KeyA.bytes, config->known_key.bytes, MIFARE_KEY_SIZE);
        card->sectors[config->target_sector].foundKeyA = true;
    } else {
        memcpy(card->sectors[config->target_sector].KeyB.bytes, config->known_key.bytes, MIFARE_KEY_SIZE);
        card->sectors[config->target_sector].foundKeyB = true;
    }
    
    // Carica chiavi da file se richiesto
    if (config->load_keys_from_file && strlen(config->keys_file) > 0) {
        mfoc_update_progress(20, "Caricamento chiavi...");
        if (!mfoc_load_keys_from_file(config->keys_file, &possibleKeys)) {
            display.clearDisplay();
            common::println("Errore caricamento", 0, 0, 1, SSD1306_WHITE);
            common::println("file chiavi", 0, 12, 1, SSD1306_WHITE);
            display.display();
            delay(2000);
            
            // Libera la memoria
            free(denonce.distances);
            if (possibleKeys.possibleKeys != NULL) free(possibleKeys.possibleKeys);
            if (brokenKeys.brokenKeys != NULL) free(brokenKeys.brokenKeys);
            
            return false;
        }
    }
    
    // Trova un settore di exploit
    int e_sector = mfoc_find_exploit_sector(card);
    if (e_sector == -1) {
        display.clearDisplay();
        common::println("Nessun settore di", 0, 0, 1, SSD1306_WHITE);
        common::println("exploit trovato", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        
        // Libera la memoria
        free(denonce.distances);
        if (possibleKeys.possibleKeys != NULL) free(possibleKeys.possibleKeys);
        if (brokenKeys.brokenKeys != NULL) free(brokenKeys.brokenKeys);
        
        return false;
    }
    
    // Esegue l'attacco
    mfoc_update_progress(30, "Raccolta nonce...");
    if (!mfoc_collect_nonces(card, e_sector, &denonce)) {
        display.clearDisplay();
        common::println("Errore raccolta", 0, 0, 1, SSD1306_WHITE);
        common::println("nonce", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        
        // Libera la memoria
        free(denonce.distances);
        if (possibleKeys.possibleKeys != NULL) free(possibleKeys.possibleKeys);
        if (brokenKeys.brokenKeys != NULL) free(brokenKeys.brokenKeys);
        
        return false;
    }
    
    // Recupero chiave
    mfoc_update_progress(50, "Recupero chiave...");
    if (mfoc_recover_key(card, config->target_sector, config->target_key_type, &denonce, &possibleKeys)) {
        // Chiave trovata con successo
        char keyHex[MIFARE_KEY_SIZE * 2 + 1];
        uint8_t* foundKey = (config->target_key_type == KEY_A) ? 
                           card->sectors[config->target_sector].KeyA.bytes : 
                           card->sectors[config->target_sector].KeyB.bytes;
        
        bytes_to_hex(foundKey, keyHex, MIFARE_KEY_SIZE);
        
        display.clearDisplay();
        common::println("Chiave trovata!", 0, 0, 1, SSD1306_WHITE);
        common::println(keyHex, 0, 12, 1, SSD1306_WHITE);
        display.setCursor(0, 24);
        display.print("Settore: ");
        display.print(config->target_sector);
        display.setCursor(0, 36);
        display.print("Tipo: ");
        display.print(config->target_key_type == KEY_A ? "A" : "B");
        display.display();
        
        // Salva la chiave trovata
        String result = "Settore: " + String(config->target_sector) + "\n";
        result += "Tipo chiave: " + String(config->target_key_type == KEY_A ? "A" : "B") + "\n";
        result += "Chiave: " + String(keyHex) + "\n";
        
        // Genera un nome file unico basato sul timestamp
        String filename = "/mfoc_key_" + String(millis()) + ".txt";
        File file = LittleFS.open(filename, "w");
        if (file) {
            file.println(result);
            file.close();
            
            display.setCursor(0, 48);
            display.print("Salvato: ");
            display.print(filename);
            display.display();
        }
        
        success = true;
        delay(3000);
    } else {
        display.clearDisplay();
        common::println("Chiave non trovata", 0, 0, 1, SSD1306_WHITE);
        common::println("Riprovare", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
    }
    
    // Libera la memoria
    free(denonce.distances);
    if (possibleKeys.possibleKeys != NULL) free(possibleKeys.possibleKeys);
    if (brokenKeys.brokenKeys != NULL) free(brokenKeys.brokenKeys);
    
    return success;
}

/**
 * Trova un settore di exploit (settore con almeno una chiave conosciuta)
 * @return L'indice del settore di exploit, o -1 se non trovato
 */
int mfoc_find_exploit_sector(MfocCard* card) {
    // In questa simulazione, utilizziamo il settore con la chiave già nota
    return 0; // Per ora ritorniamo il settore 0, in una implementazione reale andrebbe calcolato
}

/**
 * Carica le chiavi da un file
 */
bool mfoc_load_keys_from_file(const char* filename, mfoc_pKeys* keys) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Errore apertura file delle chiavi");
        return false;
    }
    
    // Conta le linee
    int numLines = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() >= 12) { // 12 caratteri HEX per una chiave a 6 byte
            numLines++;
        }
    }
    
    // Resetta il file
    file.seek(0);
    
    // Alloca memoria per le chiavi
    keys->size = numLines;
    keys->possibleKeys = (uint64_t*)malloc(numLines * sizeof(uint64_t));
    if (keys->possibleKeys == NULL) {
        file.close();
        return false;
    }
    
    // Legge le chiavi
    int i = 0;
    while (file.available() && i < numLines) {
        String line = file.readStringUntil('\n');
        if (line.length() >= 12) { // 12 caratteri HEX per una chiave a 6 byte
            // Converti da HEX a uint64_t
            char keyStr[13];
            strncpy(keyStr, line.c_str(), 12);
            keyStr[12] = '\0';
            
            uint8_t keyBytes[6];
            hex_to_bytes(keyStr, keyBytes, 6);
            
            keys->possibleKeys[i++] = bytes_to_num(keyBytes, 6);
        }
    }
    
    file.close();
    return true;
}

/**
 * Converte un array di byte in un numero a 64 bit
 * Già definita altrove, quindi commentata qui
 */
/*
uint64_t bytes_to_num(uint8_t* src, uint32_t len) {
    uint64_t num = 0;
    while (len--) {
        num = (num << 8) | (*src);
        src++;
    }
    return num;
}
*/

/**
 * Converte un numero a 64 bit in un array di byte
 */
void num_to_bytes(uint64_t n, uint32_t len, uint8_t* dest) {
    while (len--) {
        dest[len] = (uint8_t) n;
        n >>= 8;
    }
}

/**
 * Raccoglie nonce per l'attacco
 * Simulazione - in un'implementazione reale, questa funzione comunicherebbe con la carta
 */
bool mfoc_collect_nonces(MfocCard* card, uint8_t sector, mfoc_denonce* d) {
    uint32_t last_nonce = 0;
    
    for (int i = 0; i < d->num_distances; i++) {
        // Simulazione di generazione di nonce
        uint32_t new_nonce;
        if (i == 0) {
            new_nonce = random(0xFFFFFFFF);
        } else {
            // Genera una distanza plausibile
            new_nonce = last_nonce + random(100, 10000);
        }
        
        // Controllo per interruzione utente
        if (digitalRead(buttonPin_RST) == LOW) {
            return false;
        }
        
        // Aggiorna lo stato
        char status[32];
        sprintf(status, "Nonce %d di %d...", i+1, (int)d->num_distances);
        mfoc_update_progress(30 + (i * 20 / d->num_distances), status);
        
        // Memorizza il nonce
        if (i > 0) {
            d->distances[i-1] = new_nonce - last_nonce;
        }
        
        last_nonce = new_nonce;
        delay(50); // Simulazione di comunicazione con la carta
    }
    
    // Calcola la mediana delle distanze
    d->median = mfoc_median(d);
    
    return true;
}

/**
 * Calcola la mediana delle distanze tra nonce
 */
uint32_t mfoc_median(mfoc_denonce* d) {
    // Ordina le distanze
    qsort(d->distances, d->num_distances - 1, sizeof(uint32_t), mfoc_compare_keys);
    
    // Calcola la mediana
    int middle = (int)(d->num_distances - 1) / 2;
    if ((d->num_distances - 1) % 2 == 1) {
        // Numero dispari di elementi
        return d->distances[middle];
    } else {
        // Numero pari di elementi, restituisce il valore inferiore
        return (uint32_t)(d->distances[middle]);
    }
}

/**
 * Funzione di confronto per qsort
 */
int mfoc_compare_keys(const void* a, const void* b) {
    uint32_t valueA = *(uint32_t*)a;
    uint32_t valueB = *(uint32_t*)b;
    if (valueA == valueB) return 0;
    if (valueA < valueB) return -1;
    return 1;
}

/**
 * Recupera la chiave basandosi sulle distanze dei nonce e sulle chiavi possibili
 * Simulazione - in un'implementazione reale, questa funzione utilizzerebbe l'algoritmo di crypto1
 */
bool mfoc_recover_key(MfocCard* card, uint8_t sector, uint8_t key_type, mfoc_denonce* d, mfoc_pKeys* pk) {
    // Simula il recupero della chiave
    uint8_t foundKey[6];
    bool success = false;
    
    // Simuliamo un tentativo con le chiavi predefinite
    for (int i = 0; i < mfoc_default_keys_count; i++) {
        mfoc_update_progress(50 + (i * 40 / mfoc_default_keys_count), "Prova chiavi...");
        
        // Simulazione di tentativo con la chiave
        memcpy(foundKey, mfoc_default_keys[i], 6);
        
        // Simula l'autenticazione
        if (random(100) < 10) { // 10% di probabilità di successo (solo per demo)
            success = true;
            break;
        }
        
        delay(50); // Simulazione di comunicazione con la carta
        
        // Controllo per interruzione utente
        if (digitalRead(buttonPin_RST) == LOW) {
            return false;
        }
    }
    
    // Se abbiamo trovato la chiave, salviamola nella struttura della carta
    if (success) {
        if (key_type == KEY_A) {
            memcpy(card->sectors[sector].KeyA.bytes, foundKey, MIFARE_KEY_SIZE);
            card->sectors[sector].foundKeyA = true;
        } else {
            memcpy(card->sectors[sector].KeyB.bytes, foundKey, MIFARE_KEY_SIZE);
            card->sectors[sector].foundKeyB = true;
        }
    }
    
    return success;
}

/**
 * Verifica se un nonce è valido
 */
bool mfoc_valid_nonce(uint32_t Nt, uint32_t NtEnc, uint32_t Ks1, uint8_t* parity) {
    // In un'implementazione reale, questa funzione verificherebbe la validità del nonce
    // usando la parità e altre proprietà
    return true;
}

/**
 * Raggruppa e ordina le chiavi possibili
 */
mfoc_countKeys* mfoc_uniqsort_keys(uint64_t* possibleKeys, uint32_t size) {
    unsigned int i, j = 0;
    int count = 0;
    mfoc_countKeys* our_counts;
    
    // Ordina le chiavi
    qsort(possibleKeys, size, sizeof(uint64_t), mfoc_compare_keys);
    
    our_counts = (mfoc_countKeys*)calloc(size, sizeof(mfoc_countKeys));
    if (our_counts == NULL) {
        Serial.println("Errore allocazione memoria");
        return NULL;
    }
    
    for (i = 0; i < size; i++) {
        if (possibleKeys[i + 1] == possibleKeys[i]) {
            count++;
        } else {
            our_counts[j].key = possibleKeys[i];
            our_counts[j].count = count;
            j++;
            count = 0;
        }
    }
    
    // Ordina per conteggio
    qsort(our_counts, j, sizeof(mfoc_countKeys), 
          [](const void* a, const void* b) -> int { 
              return ((mfoc_countKeys*)b)->count - ((mfoc_countKeys*)a)->count; 
          });
    
    return our_counts;
}

/**
 * Menu principale MFOC Modificato
 * Versione alternativa con funzionalità estese
 */
void mfoc_menu_mod() {
    const char* voci[] = {"Configura", "Esegui", "Carica config", "Salva config", "Esci"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    
    // Imposta la configurazione predefinita
    gMfocConfig.known_key_type = KEY_A;
    gMfocConfig.target_key_type = KEY_A;
    gMfocConfig.target_sector = 0;
    gMfocConfig.max_iterations = 1000;
    gMfocConfig.num_probes = DEFAULT_PROBES_NR;
    gMfocConfig.sets = DEFAULT_SETS_NR;
    gMfocConfig.tolerance = DEFAULT_TOLERANCE;
    gMfocConfig.load_keys_from_file = false;
    memset(gMfocConfig.keys_file, 0, sizeof(gMfocConfig.keys_file));
    
    // Loop principale del menu
    while(true) {
        if (needRedraw) {
            // Disegna l'interfaccia del menu
            display.clearDisplay();
            common::println("MFOC Menu Mod", 0, 0, 1, SSD1306_WHITE);
            
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
                case 0: 
                    // Menu configurazione mod - per ora usa quello standard
                    mfoc_config(); 
                    break;
                case 1: 
                    // Esecuzione attacco - per ora usa quello standard
                    mfoc_run(&gMfocConfig); 
                    break;
                case 2: 
                    // Caricamento config - da implementare
                    display.clearDisplay();
                    common::println("Funzione non", 0, 0, 1, SSD1306_WHITE);
                    common::println("ancora implementata", 0, 12, 1, SSD1306_WHITE);
                    display.display();
                    delay(2000);
                    break;
                case 3: 
                    // Salvataggio config - da implementare
                    display.clearDisplay();
                    common::println("Funzione non", 0, 0, 1, SSD1306_WHITE);
                    common::println("ancora implementata", 0, 12, 1, SSD1306_WHITE);
                    display.display();
                    delay(2000);
                    break;
                case 4: 
                    return; // Uscita
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
        
        delay(10);  // Piccolo ritardo per evitare consumo CPU
    }
}
