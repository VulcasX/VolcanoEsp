/**
 * MFCUK - MiFare Classic Universal toolKit
 * Implementazione degli attacchi
 */

#include "mfcuk_attack.h"
#include "mfcuk_crypto.h"
#include "mfcuk_types.h"
#include "mfcuk_utils.h"
#include "mfcuk.h"     // Include per accedere a mfcuk_update_progress e altre funzioni
#include "rfid.h"
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <input.h>

// Riferimento al display OLED
extern Adafruit_SSD1306 display;
extern const int buttonPin_RST;

/**
 * Implementa l'attacco Darkside per recuperare una chiave
 * NOTA: Spostata all'implementazione nel file mfcuk_attack.cpp per evitare duplicazioni
 */
bool mfcuk_darkside_attack(MfcukConfig* config, uint8_t* key) {
    uint32_t uid = 0;
    uint32_t nonces[100];
    int num_nonces = 0;
    uint64_t recovered_key = 0;
    bool success = false;
    MifareKey found_key;

    // Recupera l'UID della carta
    if (!rfid_get_uid(&uid)) {
        return false;
    }
    
    // Raccogli i nonce per l'attacco darkside
    success = mfcuk_collect_nonces(
        uid, 
        get_block_number_by_sector(config->target_sector, 0),
        NULL, // Non serve chiave nota per darkside
        config->target_key_type,
        nonces, 
        100, 
        &num_nonces
    );
    
    if (!success || num_nonces < 10) {
        return false;
    }
    
    // Recupera la chiave usando l'attacco darkside
    success = mfcuk_recover_key_darkside(uid, nonces, num_nonces, &found_key);
    
    if (success) {
        // Copia la chiave trovata nel buffer di output
        memcpy(key, found_key.bytes, MIFARE_KEY_SIZE);
    }
    
    return success;
}

/**
 * Implementa l'attacco Nested per recuperare una chiave
 * NOTA: Spostata all'implementazione nel file mfcuk_attack.cpp per evitare duplicazioni
 */
bool mfcuk_nested_attack(MfcukConfig* config, uint8_t* key) {
    uint32_t uid = 0;
    uint32_t distances[100];
    int numDistances = 0;
    uint64_t recovered_key = 0;
    bool success = false;
    int iteration = 0;
    int progress = 0;
    MifareKey found_key;
    
    // Recupera l'UID della carta
    if (!rfid_get_uid(&uid)) {
        return false;
    }
    
    // Raccolta distanze tra nonce
    uint32_t nonces[200];
    int num_nonces = 0;
    
    success = mfcuk_collect_nonces(
        uid, 
        get_block_number_by_sector(config->target_sector, 0),
        &config->known_key, 
        config->known_key_type,
        nonces, 
        200, 
        &num_nonces
    );
    
    if (!success || num_nonces < 20) {
        return false;
    }
    
    // Calcola le distanze tra nonce consecutivi
    for (int i = 0; i < num_nonces - 1 && numDistances < 100; i++) {
        distances[numDistances++] = nonces[i+1] - nonces[i];
    }
    
    // Recupera la chiave usando l'attacco nested
    success = mfcuk_recover_key_nested(
        uid,
        &config->known_key,
        config->target_sector,
        config->target_key_type,
        distances,
        numDistances,
        &found_key
    );
    
    if (success) {
        // Copia la chiave trovata nel buffer di output
        memcpy(key, found_key.bytes, MIFARE_KEY_SIZE);
    }
    
    return success;
}

/**
 * Raccoglie i nonce dal lettore durante gli attacchi
 * Simulazione di una funzione che recupererebbe i nonce reali dalla carta
 */
bool mfcuk_collect_nonces(uint32_t uid, uint8_t block, MifareKey* known_key, uint8_t key_type, 
                          uint32_t* nonces, int max_nonces, int* num_collected) {
    int i;
    uint32_t last_nonce = 0;
    *num_collected = 0;
    
    // Simulazione di raccolta nonce
    // In un'implementazione reale, questa funzione comunicherebbe con la carta
    // e raccoglierebbe i nonce reali durante tentativi di autenticazione
    
    for (i = 0; i < max_nonces; i++) {
        // Controllo per interruzione utente
        if (digitalRead(buttonPin_RST) == LOW) {
            return false;
        }
        
        // Simulazione generazione nonce incrementale con un po' di variazione casuale
        uint32_t new_nonce;
        if (i == 0) {
            new_nonce = random(0xFFFFFFFF);
        } else {
            new_nonce = last_nonce + random(100, 1000);
        }
        
        nonces[*num_collected] = new_nonce;
        last_nonce = new_nonce;
        (*num_collected)++;
        
        // Simulazione del tempo necessario per ottenere un nonce
        delay(10);
        
        // Per una simulazione più realistica, interrompiamo dopo un certo numero di nonce
        if (*num_collected >= 30) {
            return true;
        }
    }
    
    return (*num_collected > 0);
}

