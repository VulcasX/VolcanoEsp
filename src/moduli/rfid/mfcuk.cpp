/**
 * MFCUK - MiFare Classic Universal toolKit
 * Implementazione per ESP32 con display OLED
 * 
 * Basato su:
 * - mfoc (https://github.com/nfc-tools/mfoc)
 * - FlipperMfkey (https://github.com/noproto/FlipperMfkey)
 * - MFRC522_nested_attack (https://github.com/HakonHystad/MFRC522_nested_attack)
 * 
 * Questa implementazione è ottimizzata per ESP32 e utilizza il threading per accelerare
 * il processo di recupero delle chiavi.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <Adafruit_SSD1306.h>
#include "mfcuk.h"
#include "mfcuk_attack.h"
#include "mfcuk_types.h"
#include "mfcuk_utils.h"
#include "mfcuk_crypto.h"
#include "rfid.h"
#include "../../lib/input/input.h"
#include "../../core/common/virtualkeyboard.h"
#include "../../core/common/common.h"

// Riferimento al display OLED
extern Adafruit_SSD1306 display;

// Configurazione globale per MFCUK
MfcukConfig gConfig;

// Tempo di aggiornamento della descrizione (ms)
unsigned long mfcuk_desc_update_time = 3000; // default 3s

// File di configurazione
const char* CONFIG_FILE = "/mfcuk.cfg";

/**
 * Menu principale MFCUK
 * Gestisce la navigazione e la selezione delle opzioni
 */
