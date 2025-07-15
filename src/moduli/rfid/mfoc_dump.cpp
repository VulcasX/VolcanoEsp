#include "mfoc.h"
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "core/common/common.h"
#include "moduli/rfid/rfid.h"
#include "moduli/rfid/mfoc_keys.h"
#include <input.h>
#include "core/littlefs/littlefs.h"
#include <FS.h>
#include <LittleFS.h>
#include <time.h>

// Define per compatibilità
#define MifareKey MfocKey
#define MFOC_KEY_TYPE_A 0
#define MFOC_KEY_TYPE_B 1

// Riferimento al display OLED
extern Adafruit_SSD1306 display;
extern Extended_PN532 nfc;

/**
 * Funzione principale per il dump completo di una carta MIFARE Classic
 * Questa funzione esegue:
 * 1. Lettura dell'UID della carta
 * 2. Tentativo di recupero di tutte le chiavi A e B di tutti i settori
 * 3. Salvataggio delle chiavi trovate in un file
 * 4. Dump completo della carta nei formati .mfd (binario) e .txt (testo)
 */
void mfoc_dump_complete() {
    display.clearDisplay();
    common::println("MFOC Dump Completo", 0, 0, 1, SSD1306_WHITE);
    common::println("Avvicinare la carta", 0, 16, 1, SSD1306_WHITE);
    common::println("RST: Annulla", 0, 54, 1, SSD1306_WHITE);
    display.display();

    // Variabili per gestire il pulsante RST
    unsigned long rstPressStart = 0;
    
    // Attendi che l'utente posizioni la carta o prema RST per uscire
    while (true) {
        if (digitalRead(buttonPin_RST) == LOW) {
            if (rstPressStart == 0) rstPressStart = millis();
            if (millis() - rstPressStart > 100) {
                common::debounceButton(buttonPin_RST, 50);
                return;
            }
        } else {
            rstPressStart = 0;
        }

        // Controlla se c'è una carta presente
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
        uint8_t uidLength;
        
        if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
            // Carta rilevata, inizia il dump
            display.clearDisplay();
            common::println("Carta rilevata", 0, 0, 1, SSD1306_WHITE);
            char uidStr[32];
            sprintf(uidStr, "UID: ");
            for (uint8_t i = 0; i < uidLength; i++) {
                char byte[3];
                sprintf(byte, "%02X", uid[i]);
                strcat(uidStr, byte);
                if (i < uidLength - 1) strcat(uidStr, ":");
            }
            common::println(uidStr, 0, 16, 1, SSD1306_WHITE);
            display.display();
            delay(1000);
            
            // Inizializza la struttura della carta
            MfocCard card;
            memset(&card, 0, sizeof(MfocCard));
            
            // Converti l'UID in un formato numerico
            card.uid = (uint32_t)uid[0] << 24 | (uint32_t)uid[1] << 16 | (uint32_t)uid[2] << 8 | (uint32_t)uid[3];
            
            // Determina il tipo di carta (numero di settori)
            // Assumiamo che sia una MIFARE Classic da 1K (16 settori)
            card.num_sectors = 16;
            
            // Esegui il dump completo
            if (mfoc_run_complete_dump(&card)) {
                display.clearDisplay();
                common::println("Dump completato", 0, 0, 1, SSD1306_WHITE);
                common::println("con successo!", 0, 16, 1, SSD1306_WHITE);
                common::println("Premere un tasto", 0, 40, 1, SSD1306_WHITE);
                common::println("per continuare", 0, 54, 1, SSD1306_WHITE);
                display.display();
            } else {
                display.clearDisplay();
                common::println("Errore durante", 0, 0, 1, SSD1306_WHITE);
                common::println("il dump", 0, 16, 1, SSD1306_WHITE);
                common::println("Premere un tasto", 0, 40, 1, SSD1306_WHITE);
                common::println("per continuare", 0, 54, 1, SSD1306_WHITE);
                display.display();
            }
            
            // Attendi che l'utente prema un tasto
            while (digitalRead(buttonPin_SET) == HIGH && 
                   digitalRead(buttonPin_UP) == HIGH && 
                   digitalRead(buttonPin_DWN) == HIGH && 
                   digitalRead(buttonPin_RST) == HIGH) {
                delay(10);
            }
            
            // Debounce per il pulsante premuto
            while (digitalRead(buttonPin_SET) == LOW || 
                   digitalRead(buttonPin_UP) == LOW || 
                   digitalRead(buttonPin_DWN) == LOW || 
                   digitalRead(buttonPin_RST) == LOW) {
                delay(10);
            }
            
            return;
        }
        
        delay(100);
    }
}

/**
 * Esegue il dump completo della carta, recuperando tutte le chiavi
 * @param card Puntatore alla struttura della carta
 * @return true se il dump è stato completato con successo, false altrimenti
 */
