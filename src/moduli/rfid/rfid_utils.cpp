/**
 * Implementazioni delle funzioni di utilità RFID richieste da MFCUK
 */

#include "rfid.h"
#include <Arduino.h>
#include <input.h>

/**
 * Ottiene l'UID della carta attualmente presente sul lettore
 * @param uid puntatore alla variabile che conterrà l'UID come uint32_t
 * @return true se l'operazione ha successo, false altrimenti
 */
bool rfid_get_uid(uint32_t* uid) {
    if (uid == nullptr) {
        Serial.println("[RFID] Errore: puntatore uid non valido");
        return false;
    }
    
    // Verifica che NFC sia inizializzato correttamente
    static bool nfc_initialized = false;
    if (!nfc_initialized) {
        nfc.begin();
        if (!nfc.getFirmwareVersion()) {
            Serial.println("[ERROR] PN532 non trovato in rfid_get_uid!");
            return false;
        }
        nfc.SAMConfig();
        nfc_initialized = true;
        Serial.println("[INFO] Modulo NFC inizializzato in rfid_get_uid");
    }
    
    uint8_t uid_bytes[7];
    uint8_t uid_length;
    
    // Legge l'UID dalla carta
    if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid_bytes, &uid_length)) {
        Serial.println("[RFID] Carta non rilevata");
        return false;
    }
    
    // Stampa l'UID letto
    Serial.print("[RFID] UID: ");
    for (uint8_t i = 0; i < uid_length; i++) {
        Serial.print(uid_bytes[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Converte i byte dell'UID in un uint32_t
    // Per carte con UID a 4 byte
    if (uid_length == 4) {
        *uid = ((uint32_t)uid_bytes[0] << 24) | 
               ((uint32_t)uid_bytes[1] << 16) | 
               ((uint32_t)uid_bytes[2] << 8) | 
               (uint32_t)uid_bytes[3];
    } 
    // Per carte con UID a 7 byte, prendiamo solo i primi 4
    else if (uid_length == 7) {
        *uid = ((uint32_t)uid_bytes[0] << 24) | 
               ((uint32_t)uid_bytes[1] << 16) | 
               ((uint32_t)uid_bytes[2] << 8) | 
               (uint32_t)uid_bytes[3];
        
        Serial.println("[RFID] Avviso: UID a 7 byte, utilizzati solo i primi 4 byte");
    }
    // Per altri formati di UID non supportati
    else {
        Serial.println("[RFID] Formato UID non supportato");
        return false;
    }
    
    return true;
}

/**
 * Attende che una carta sia avvicinata al lettore
 * @param timeout_ms Tempo massimo di attesa in millisecondi (0 = attesa infinita)
 * @return true se una carta è stata rilevata, false se è scaduto il timeout
 */
bool rfid_wait_for_tag(uint32_t timeout_ms) {
    uint8_t uid[7];  // Buffer per memorizzare l'UID
    uint8_t uidLength;  // Lunghezza dell'UID
    bool found = false;
    uint32_t startTime = millis();
    
    // Verifica che NFC sia inizializzato correttamente
    static bool nfc_initialized = false;
    if (!nfc_initialized) {
        nfc.begin();
        if (!nfc.getFirmwareVersion()) {
            Serial.println("[ERROR] PN532 non trovato in rfid_wait_for_tag!");
            return false;
        }
        nfc.SAMConfig();
        nfc_initialized = true;
        Serial.println("[INFO] Modulo NFC inizializzato in rfid_wait_for_tag");
    }
    
    Serial.print("[RFID] Attesa carta (timeout: ");
    Serial.print(timeout_ms);
    Serial.println(" ms)");
    
    do {
        // Controllo se è stato premuto il pulsante RST (escape)
        if (digitalRead(buttonPin_RST) == LOW) {
            Serial.println("[RFID] Attesa interrotta dall'utente");
            return false;
        }
        
        // Cerca una carta nel campo
        found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        
        if (found) {
            Serial.print("[RFID] Carta trovata! UID: ");
            for (uint8_t i = 0; i < uidLength; i++) {
                Serial.print(uid[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            return true;
        }
        
        // Piccola pausa per evitare di sovraccaricare la CPU
        delay(50);
        
        // Verifica timeout se specificato
        if (timeout_ms > 0 && (millis() - startTime > timeout_ms)) {
            Serial.println("[RFID] Timeout scaduto, nessuna carta rilevata");
            return false;
        }
    } while (!found);
    
    // Non dovremmo mai arrivare qui
    return false;
}

/**
 * Calcola il numero di blocco assoluto dato il settore e il blocco relativo al settore
 * NOTA: Questa funzione è dichiarata come extern per evitare duplicazioni con mfcuk_utils.cpp
 */
extern uint8_t get_block_number_by_sector(uint8_t sector, uint8_t block_in_sector);
/*
uint8_t get_block_number_by_sector(uint8_t sector, uint8_t block_in_sector) {
    // Verifica che i parametri siano validi
    if (sector >= 40) {
        Serial.println("[RFID] Errore: settore non valido");
        return 0;
    }
    
    // Per i settori 0-31 (Mifare Classic 1K ha settori 0-15)
    if (sector < 32) {
        if (block_in_sector >= 4) {
            Serial.println("[RFID] Errore: blocco nel settore non valido");
            return 0;
        }
        return sector * 4 + block_in_sector;
    } 
    // Per i settori 32-39 (solo Mifare Classic 4K)
    else {
        if (block_in_sector >= 16) {
            Serial.println("[RFID] Errore: blocco nel settore non valido");
            return 0;
        }
        return 128 + (sector - 32) * 16 + block_in_sector;
    }
}
*/