void mfcuk_menu() {
    const char* voci[] = {"Configura", "Esegui", "Carica config", "Salva config", "Esci"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    
    // Imposta la configurazione predefinita
    set_default_config(&gConfig);
    
    // Loop principale del menu
    while(true) {
        if (needRedraw) {
            // Disegna l'interfaccia del menu
            display.clearDisplay();
            common::println("MFCUK Menu", 0, 0, 1, SSD1306_WHITE);
            
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
                case 0: mfcuk_config(); break;  // Menu configurazione
                case 1: mfcuk_run(&gConfig); break; // Esecuzione attacco
                case 2: load_config_from_fs(&gConfig); break; // Caricamento config
                case 3: save_config_to_fs(&gConfig); break; // Salvataggio config
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
 * Menu di configurazione MFCUK
 * Permette di impostare tutti i parametri dell'attacco
 */
void mfcuk_config() {
    const char* voci[] = {"Set Key", "Key Type", "Target", "Mode", "Timing", "Tempo descr.", "Esci"};
    const int vociCount = sizeof(voci)/sizeof(voci[0]);
    int selezione = 0;
    int top = 0;
    int visibili = 5; // Numero di voci visibili contemporaneamente
    unsigned long rstPressStart = 0;
    bool needRedraw = true;
    
    while(true) {
        if (needRedraw) {
            display.clearDisplay();
            common::println("MFCUK Config", 0, 0, 1, SSD1306_WHITE);
            
            // Visualizza le voci del menu con scrolling
            for(int i=0; i<visibili; i++) {
                int idx = top+i;
                if(idx >= vociCount) break;
                
                if(idx == selezione) {
                    display.fillRect(0, 10+i*12, 120, 12, SSD1306_WHITE);
                    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
                } else {
                    display.setTextColor(SSD1306_WHITE);
                }
                
                display.setCursor(2, 12+i*12);
                display.print(voci[idx]);
            }
            
            // Barra di scorrimento laterale
            int barHeight = (visibili * 60) / vociCount;
            int barPos = (top * 60) / vociCount;
            display.fillRect(124, 12+barPos, 4, barHeight, SSD1306_WHITE);
            
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0, 10+visibili*12+2);
            display.print("RST >2s per uscire");
            
            // Descrizione contestuale dell'opzione selezionata
            display.setCursor(0, 64-10);
            switch(selezione) {
                case 0: display.print("Imposta la chiave HEX"); break;
                case 1: display.print("Tipo chiave A/B"); break;
                case 2: display.print("Settore e blocco target"); break;
                case 3: display.print("Modalita' attacco"); break;
                case 4: display.print("Timing e parametri"); break;
                case 5: display.print("T. update: ");
                         display.print(mfcuk_desc_update_time/1000);
                         display.print("s"); break;
                case 6: display.print("Torna al menu MFCUK"); break;
            }
            
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
                case 0: mfcuk_set_key(); break;           // Imposta chiave
                case 1: mfcuk_set_key_type(); break;      // Imposta tipo chiave
                case 2: mfcuk_set_target(); break;        // Imposta target
                case 3: mfcuk_set_mode(); break;          // Imposta modalità
                case 4: mfcuk_set_timing(); break;        // Imposta timing
                case 5: {                                 // Imposta tempo update descrizione
                    String newTime = getKeyboardInput(
                        String(mfcuk_desc_update_time/1000),
                        2,
                        "Tempo update (s):"
                    );
                    int t = newTime.toInt();
                    if(t > 0) mfcuk_desc_update_time = t*1000;
                    break;
                }
                case 6: return;  // Esci
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

/**
 * Imposta la chiave HEX utilizzando la tastiera virtuale
 */
void mfcuk_set_key() {
    char keyHex[MIFARE_KEY_SIZE * 2 + 1] = {0};
    bytes_to_hex(gConfig.known_key.bytes, keyHex, MIFARE_KEY_SIZE);
    
    String keyStr = getKeyboardInput(String(keyHex), 12, "Inserisci chiave HEX (12 cifre):");
    
    if(keyStr.length() == 12) {
        hex_to_bytes(keyStr.c_str(), gConfig.known_key.bytes, MIFARE_KEY_SIZE);
    }
}

/**
 * Imposta il tipo di chiave (A/B)
 */
void mfcuk_set_key_type() {
    bool needRedraw = true;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Key Type:", 0, 0, 1, SSD1306_WHITE);
            common::println(gConfig.target_key_type == KEY_B ? "B" : "A", 0, 10, 1, SSD1306_WHITE);
            display.setCursor(0, 32);
            display.print("SET per toggle, RST per uscire");
            display.display();
            needRedraw = false;
        }
        
        if(digitalRead(buttonPin_SET) == LOW) {
            common::debounceButton(buttonPin_SET, 120);
            gConfig.target_key_type = (gConfig.target_key_type == KEY_A) ? KEY_B : KEY_A;
            needRedraw = true;
        }
        
        if(digitalRead(buttonPin_RST) == LOW) {
            common::debounceButton(buttonPin_RST, 50);
            return;
        }
        
        delay(10);
    }
}

/**
 * Imposta il settore e il blocco target
 */
void mfcuk_set_target() {
    int s = gConfig.target_sector;
    bool needRedraw = true;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Target Sector", 0, 0, 1, SSD1306_WHITE);
            display.setCursor(0, 12);
            display.print("Settore: "); display.print(s);
            display.setCursor(0, 40);
            display.print("UP/DWN cambia, RST per uscire");
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
            common::debounceButton(buttonPin_RST, 50);
            gConfig.target_sector = s;
            return;
        }
        
        delay(10);
    }
}

/**
 * Imposta la modalità di attacco (Darkside/Nested)
 */
void mfcuk_set_mode() {
    bool needRedraw = true;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Attack Mode", 0, 0, 1, SSD1306_WHITE);
            
            // Darkside option
            if(gConfig.mode == ATTACK_MODE_DARKSIDE) {
                common::println("* ", 0, 10, 1, SSD1306_WHITE);
            } else {
                common::println("  ", 0, 10, 1, SSD1306_WHITE);
            }
            common::println("Darkside", 20, 10, 1, SSD1306_WHITE);
            
            // Nested option
            if(gConfig.mode == ATTACK_MODE_NESTED) {
                common::println("* ", 0, 20, 1, SSD1306_WHITE);
            } else {
                common::println("  ", 0, 20, 1, SSD1306_WHITE);
            }
            common::println("Nested", 20, 20, 1, SSD1306_WHITE);
            
            display.setCursor(0, 40);
            display.print("UP/DWN per cambiare, RST per uscire");
            display.display();
            needRedraw = false;
        }
        
        // Seleziona Darkside
        if(digitalRead(buttonPin_UP) == LOW) {
            common::debounceButton(buttonPin_UP, 50);
            gConfig.mode = ATTACK_MODE_DARKSIDE;
            needRedraw = true;
        }
        
        // Seleziona Nested
        else if(digitalRead(buttonPin_DWN) == LOW) {
            common::debounceButton(buttonPin_DWN, 50);
            gConfig.mode = ATTACK_MODE_NESTED;
            needRedraw = true;
        }
        
        // Esci
        else if(digitalRead(buttonPin_RST) == LOW) {
            common::debounceButton(buttonPin_RST, 50);
            return;
        }
        
        delay(10);
    }
}

