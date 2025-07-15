#include "extended_pn532.h"

// Implementazione della funzione readResponse che sostituisce readdata
bool Extended_PN532::readResponse(uint8_t* buffer, uint8_t length, uint16_t timeout) {
    // Questa implementazione utilizza le funzioni pubbliche disponibili
    // invece delle funzioni private
    uint8_t response[8];
    uint8_t responseLength = sizeof(response);
    uint8_t cmd = PN532_COMMAND_GETFIRMWAREVERSION;  // Un comando semplice per verificare comunicazione
    
    if (!sendCommandCheckAck(&cmd, 1, timeout)) {
        return false;
    }
    
    // Attendiamo una risposta valida
    if (!getFirmwareVersion()) {
        return false;
    }
    
    // Copiamo i dati disponibili nel buffer
    memcpy(buffer, pn532_packetbuffer, (length > 8) ? 8 : length);
    
    return true;
}

bool Extended_PN532::mifareClassicGetNT(uint8_t* nt) {
    // Implementazione migliorata con gestione degli errori
    uint8_t timeout = 0;
    const uint8_t MAX_TIMEOUT = 2;
    
    while (timeout < MAX_TIMEOUT) {
        pn532_packetbuffer[0] = MIFARE_CMD_AUTH_A;  // Comando auth
        pn532_packetbuffer[1] = 0;  // Blocco 0
        
        // Tentativo con timeout progressivo
        if (!sendCommandCheckAck(pn532_packetbuffer, 2, 500 + (timeout * 200))) {
            Serial.println("[PN532] Errore invio comando AUTH per NT");
            timeout++;
            delay(50);
            continue;
        }
        
        // Attesa breve per stabilizzazione
        delay(30);
        
        // Usa readResponse invece di readdata con timeout progressivo
        if (!readResponse(pn532_packetbuffer, 4, 800 + (timeout * 300))) {
            Serial.println("[PN532] Errore lettura risposta NT");
            timeout++;
            delay(50);
            continue;
        }
        
        memcpy(nt, pn532_packetbuffer, 4);
        return true;
    }
    
    Serial.println("[PN532] Timeout finale in mifareClassicGetNT");
    return false;
}

bool Extended_PN532::mifareClassicGetAR(uint8_t* nr, uint8_t* ar) {
    // Implementazione migliorata con gestione degli errori
    uint8_t timeout = 0;
    const uint8_t MAX_TIMEOUT = 2;
    
    while (timeout < MAX_TIMEOUT) {
        pn532_packetbuffer[0] = MIFARE_CMD_AUTH_A;  // Comando auth
        memcpy(pn532_packetbuffer + 1, nr, 4);
        
        // Debug del nonce reader inviato
        Serial.print("[PN532] Invio nr per AR: ");
        for (int i = 0; i < 4; i++) {
            Serial.print(nr[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        // Tentativo con timeout progressivo
        if (!sendCommandCheckAck(pn532_packetbuffer, 5, 500 + (timeout * 200))) {
            Serial.println("[PN532] Errore invio comando AUTH per AR");
            timeout++;
            delay(50);
            continue;
        }
        
        // Attesa breve per stabilizzazione
        delay(30);
        
        // Usa readResponse invece di readdata con timeout progressivo
        if (!readResponse(pn532_packetbuffer, 4, 800 + (timeout * 300))) {
            Serial.println("[PN532] Errore lettura risposta AR");
            timeout++;
            delay(50);
            continue;
        }
        
        memcpy(ar, pn532_packetbuffer, 4);
        
        // Debug della risposta AR ricevuta
        Serial.print("[PN532] AR ricevuto: ");
        for (int i = 0; i < 4; i++) {
            Serial.print(ar[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        return true;
    }
    
    Serial.println("[PN532] Timeout finale in mifareClassicGetAR");
    return false;
}

// Funzione per il reset hardware del PN532
bool Extended_PN532::resetPN532() {
    Serial.println("[PN532] Esecuzione reset hardware");
    
    // Reset del bus I2C
    if (!resetI2CBus()) {
        Serial.println("[PN532] Errore nel reset del bus I2C");
        return false;
    }
    
    // Reset hardware usando il pin RSTPD_N (non possiamo accedere direttamente a _reset perché è private)
    // Usiamo una funzione pubblica della classe base se disponibile
    SAMConfig(); // Prova a reinizializzare il SAM
    delay(100);
    Serial.println("[PN532] Reset software tramite SAMConfig");
    
    // Reinizializza la comunicazione
    begin();
    delay(100);
    
    // Verifica se il dispositivo risponde
    uint32_t versiondata = getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[PN532] Errore: dispositivo non risponde dopo il reset");
        return false;
    }
    
    Serial.println("[PN532] Reset completato con successo");
    return true;
}

// Funzione per il reset del bus I2C
bool Extended_PN532::resetI2CBus() {
    Serial.println("[PN532] Reset del bus I2C");
    Wire.end();
    delay(50);
    Wire.begin();
    delay(50);
    return true;
}

// Implementazione robusta di mifareClassicGetNT con retry automatici
bool Extended_PN532::robustMifareClassicGetNT(uint8_t* nt, uint8_t maxRetries) {
    Serial.println("[PN532] robustMifareClassicGetNT: tentativo con gestione errori");
    
    for (uint8_t retry = 0; retry < maxRetries; retry++) {
        // Reset preventivo se non è il primo tentativo
        if (retry > 0) {
            Serial.print("[PN532] Tentativo #");
            Serial.print(retry + 1);
            Serial.println(" - reset preventivo");
            
            // Effettua un reset soft del bus I2C
            resetI2CBus();
            
            // Ogni 2 tentativi, prova un reset hardware più aggressivo
            if (retry % 2 == 0) {
                resetPN532();
            }
            
            delay(100 * retry); // Attesa progressivamente più lunga
        }
        
        // Tentativo di recupero NT
        bool success = mifareClassicGetNT(nt);
        
        if (success) {
            Serial.print("[PN532] NT ottenuto con successo: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(nt[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            return true;
        }
        
        Serial.println("[PN532] Tentativo fallito, riprovo...");
    }
    
    Serial.println("[PN532] Tutti i tentativi di recupero NT falliti");
    return false;
}

// Implementazione robusta di mifareClassicGetAR con retry automatici
bool Extended_PN532::robustMifareClassicGetAR(uint8_t* nr, uint8_t* ar, uint8_t maxRetries) {
    Serial.println("[PN532] robustMifareClassicGetAR: tentativo con gestione errori");
    
    for (uint8_t retry = 0; retry < maxRetries; retry++) {
        // Reset preventivo se non è il primo tentativo
        if (retry > 0) {
            Serial.print("[PN532] Tentativo #");
            Serial.print(retry + 1);
            Serial.println(" - reset preventivo");
            
            // Effettua un reset soft del bus I2C
            resetI2CBus();
            
            // Ogni 2 tentativi, prova un reset hardware più aggressivo
            if (retry % 2 == 0) {
                resetPN532();
            }
            
            delay(100 * retry); // Attesa progressivamente più lunga
        }
        
        // Tentativo di recupero AR
        bool success = mifareClassicGetAR(nr, ar);
        
        if (success) {
            Serial.print("[PN532] AR ottenuto con successo: ");
            for (int i = 0; i < 4; i++) {
                Serial.print(ar[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            return true;
        }
        
        Serial.println("[PN532] Tentativo fallito, riprovo...");
    }
    
    Serial.println("[PN532] Tutti i tentativi di recupero AR falliti");
    return false;
}