bool mfoc_run_complete_dump(MfocCard* card) {
    display.clearDisplay();
    common::println("Recupero chiavi...", 0, 0, 1, SSD1306_WHITE);
    display.display();
    
    // Utilizziamo una chiave nota per iniziare
    // La chiave di fabbrica FFFFFFFFFFFF è spesso presente nei settori nuovi
    MifareKey defaultKey;
    for (int i = 0; i < 6; i++) {
        defaultKey.bytes[i] = 0xFF;
    }
    
    // Configurazione per il recupero delle chiavi
    MfocConfig config;
    config.known_key_type = 0;  // 0 = Chiave A
    memcpy(&config.known_key, &defaultKey, sizeof(MifareKey));
    config.max_iterations = 2000;  // Più iterazioni per aumentare le probabilità di successo
    config.num_probes = 15;
    config.sets = 1;
    config.tolerance = 20;
    config.load_keys_from_file = false;

    // Array per tenere traccia dei settori già processati
    bool processed_sectors[MIFARE_MAXSECTOR] = {false};
    bool found_at_least_one_key = false;
    
    // Primo tentativo con la chiave di fabbrica
    for (uint8_t sector = 0; sector < card->num_sectors; sector++) {
        display.clearDisplay();
        char statusMsg[32];
        sprintf(statusMsg, "Settore %d/16", sector);
        common::println("Recupero chiavi...", 0, 0, 1, SSD1306_WHITE);
        common::println(statusMsg, 0, 16, 1, SSD1306_WHITE);
        common::println("Chiave: FFFFFFFFFFFF", 0, 32, 1, SSD1306_WHITE);
        display.display();
        
        // Prova con chiave A
        config.target_sector = sector;
        config.target_key_type = MFOC_KEY_TYPE_A; // 0 = Chiave A
        
        if (mfoc_run(&config, card)) {
            processed_sectors[sector] = true;
            found_at_least_one_key = true;
        }
        
        // Prova con chiave B
        config.target_key_type = MFOC_KEY_TYPE_B; // 1 = Chiave B
        if (mfoc_run(&config, card)) {
            processed_sectors[sector] = true;
            found_at_least_one_key = true;
        }
    }
    
    // Se abbiamo trovato almeno una chiave, possiamo usarla per trovare le altre
    if (found_at_least_one_key) {
        // Cicla attraverso tutti i settori per trovare quelli senza chiavi
        for (uint8_t exploit_sector = 0; exploit_sector < card->num_sectors; exploit_sector++) {
            // Se questo settore ha una chiave trovata, usala come chiave nota
            if (processed_sectors[exploit_sector]) {
                if (card->sectors[exploit_sector].foundKeyA) {
                    memcpy(&config.known_key, &card->sectors[exploit_sector].KeyA, sizeof(MifareKey));
                    config.known_key_type = 0; // MFOC_KEY_TYPE_A
                } else if (card->sectors[exploit_sector].foundKeyB) {
                    memcpy(&config.known_key, &card->sectors[exploit_sector].KeyB, sizeof(MifareKey));
                    config.known_key_type = 1; // MFOC_KEY_TYPE_B
                } else {
                    continue;  // Non dovrebbe accadere
                }
                
                // Usa questa chiave nota per trovare le altre chiavi
                for (uint8_t target_sector = 0; target_sector < card->num_sectors; target_sector++) {
                    // Salta i settori già processati
                    if (processed_sectors[target_sector] && 
                        card->sectors[target_sector].foundKeyA && 
                        card->sectors[target_sector].foundKeyB) {
                        continue;
                    }
                    
                    display.clearDisplay();
                    char statusMsg[32];
                    sprintf(statusMsg, "Settore %d -> %d", exploit_sector, target_sector);
                    common::println("Recupero chiavi...", 0, 0, 1, SSD1306_WHITE);
                    common::println(statusMsg, 0, 16, 1, SSD1306_WHITE);
                    display.display();
                    
                    // Prova a recuperare la chiave A se non è ancora stata trovata
                    if (!card->sectors[target_sector].foundKeyA) {
                        config.target_sector = target_sector;
                        config.target_key_type = 0; // 0 = Chiave A
                        if (mfoc_run(&config, card)) {
                            processed_sectors[target_sector] = true;
                        }
                    }
                    
                    // Prova a recuperare la chiave B se non è ancora stata trovata
                    if (!card->sectors[target_sector].foundKeyB) {
                        config.target_sector = target_sector;
                        config.target_key_type = 1; // 1 = Chiave B
                        if (mfoc_run(&config, card)) {
                            processed_sectors[target_sector] = true;
                        }
                    }
                }
            }
        }
    }
    
    // Verifica quante chiavi sono state trovate
    int keys_found = 0;
    for (uint8_t sector = 0; sector < card->num_sectors; sector++) {
        if (card->sectors[sector].foundKeyA) keys_found++;
        if (card->sectors[sector].foundKeyB) keys_found++;
    }
    
    // Mostra il risultato
    display.clearDisplay();
    char resultMsg[32];
    sprintf(resultMsg, "Trovate %d/%d chiavi", keys_found, card->num_sectors * 2);
    common::println("Recupero completato", 0, 0, 1, SSD1306_WHITE);
    common::println(resultMsg, 0, 16, 1, SSD1306_WHITE);
    display.display();
    delay(2000);
    
    // Salva le chiavi trovate
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char filename[64];
    sprintf(filename, "/keys_%02d%02d%02d_%02d%02d%02d.txt", 
            timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    display.clearDisplay();
    common::println("Salvataggio chiavi...", 0, 0, 1, SSD1306_WHITE);
    common::println(filename, 0, 16, 1, SSD1306_WHITE);
    display.display();
    
    if (!mfoc_save_keys(card, filename)) {
        display.clearDisplay();
        common::println("Errore salvataggio", 0, 0, 1, SSD1306_WHITE);
        common::println("chiavi", 0, 16, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return false;
    }
    
    // Salva il dump completo
    char mfd_filename[64];
    char txt_filename[64];
    sprintf(mfd_filename, "/dump_%02d%02d%02d_%02d%02d%02d.mfd", 
            timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    sprintf(txt_filename, "/dump_%02d%02d%02d_%02d%02d%02d.txt", 
            timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    display.clearDisplay();
    common::println("Salvataggio dump...", 0, 0, 1, SSD1306_WHITE);
    common::println(mfd_filename, 0, 16, 1, SSD1306_WHITE);
    display.display();
    
    if (!mfoc_save_dump(card, mfd_filename, txt_filename)) {
        display.clearDisplay();
        common::println("Errore salvataggio", 0, 0, 1, SSD1306_WHITE);
        common::println("dump", 0, 16, 1, SSD1306_WHITE);
        display.display();
        delay(2000);
        return false;
    }
    
    return true;
}

/**
 * Salva le chiavi trovate in un file
 * @param card Puntatore alla struttura della carta
 * @param filename Nome del file in cui salvare le chiavi
 * @return true se il salvataggio è riuscito, false altrimenti
 */
bool mfoc_save_keys(MfocCard* card, const char* filename) {
    // Apri il file in scrittura
    File keyFile = LittleFS.open(filename, "w");
    if (!keyFile) {
        return false;
    }
    
    // Scrivi l'intestazione
    keyFile.println("# MFOC Dump Completo - Chiavi recuperate");
    keyFile.println("# Formato: <settore>:<tipo>:<chiave>");
    keyFile.println();
    
    // Scrivi le chiavi trovate
    for (uint8_t sector = 0; sector < card->num_sectors; sector++) {
        if (card->sectors[sector].foundKeyA) {
            keyFile.printf("%d:A:", sector);
            for (int i = 0; i < 6; i++) {
                keyFile.printf("%02X", card->sectors[sector].KeyA.bytes[i]);
            }
            keyFile.println();
        }
        
        if (card->sectors[sector].foundKeyB) {
            keyFile.printf("%d:B:", sector);
            for (int i = 0; i < 6; i++) {
                keyFile.printf("%02X", card->sectors[sector].KeyB.bytes[i]);
            }
            keyFile.println();
        }
    }
    
    keyFile.close();
    return true;
}

/**
 * Salva il dump completo della carta in formato MFD (binario) e TXT (testo)
 * @param card Puntatore alla struttura della carta
 * @param mfd_filename Nome del file MFD in cui salvare il dump binario
 * @param txt_filename Nome del file TXT in cui salvare il dump in formato testo
 * @return true se il salvataggio è riuscito, false altrimenti
 */
bool mfoc_save_dump(MfocCard* card, const char* mfd_filename, const char* txt_filename) {
    uint8_t dump_data[16][64];  // Buffer per i dati dei settori
    memset(dump_data, 0, sizeof(dump_data));
    
    // Crea uid_bytes una sola volta per tutto il metodo
    uint8_t uid_bytes[4];
    uid_bytes[0] = (card->uid >> 24) & 0xFF;
    uid_bytes[1] = (card->uid >> 16) & 0xFF;
    uid_bytes[2] = (card->uid >> 8) & 0xFF;
    uid_bytes[3] = card->uid & 0xFF;
    uint8_t uid_length = 4;  // Lunghezza UID 4 byte
    
    // Per ogni settore, leggi i dati se abbiamo almeno una chiave
    for (uint8_t sector = 0; sector < card->num_sectors; sector++) {
        bool sector_read = false;
        
        // Prova a leggere con la chiave A
        if (card->sectors[sector].foundKeyA) {
            uint8_t key[6];
            memcpy(key, card->sectors[sector].KeyA.bytes, 6);
            
            display.clearDisplay();
            char statusMsg[32];
            sprintf(statusMsg, "Lettura settore %d", sector);
            common::println("Dump in corso...", 0, 0, 1, SSD1306_WHITE);
            common::println(statusMsg, 0, 16, 1, SSD1306_WHITE);
            common::println("Uso chiave A", 0, 32, 1, SSD1306_WHITE);
            display.display();
            
            // Converti il numero di settore in block address
            uint8_t firstBlock = sector * 4;
            uint8_t trailerBlock = firstBlock + 3;
            
            // Leggi i primi 3 blocchi del settore
            for (uint8_t block = firstBlock; block < trailerBlock; block++) {
                // Autenticazione con la chiave A
                if (nfc.mifareclassic_AuthenticateBlock(uid_bytes, uid_length, block, 0, key)) {
                    // Leggi il blocco
                    if (nfc.mifareclassic_ReadDataBlock(block, dump_data[sector] + ((block - firstBlock) * 16))) {
                        sector_read = true;
                    }
                }
            }
            
            // Leggi il blocco trailer (contiene i permessi e le chiavi)
            // Riutilizzo uid_bytes creato sopra
            if (nfc.mifareclassic_AuthenticateBlock(uid_bytes, uid_length, trailerBlock, 0, key)) {
                nfc.mifareclassic_ReadDataBlock(trailerBlock, dump_data[sector] + 48);
            }
        }
        // Se non è stato possibile leggere con la chiave A, prova con la chiave B
        else if (!sector_read && card->sectors[sector].foundKeyB) {
            uint8_t key[6];
            memcpy(key, card->sectors[sector].KeyB.bytes, 6);
            
            display.clearDisplay();
            char statusMsg[32];
            sprintf(statusMsg, "Lettura settore %d", sector);
            common::println("Dump in corso...", 0, 0, 1, SSD1306_WHITE);
            common::println(statusMsg, 0, 16, 1, SSD1306_WHITE);
            common::println("Uso chiave B", 0, 32, 1, SSD1306_WHITE);
            display.display();
            
            // Converti il numero di settore in block address
            uint8_t firstBlock = sector * 4;
            uint8_t trailerBlock = firstBlock + 3;
            
            // Leggi i primi 3 blocchi del settore
            for (uint8_t block = firstBlock; block < trailerBlock; block++) {
                // Autenticazione con la chiave B
                if (nfc.mifareclassic_AuthenticateBlock(uid_bytes, uid_length, block, 1, key)) {
                    // Leggi il blocco
                    if (nfc.mifareclassic_ReadDataBlock(block, dump_data[sector] + ((block - firstBlock) * 16))) {
                        sector_read = true;
                    }
                }
            }
            
            // Leggi il blocco trailer (contiene i permessi e le chiavi)
            // Riutilizzo uid_bytes creato sopra
            if (nfc.mifareclassic_AuthenticateBlock(uid_bytes, uid_length, trailerBlock, 1, key)) {
                nfc.mifareclassic_ReadDataBlock(trailerBlock, dump_data[sector] + 48);
            }
        }
    }
    
    // Salva il dump in formato binario (.mfd)
    File mfdFile = LittleFS.open(mfd_filename, "w");
    if (!mfdFile) {
        return false;
    }
    
    for (uint8_t sector = 0; sector < card->num_sectors; sector++) {
        mfdFile.write(dump_data[sector], 64);
    }
    
    mfdFile.close();
    
    // Salva il dump in formato testo (.txt)
    File txtFile = LittleFS.open(txt_filename, "w");
    if (!txtFile) {
        return false;
    }
    
    // Scrivi intestazione
    txtFile.println("# MFOC Dump Completo - Dump della carta");
    txtFile.println("# Formato: <block>: <data in hex>");
    txtFile.println();
    
    for (uint8_t sector = 0; sector < card->num_sectors; sector++) {
        uint8_t firstBlock = sector * 4;
        
        txtFile.printf("# Settore %d\n", sector);
        for (uint8_t blockInSector = 0; blockInSector < 4; blockInSector++) {
            uint8_t block = firstBlock + blockInSector;
            uint8_t* blockData = dump_data[sector] + (blockInSector * 16);
            
            txtFile.printf("Block %03d: ", block);
            for (int i = 0; i < 16; i++) {
                txtFile.printf("%02X ", blockData[i]);
            }
            
            // Aggiungi interpretazione ASCII
            txtFile.print(" | ");
            for (int i = 0; i < 16; i++) {
                if (blockData[i] >= 32 && blockData[i] <= 126) {
                    txtFile.write(blockData[i]);
                } else {
                    txtFile.print(".");
                }
            }
            
            txtFile.println();
        }
        txtFile.println();
    }
    
    txtFile.close();
    return true;
}