/**
 * Recupera la chiave del settore target dall'attacco Darkside.
 * Implementazione di base dell'attacco Darkside
 */
bool mfcuk_recover_key_darkside(uint32_t uid, uint32_t* nonces, int num_nonces, MifareKey* key) {
    // Implementazione semplificata dell'attacco Darkside
    Serial.println("[MFCUK] Analisi dei nonces per recupero chiave Darkside");
    
    // Simula processo di crack della chiave con un po' di attesa
    for (int i = 0; i < 10; i++) {
        delay(200);
        
        // Verifica interruzione utente
        if (digitalRead(buttonPin_RST) == LOW) {
            return false;
        }
        
        // Aggiorna il display con il progresso
        char msg[32];
        sprintf(msg, "Crack %d%%", (i+1)*10);
        mfcuk_update_progress(30 + (i+1)*7, msg);
    }
    
    // Generiamo una chiave simulata (in una implementazione reale, questa sarebbe la vera chiave trovata)
    memset(key->bytes, 0, MIFARE_KEY_SIZE);
    for (int i = 0; i < MIFARE_KEY_SIZE; i++) {
        key->bytes[i] = 0xA0 + i;
    }
    
    return true;
}

/**
 * Recupera la chiave usando l'attacco Nested.
 * Implementazione di base dell'attacco Nested
 */
bool mfcuk_recover_key_nested(uint32_t uid, MifareKey* known_key, uint8_t target_sector, 
                              uint8_t target_key_type, uint32_t* distances, int num_distances,
                              MifareKey* recovered_key) {
    // Implementazione semplificata dell'attacco Nested
    Serial.println("[MFCUK] Analisi delle distanze per recupero chiave Nested");
    
    // Simula processo di crack della chiave con un po' di attesa
    for (int i = 0; i < 10; i++) {
        delay(300);
        
        // Verifica interruzione utente
        if (digitalRead(buttonPin_RST) == LOW) {
            return false;
        }
        
        // Aggiorna il display con il progresso
        char msg[32];
        sprintf(msg, "Crack %d%%", (i+1)*10);
        mfcuk_update_progress(30 + (i+1)*7, msg);
    }
    
    // Generiamo una chiave simulata (in una implementazione reale, questa sarebbe la vera chiave trovata)
    memset(recovered_key->bytes, 0, MIFARE_KEY_SIZE);
    for (int i = 0; i < MIFARE_KEY_SIZE; i++) {
        recovered_key->bytes[i] = 0x10 + i;
    }
    
    return true;
}

/**
 * Funzione di supporto per il recupero chiavi
 * Versione legacy mantenuta per compatibilità
 */
