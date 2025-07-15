/**
 * MFCUK - Utilità comuni
 * 
 * Implementazione delle funzioni di utilità per il progetto MFCUK
 */

#include "mfcuk_utils.h"
#include <FS.h>
#include <LittleFS.h>

// Funzioni di conversione

/**
 * Converte un carattere esadecimale in un byte
 */
uint8_t hex_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

/**
 * Converte una stringa esadecimale in un array di byte
 */
void hex_to_bytes(const char* hex, uint8_t* bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        bytes[i] = (hex_to_byte(hex[i*2]) << 4) | hex_to_byte(hex[i*2+1]);
    }
}

/**
 * Converte un array di byte in una stringa esadecimale
 */
void bytes_to_hex(const uint8_t* bytes, char* hex, size_t len) {
    static const char hex_chars[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i++) {
        hex[i*2] = hex_chars[(bytes[i] >> 4) & 0xF];
        hex[i*2+1] = hex_chars[bytes[i] & 0xF];
    }
    hex[len*2] = '\0';
}

/**
 * Converte 4 byte in un uint32_t (little-endian)
 */
uint32_t bytes_to_uint32(const uint8_t* bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | 
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

/**
 * Converte un uint32_t in 4 byte (little-endian)
 */
void uint32_to_bytes(uint32_t value, uint8_t* bytes) {
    bytes[0] = value & 0xFF;
    bytes[1] = (value >> 8) & 0xFF;
    bytes[2] = (value >> 16) & 0xFF;
    bytes[3] = (value >> 24) & 0xFF;
}

// Funzioni di visualizzazione

/**
 * Stampa un array di byte in formato esadecimale
 */
void print_hex(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
    }
}

/**
 * Stampa un array di byte in formato esadecimale con spazi
 */
void print_hex_array(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
}

/**
 * Stampa una chiave Mifare in formato esadecimale
 */
void print_mifare_key(const MifareKey* key) {
    print_hex(key->bytes, MIFARE_KEY_SIZE);
}

/**
 * Stampa un UID Mifare in formato esadecimale
 */
void print_mifare_uid(uint32_t uid) {
    uint8_t bytes[4];
    uint32_to_bytes(uid, bytes);
    print_hex(bytes, 4);
}

/**
 * Stampa i dati di un settore Mifare
 */
void dump_mifare_sector(const MifareSector* sector, uint8_t sector_index) {
    Serial.print("Settore ");
    Serial.print(sector_index);
    Serial.println(":");
    
    Serial.print("  Chiave A: ");
    print_mifare_key(&sector->key_a);
    Serial.print(" (");
    Serial.print(sector->authenticated_a ? "autenticato" : "non autenticato");
    Serial.println(")");
    
    Serial.print("  Chiave B: ");
    print_mifare_key(&sector->key_b);
    Serial.print(" (");
    Serial.print(sector->authenticated_b ? "autenticato" : "non autenticato");
    Serial.println(")");
    
    for (int i = 0; i < MIFARE_BLOCKS_PER_SECTOR; i++) {
        Serial.print("  Blocco ");
        Serial.print(get_block_number_by_sector(sector_index, i));
        Serial.print(": ");
        print_hex_array(sector->block_data[i], MIFARE_BLOCK_SIZE);
        Serial.println();
    }
}

/**
 * Stampa i dati di una carta Mifare
 */
void dump_mifare_card(const MifareCard* card) {
    Serial.print("UID: ");
    print_mifare_uid(card->uid);
    Serial.println();
    
    Serial.print("SAK: ");
    Serial.println(card->sak, HEX);
    
    Serial.print("ATQA: ");
    print_hex(card->atqa, 2);
    Serial.println();
    
    Serial.print("ATS: ");
    if (card->ats_len > 0) {
        print_hex(card->ats, card->ats_len);
    } else {
        Serial.print("(nessuno)");
    }
    Serial.println();
    
    Serial.print("Numero di settori: ");
    Serial.println(card->num_sectors);
    
    for (int i = 0; i < card->num_sectors; i++) {
        dump_mifare_sector(&card->sectors[i], i);
    }
}

// Funzioni di utilità per le schede Mifare

/**
 * Restituisce il numero assoluto del blocco dato il settore e il blocco relativo
 */
uint8_t get_block_number_by_sector(uint8_t sector, uint8_t block_in_sector) {
    if (sector < 32) {
        // Settori 0-31 hanno 4 blocchi ciascuno
        return sector * 4 + block_in_sector;
    } else {
        // Settori 32-39 hanno 16 blocchi ciascuno
        return 128 + (sector - 32) * 16 + block_in_sector;
    }
}

/**
 * Restituisce il settore dato il numero assoluto del blocco
 */