/**
 * Imposta i parametri di timing
 */
void mfcuk_set_timing() {
    int iter = gConfig.max_iterations;
    bool needRedraw = true;
    
    while(true) {
        if(needRedraw) {
            display.clearDisplay();
            common::println("Max Iterations", 0, 0, 1, SSD1306_WHITE);
            display.setCursor(0, 12);
            display.print("Iterazioni: "); display.print(iter);
            display.setCursor(0, 40);
            display.print("UP/DWN cambia, RST per uscire");
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
            common::debounceButton(buttonPin_RST, 50);
            gConfig.max_iterations = iter;
            return;
        }
        
        delay(10);
    }
}

/**
 * Aggiorna la barra di progresso e lo stato dell'attacco sul display
 */
void mfcuk_update_progress(int progress, const char* status) {
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
 * Salva il risultato dell'attacco su file
 */
void mfcuk_save_result(const String& result) {
    // Genera un nome file unico basato sul timestamp
    String filename = "/mfcuk_result_" + String(millis()) + ".txt";
    File file = LittleFS.open(filename, "w");
    
    if(!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    
    file.println(result);
    file.close();
    
    display.clearDisplay();
    common::println("Result saved:", 0, 0, 1, SSD1306_WHITE);
    common::println(filename.c_str(), 0, 10, 1, SSD1306_WHITE);
    display.display();
    delay(2000);
}

/**
 * Esegue l'attacco MFCUK
 * Questa funzione è il punto di ingresso dell'interfaccia utente per gli attacchi
 */
bool mfcuk_run(MfcukConfig* config, uint8_t* key_out) {
    bool success = false;
    uint8_t local_key[MIFARE_KEY_SIZE] = {0};
    char keyHex[MIFARE_KEY_SIZE * 2 + 1];
    
    // Se key_out è nullptr, utilizziamo un buffer locale
    uint8_t* key = (key_out != nullptr) ? key_out : local_key;
    
    display.clearDisplay();
    common::println("MFCUK Attack", 0, 0, 1, SSD1306_WHITE);
    common::println("Attendere carta...", 0, 12, 1, SSD1306_WHITE);
    display.display();
    
    // Preparazione per l'attacco
    mfcuk_update_progress(0, "Inizializzazione...");
    
    // Attesa carta
    if (!rfid_wait_for_tag(5000)) {
        common::println("Nessuna carta", 0, 0, 1, SSD1306_WHITE);
        common::println("rilevata", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return false;
    }
    
    // Esegue l'attacco selezionato
    switch(config->mode) {
        case ATTACK_MODE_DARKSIDE:
            mfcuk_update_progress(10, "Darkside attack...");
            success = mfcuk_darkside_attack(config, key);
            break;
        
        case ATTACK_MODE_NESTED:
            mfcuk_update_progress(10, "Nested attack...");
            success = mfcuk_nested_attack(config, key);
            break;
        
        default:
            mfcuk_update_progress(100, "Modalità non supportata");
            delay(2000);
            return false;
    }
    
    // Mostra risultato
    if (success) {
        bytes_to_hex(key, keyHex, MIFARE_KEY_SIZE);
        
        display.clearDisplay();
        common::println("Chiave trovata!", 0, 0, 1, SSD1306_WHITE);
        common::println(keyHex, 0, 12, 1, SSD1306_WHITE);
        display.display();
        
        // Salva il risultato
        String result = "Settore: " + String(config->target_sector) + "\n";
        result += "Tipo chiave: " + String(config->target_key_type == KEY_A ? "A" : "B") + "\n";
        result += "Chiave: " + String(keyHex) + "\n";
        mfcuk_save_result(result);
    } else {
        display.clearDisplay();
        common::println("Chiave non trovata", 0, 0, 1, SSD1306_WHITE);
        common::println("Riprovare", 0, 12, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
    }
    
    return success;
}

// L'attacco Darkside è implementato in mfcuk_attack.cpp
bool mfcuk_darkside_attack(MfcukConfig* config, uint8_t* key);

// L'attacco Nested è implementato in mfcuk_attack.cpp
bool mfcuk_nested_attack(MfcukConfig* config, uint8_t* key);