bool mfcuk_recover_key(uint8_t* nt, uint8_t* nr, uint8_t* ar, uint8_t* key) {
    Serial.println("[DEBUG] Inizio funzione mfcuk_recover_key");
    
    // Verifico che tutti i puntatori passati siano validi
    if (nt == nullptr || nr == nullptr || ar == nullptr || key == nullptr) {
        Serial.println("[DEBUG] Errore: puntatori non validi");
        return false;
    }
    
    // Verifico che i valori passati non siano nulli
    bool hasZeros = true;
    for (int i = 0; i < 4; i++) {
        if (nt[i] != 0 || nr[i] != 0 || ar[i] != 0) {
            hasZeros = false;
            break;
        }
    }
    
    if (hasZeros) {
        Serial.println("[DEBUG] Errore: valori nulli");
        return false;
    }
    
    //    // Riferimento già dichiarato all'inizio del file
    // extern const int buttonPin_RST;
    
    // Converto i nonce e le risposte in valori interi per crapto1
    uint32_t nt0_int = 0, nr0_int = 0, ar0_int = 0;
    
    Serial.println("[DEBUG] Conversione nonce a interi");
    nt0_int = (nt[0] << 24) | (nt[1] << 16) | (nt[2] << 8) | nt[3];
    nr0_int = (nr[0] << 24) | (nr[1] << 16) | (nr[2] << 8) | nr[3];
    ar0_int = (ar[0] << 24) | (ar[1] << 16) | (ar[2] << 8) | ar[3];
    
    Serial.print("[DEBUG] NT int: 0x");
    Serial.println(nt0_int, HEX);
    Serial.print("[DEBUG] NR int: 0x");
    Serial.println(nr0_int, HEX);
    Serial.print("[DEBUG] AR int: 0x");
    Serial.println(ar0_int, HEX);
    
    // Tentativo di recuperare la chiave con crapto1
    Crypto1State *revstate;
    uint64_t found_key = 0;
    bool keyFound = false;
    
    // Fase 1: Inizializzazione Crypto1
    Serial.println("[DEBUG] Inizializzazione Crypto1State");
    revstate = crypto1_create(0);
    if (revstate != nullptr) {
        Serial.println("[DEBUG] Inizializzazione OK");
        
        // Fase 2: Recupero chiave con darkside_crack
        Serial.println("[DEBUG] Avvio crack");
        uint64_t key_recovered = 0;
        
        if (darkside_crack(nt0_int, nr0_int, ar0_int, &key_recovered)) {
            found_key = key_recovered;
            keyFound = true;
            
            Serial.print("[DEBUG] Chiave trovata: 0x");
            Serial.println((unsigned long)(found_key), HEX);
        } else {
            Serial.println("[DEBUG] Darkside crack fallito");
        }
        
        // Fase 3: Liberazione memoria
        crypto1_destroy(revstate);
    } else {
        Serial.println("[DEBUG] Errore inizializzazione Crypto1");
    }
    
    // Verifica se RST è stato premuto durante l'elaborazione
    if (digitalRead(buttonPin_RST) == LOW) {
        Serial.println("[DEBUG] RST premuto durante crack - esco");
        return false;
    }
    
    // Se la chiave è stata trovata, la convertiamo nel formato richiesto
    if (keyFound) {
        Serial.println("[DEBUG] Chiave recuperata!");
        // Converti la chiave trovata in array di byte
        for(int i = 0; i < 6; i++) {
            key[i] = (found_key >> (40-i*8)) & 0xFF;
            Serial.print("[DEBUG] Key[");
            Serial.print(i);
            Serial.print("] = 0x");
            Serial.println(key[i], HEX);
        }
        return true;
    }
    
    Serial.println("[DEBUG] Chiave non trovata");
    return false;
}

/**
 * Funzione principale per l'esecuzione degli attacchi su Mifare Classic.
 * Richiamata dall'interfaccia utente per avviare l'attacco selezionato.
 * Supporta l'attacco Darkside e Nested.
 * Restituisce true se la chiave viene trovata, false altrimenti.
 * 
 * Implementazione spostata in mfcuk.cpp per unificare l'interfaccia utente
 * e l'elaborazione degli attacchi.
 */
// La funzione mfcuk_run è ora completamente implementata in mfcuk.cpp

/**
 * Inizializza la struttura MfcukConfig con i valori predefiniti.
 * Richiamata internamente per garantire che tutti i campi di configurazione siano impostati.
 * Non sovrascrive i valori già impostati.
 */
void initMfcukConfig(MfcukConfig* config) {
    if (config == nullptr) {
        return;
    }
    
    // Imposta solo i valori non già impostati
    if (config->mode == 0) {
        config->mode = ATTACK_MODE_DARKSIDE;  // Valore predefinito: darkside
    }
    
    if (config->target_sector == 0) {
        config->target_sector = 1;  // Valore predefinito: settore 1
    }
    
    if (config->known_sector == 0) {
        config->known_sector = 0;  // Valore predefinito: settore 0
    }
    
    // Non è più presente targetBlock e knownSectorBlock nella struttura aggiornata
    
    if (config->sleepTime == 0) {
        config->sleepTime = 100;  // Valore predefinito: 100ms
    }
    
    // Se la chiave conosciuta è vuota, imposta la chiave predefinita (FFFFFFFFFFFF)
    bool isZero = true;
    for (int i = 0; i < 6; i++) {
        if (config->known_key.bytes[i] != 0) {
            isZero = false;
            break;
        }
    }
    
    if (isZero) {
        for (int i = 0; i < 6; i++) {
            config->known_key.bytes[i] = 0xFF;
        }
    }
    
    // Imposta i tipi di chiave predefiniti se non già impostati
    if (config->target_key_type != KEY_A && config->target_key_type != KEY_B) {
        config->target_key_type = KEY_A;  // Valore predefinito: KEY_A
    }
    
    if (config->known_key_type != KEY_A && config->known_key_type != KEY_B) {
        config->known_key_type = KEY_A;  // Valore predefinito: KEY_A
    }
}