uint8_t get_sector_by_block(uint8_t block) {
    if (block < 128) {
        return block / 4;
    } else {
        return 32 + ((block - 128) / 16);
    }
}

/**
 * Verifica se un blocco è un trailer (blocco di controllo settore)
 */
bool is_trailer_block(uint8_t block) {
    if (block < 128) {
        return ((block + 1) % 4 == 0);
    } else {
        return ((block - 128 + 1) % 16 == 0);
    }
}

/**
 * Restituisce il numero del blocco trailer per un settore dato
 */
uint8_t get_trailer_block_for_sector(uint8_t sector) {
    if (sector < 32) {
        return sector * 4 + 3;
    } else {
        return 128 + (sector - 32) * 16 + 15;
    }
}

/**
 * Calcola i bit di accesso per un blocco di controllo
 */
void calculate_block_access_bits(uint8_t* block_data, uint8_t block_type, bool read_a, bool write_a, bool read_b, bool write_b) {
    // Questa è una funzione semplificata per impostare i bit di accesso
    // In una implementazione completa, sarebbe più elaborata
    
    // I bit di accesso sono complessi e dipendono dal tipo di blocco
    // Qui impostiamo solo alcuni valori predefiniti comuni
    
    // Bit di accesso predefiniti per un blocco dati standard
    uint8_t access_bits[3] = {0xFF, 0x07, 0x80};
    
    if (block_type == 0) {
        // Blocco dati standard
        if (read_a && write_a && read_b && !write_b) {
            access_bits[0] = 0x78;
            access_bits[1] = 0x87;
            access_bits[2] = 0x88;
        } else if (read_a && !write_a && read_b && write_b) {
            access_bits[0] = 0xB4;
            access_bits[1] = 0x4B;
            access_bits[2] = 0x44;
        }
    } else if (block_type == 1) {
        // Blocco trailer
        access_bits[0] = 0x7F;
        access_bits[1] = 0x07;
        access_bits[2] = 0x88;
    }
    
    // Imposta i bit di accesso nel blocco
    block_data[6] = access_bits[0];
    block_data[7] = access_bits[1];
    block_data[8] = access_bits[2];
}

// Funzioni per il salvataggio e caricamento dei dati

/**
 * Salva le chiavi recuperate su file
 */
bool save_keys_to_file(const char* filename, const MifareKey keys[MIFARE_MAXSECTOR][2], const bool key_found[MIFARE_MAXSECTOR][2]) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        return false;
    }
    
    char hex_key[MIFARE_KEY_SIZE * 2 + 1];
    
    for (int sector = 0; sector < MIFARE_MAXSECTOR; sector++) {
        for (int key_type = 0; key_type < 2; key_type++) {
            if (key_found[sector][key_type]) {
                bytes_to_hex(keys[sector][key_type].bytes, hex_key, MIFARE_KEY_SIZE);
                file.print(sector);
                file.print(':');
                file.print(key_type);
                file.print(':');
                file.println(hex_key);
            }
        }
    }
    
    file.close();
    return true;
}

/**
 * Carica le chiavi da file
 */
bool load_keys_from_file(const char* filename, MifareKey keys[MIFARE_MAXSECTOR][2], bool key_found[MIFARE_MAXSECTOR][2]) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return false;
    }
    
    // Inizializza array key_found a false
    for (int sector = 0; sector < MIFARE_MAXSECTOR; sector++) {
        for (int key_type = 0; key_type < 2; key_type++) {
            key_found[sector][key_type] = false;
        }
    }
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        int sep1 = line.indexOf(':');
        if (sep1 <= 0) continue;
        
        int sep2 = line.indexOf(':', sep1 + 1);
        if (sep2 <= sep1 + 1) continue;
        
        int sector = line.substring(0, sep1).toInt();
        int key_type = line.substring(sep1 + 1, sep2).toInt();
        String key_hex = line.substring(sep2 + 1);
        
        if (sector >= 0 && sector < MIFARE_MAXSECTOR && 
            key_type >= 0 && key_type < 2 && 
            key_hex.length() == MIFARE_KEY_SIZE * 2) {
            
            hex_to_bytes(key_hex.c_str(), keys[sector][key_type].bytes, MIFARE_KEY_SIZE);
            key_found[sector][key_type] = true;
        }
    }
    
    file.close();
    return true;
}

/**
 * Salva i dati di una carta su file
 */
bool save_card_to_file(const char* filename, const MifareCard* card) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        return false;
    }
    
    // Salva UID e info della carta
    file.print("UID:");
    uint8_t uid_bytes[4];
    uint32_to_bytes(card->uid, uid_bytes);
    char hex_uid[9];
    bytes_to_hex(uid_bytes, hex_uid, 4);
    file.println(hex_uid);
    
    file.print("SAK:");
    file.println(card->sak, HEX);
    
    file.print("ATQA:");
    char hex_atqa[5];
    bytes_to_hex(card->atqa, hex_atqa, 2);
    file.println(hex_atqa);
    
    file.print("ATS:");
    if (card->ats_len > 0) {
        char* hex_ats = new char[card->ats_len * 2 + 1];
        bytes_to_hex(card->ats, hex_ats, card->ats_len);
        file.println(hex_ats);
        delete[] hex_ats;
    } else {
        file.println("");
    }
    
    file.print("SECTORS:");
    file.println(card->num_sectors);
    
    // Salva i dati dei settori
    for (int sector = 0; sector < card->num_sectors; sector++) {
        const MifareSector* s = &card->sectors[sector];
        
        char hex_key_a[MIFARE_KEY_SIZE * 2 + 1];
        char hex_key_b[MIFARE_KEY_SIZE * 2 + 1];
        bytes_to_hex(s->key_a.bytes, hex_key_a, MIFARE_KEY_SIZE);
        bytes_to_hex(s->key_b.bytes, hex_key_b, MIFARE_KEY_SIZE);
        
        file.print("S");
        file.print(sector);
        file.print(":A:");
        file.print(s->authenticated_a ? "1:" : "0:");
        file.println(hex_key_a);
        
        file.print("S");
        file.print(sector);
        file.print(":B:");
        file.print(s->authenticated_b ? "1:" : "0:");
        file.println(hex_key_b);
        
        // Salva i dati dei blocchi
        for (int block = 0; block < MIFARE_BLOCKS_PER_SECTOR; block++) {
            file.print("B");
            file.print(get_block_number_by_sector(sector, block));
            file.print(":");
            char* hex_block = new char[MIFARE_BLOCK_SIZE * 2 + 1];
            bytes_to_hex(s->block_data[block], hex_block, MIFARE_BLOCK_SIZE);
            file.println(hex_block);
            delete[] hex_block;
        }
    }
    
    file.close();
    return true;
}

/**
 * Carica i dati di una carta da file
 */
bool load_card_from_file(const char* filename, MifareCard* card) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return false;
    }
    
    // Inizializza la struttura della carta
    memset(card, 0, sizeof(MifareCard));
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        
        if (line.startsWith("UID:")) {
            String uid_hex = line.substring(4);
            uint8_t uid_bytes[4];
            hex_to_bytes(uid_hex.c_str(), uid_bytes, 4);
            card->uid = bytes_to_uint32(uid_bytes);
        }
        else if (line.startsWith("SAK:")) {
            card->sak = strtol(line.substring(4).c_str(), NULL, 16);
        }
        else if (line.startsWith("ATQA:")) {
            String atqa_hex = line.substring(5);
            hex_to_bytes(atqa_hex.c_str(), card->atqa, 2);
        }
        else if (line.startsWith("ATS:")) {
            String ats_hex = line.substring(4);
            card->ats_len = ats_hex.length() / 2;
            if (card->ats_len > 0) {
                hex_to_bytes(ats_hex.c_str(), card->ats, card->ats_len);
            }
        }
        else if (line.startsWith("SECTORS:")) {
            card->num_sectors = line.substring(8).toInt();
            if (card->num_sectors > MIFARE_MAXSECTOR) {
                card->num_sectors = MIFARE_MAXSECTOR;
            }
        }
        else if (line.startsWith("S")) {
            // Linee del formato S<sector>:<key_type>:<auth>:<key_hex>
            int sep1 = line.indexOf(':', 1);
            if (sep1 <= 1) continue;
            
            int sector = line.substring(1, sep1).toInt();
            if (sector < 0 || sector >= card->num_sectors) continue;
            
            char key_type = line.charAt(sep1 + 1);
            int sep2 = line.indexOf(':', sep1 + 2);
            if (sep2 <= sep1 + 2) continue;
            
            bool authenticated = (line.substring(sep1 + 3, sep2) == "1");
            String key_hex = line.substring(sep2 + 1);
            
            if (key_type == 'A' && key_hex.length() == MIFARE_KEY_SIZE * 2) {
                hex_to_bytes(key_hex.c_str(), card->sectors[sector].key_a.bytes, MIFARE_KEY_SIZE);
                card->sectors[sector].authenticated_a = authenticated;
            }
            else if (key_type == 'B' && key_hex.length() == MIFARE_KEY_SIZE * 2) {
                hex_to_bytes(key_hex.c_str(), card->sectors[sector].key_b.bytes, MIFARE_KEY_SIZE);
                card->sectors[sector].authenticated_b = authenticated;
            }
        }
        else if (line.startsWith("B")) {
            // Linee del formato B<block>:<data_hex>
            int sep = line.indexOf(':', 1);
            if (sep <= 1) continue;
            
            int block = line.substring(1, sep).toInt();
            int sector = get_sector_by_block(block);
            int block_in_sector = block - get_block_number_by_sector(sector, 0);
            
            if (sector < 0 || sector >= card->num_sectors || 
                block_in_sector < 0 || block_in_sector >= MIFARE_BLOCKS_PER_SECTOR) continue;
            
            String data_hex = line.substring(sep + 1);
            if (data_hex.length() == MIFARE_BLOCK_SIZE * 2) {
                hex_to_bytes(data_hex.c_str(), card->sectors[sector].block_data[block_in_sector], MIFARE_BLOCK_SIZE);
            }
        }
    }
    
    file.close();
    return true;
}

// Funzioni di configurazione

/**
 * Imposta la configurazione predefinita
 */
void set_default_config(MfcukConfig* config) {
    config->mode = ATTACK_MODE_DARKSIDE;
    config->target_sector = 0;
    config->target_key_type = KEY_A;
    config->known_sector = 0;
    config->known_key_type = KEY_A;
    memset(config->known_key.bytes, 0xFF, MIFARE_KEY_SIZE); // Default key FFFFFFFFFFFF
    config->max_iterations = 1000;
    config->verbose = false;
}

/**
 * Converte la configurazione in una stringa
 */
String config_to_string(const MfcukConfig* config) {
    String result = "";
    
    result += String(config->mode) + "|";
    result += String(config->target_sector) + "|";
    result += String(config->target_key_type) + "|";
    result += String(config->known_sector) + "|";
    result += String(config->known_key_type) + "|";
    
    char key_hex[MIFARE_KEY_SIZE * 2 + 1];
    bytes_to_hex(config->known_key.bytes, key_hex, MIFARE_KEY_SIZE);
    result += String(key_hex) + "|";
    
    result += String(config->max_iterations) + "|";
    result += String(config->verbose ? "1" : "0");
    
    return result;
}

/**
 * Analizza una stringa di configurazione
 */
bool parse_config_string(const String& str, MfcukConfig* config) {
    int start = 0;
    int end = str.indexOf('|', start);
    if (end <= start) return false;
    
    // Modalità di attacco
    config->mode = (MfcukAttackMode)str.substring(start, end).toInt();
    
    // Settore target
    start = end + 1;
    end = str.indexOf('|', start);
    if (end <= start) return false;
    config->target_sector = str.substring(start, end).toInt();
    
    // Tipo di chiave target
    start = end + 1;
    end = str.indexOf('|', start);
    if (end <= start) return false;
    config->target_key_type = str.substring(start, end).toInt();
    
    // Settore con chiave nota
    start = end + 1;
    end = str.indexOf('|', start);
    if (end <= start) return false;
    config->known_sector = str.substring(start, end).toInt();
    
    // Tipo di chiave nota
    start = end + 1;
    end = str.indexOf('|', start);
    if (end <= start) return false;
    config->known_key_type = str.substring(start, end).toInt();
    
    // Chiave nota
    start = end + 1;
    end = str.indexOf('|', start);
    if (end <= start || end - start != MIFARE_KEY_SIZE * 2) return false;
    hex_to_bytes(str.substring(start, end).c_str(), config->known_key.bytes, MIFARE_KEY_SIZE);
    
    // Numero massimo di iterazioni
    start = end + 1;
    end = str.indexOf('|', start);
    if (end <= start) return false;
    config->max_iterations = str.substring(start, end).toInt();
    
    // Modalità verbose
    start = end + 1;
    if (start >= str.length()) return false;
    config->verbose = (str.substring(start).toInt() == 1);
    
    return true;
}

/**
 * Salva la configurazione nel filesystem
 */
void save_config_to_fs(const MfcukConfig* config) {
    File file = LittleFS.open("/mfcuk_config.dat", "w");
    if (file) {
        String config_str = config_to_string(config);
        file.println(config_str);
        file.close();
    }
}

/**
 * Carica la configurazione dal filesystem
 */
bool load_config_from_fs(MfcukConfig* config) {
    File file = LittleFS.open("/mfcuk_config.dat", "r");
    if (!file) {
        // Se il file non esiste, usa la configurazione predefinita
        set_default_config(config);
        return false;
    }
    
    String config_str = file.readStringUntil('\n');
    file.close();
    
    if (!parse_config_string(config_str, config)) {
        // Se il parsing fallisce, usa la configurazione predefinita
        set_default_config(config);
        return false;
    }
    
    return true;
}
