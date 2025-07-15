/**
 * MFOC - Mifare Classic Offline Cracker
 * Implementazione unificata per ESP32 con display OLED
 * 
 * Basato su:
 * - mfoc originale (https://github.com/nfc-tools/mfoc)
 * - Adattato per funzionare con la nostra implementazione MFCUK
 * - Versione migliorata con funzionalità estese
 */

#ifndef MFOC_H
#define MFOC_H

#include <Arduino.h>
#include "mfcuk_types.h"
#include "mfcuk_crypto.h"

// Strutture dati per MFOC
typedef struct mfoc_denonce {
    uint32_t* distances;
    uint32_t num_distances;
    uint32_t tolerance;
    uint32_t median;
} mfoc_denonce;

typedef struct mfoc_pKeys {
    uint64_t* possibleKeys;
    uint32_t size;
} mfoc_pKeys;

typedef struct mfoc_bKeys {
    uint64_t* brokenKeys;
    uint32_t size;
} mfoc_bKeys;

typedef struct mfoc_countKeys {
    uint64_t key;
    uint32_t count;
} mfoc_countKeys;

// ----- COSTANTI -----
// Numero predefinito di tentativi
#define DEFAULT_PROBES_NR 20
// Numero predefinito di set di chiavi
#define DEFAULT_SETS_NR 5
// Tolleranza per le distanze di nonce
#define DEFAULT_TOLERANCE 20
// Numero di distanze di nonce da raccogliere
#define DEFAULT_DIST_NR 15
// Numero di chiavi da provare per settore
#define TRY_KEYS 15
// Chunk di memoria per le chiavi possibili
#define MEM_CHUNK 10000

// ----- TIPI DI DATI E STRUTTURE -----
// Le strutture mfoc_denonce, mfoc_pKeys, mfoc_bKeys e mfoc_countKeys sono già definite sopra
/*
// COMMENTO VECCHIE DICHIARAZIONI
*/
// Struttura per memorizzare la chiave
typedef struct {
    uint8_t bytes[MIFARE_KEY_SIZE];
} MfocKey;

// Struttura per memorizzare le informazioni del settore
typedef struct {
    uint8_t trailerBlock;    // Numero del blocco trailer
    MfocKey KeyA;           // Chiave A
    MfocKey KeyB;           // Chiave B
    bool foundKeyA;         // Flag per indicare se la chiave A è stata trovata
    bool foundKeyB;         // Flag per indicare se la chiave B è stata trovata
} MfocSector;

// Struttura per memorizzare le informazioni della carta
typedef struct {
    uint32_t uid;                      // ID univoco della carta
    uint8_t num_sectors;              // Numero di settori
    MfocSector sectors[MIFARE_MAXSECTOR]; // Dati dei settori
} MfocCard;

// Configurazione per MFOC
typedef struct {
    uint8_t known_key_type;      // Tipo di chiave conosciuta (A o B)
    MifareKey known_key;         // Chiave conosciuta 
    uint8_t target_sector;       // Settore target
    uint8_t target_key_type;     // Tipo di chiave target (A o B)
    uint32_t max_iterations;     // Numero massimo di iterazioni
    uint32_t num_probes;         // Numero di tentativi per settore
    uint32_t sets;               // Numero di set di chiavi
    uint32_t tolerance;          // Tolleranza per le distanze di nonce
    bool load_keys_from_file;    // Caricare chiavi da file
    char keys_file[32];          // Nome file per le chiavi
} MfocConfig;

// ----- FUNZIONI VERSIONE ORIGINALE -----

// Funzioni principali
void mfoc_menu();
void mfoc_config();
bool mfoc_run(MfocConfig* config, MfocCard* card = nullptr);

// Funzioni di configurazione
void mfoc_set_known_key();
void mfoc_set_known_key_type();
void mfoc_set_target();
void mfoc_set_key_file();
void mfoc_set_iterations();

// Funzioni di utilità
bool mfoc_load_keys_from_file(const char* filename, mfoc_pKeys* keys);
void mfoc_update_progress(int progress, const char* status);
uint32_t mfoc_median(mfoc_denonce* d);
int mfoc_compare_keys(const void* a, const void* b);
mfoc_countKeys* mfoc_uniqsort_keys(uint64_t* possibleKeys, uint32_t size);
bool mfoc_valid_nonce(uint32_t Nt, uint32_t NtEnc, uint32_t Ks1, uint8_t* parity);
uint64_t bytes_to_num(uint8_t* src, uint32_t len);

// Funzioni di attacco
int mfoc_enhanced_auth(uint8_t e_sector, uint8_t a_sector, MfocCard* card, 
                     mfoc_denonce* d, mfoc_pKeys* pk, char mode, bool dumpKeysA);
int mfoc_find_exploit_sector(MfocCard* card);
bool mfoc_collect_nonces(MfocCard* card, uint8_t sector, mfoc_denonce* d);
bool mfoc_recover_key(MfocCard* card, uint8_t sector, uint8_t key_type, mfoc_denonce* d, mfoc_pKeys* pk);

// ----- FUNZIONI VERSIONE MODIFICATA -----

// Funzioni principali
void mfoc_menu_mod();
void mfoc_config_mod();
bool mfoc_run_mod(MfocConfig* config, MfocCard* card = nullptr);

// Funzioni di configurazione
void mfoc_set_known_key_mod();
void mfoc_set_known_key_type_mod();
void mfoc_set_target_mod();
void mfoc_set_key_file_mod();
void mfoc_set_iterations_mod();

// Funzioni di utilità
bool mfoc_load_keys_from_file_mod(const char* filename, mfoc_pKeys* keys);
void mfoc_update_progress_mod(int progress, const char* status);
uint32_t mfoc_median_mod(mfoc_denonce* d);
int mfoc_compare_keys_mod(const void* a, const void* b);
mfoc_countKeys* mfoc_uniqsort_keys_mod(uint64_t* possibleKeys, uint32_t size);
bool mfoc_valid_nonce_mod(uint32_t Nt, uint32_t NtEnc, uint32_t Ks1, uint8_t* parity);

// Funzioni di attacco
int mfoc_enhanced_auth_mod(uint8_t e_sector, uint8_t a_sector, MfocCard* card, 
                     mfoc_denonce* d, mfoc_pKeys* pk, char mode, bool dumpKeysA);
int mfoc_find_exploit_sector_mod(MfocCard* card);
bool mfoc_collect_nonces_mod(MfocCard* card, uint8_t sector, mfoc_denonce* d);
bool mfoc_recover_key_mod(MfocCard* card, uint8_t sector, uint8_t key_type, mfoc_denonce* d, mfoc_pKeys* pk);

// File manager per le chiavi
bool mfoc_select_key_file(char* filename, size_t max_len);
bool mfoc_select_key_file_mod(char* filename, size_t max_len);

// Funzione per navigare nel filesystem
String browse_files(const char* directory);

// Funzione per dump completo (recupero di tutte le chiavi e dump della carta)
void mfoc_dump_complete();
bool mfoc_run_complete_dump(MfocCard* card);
bool mfoc_save_keys(MfocCard* card, const char* filename);
bool mfoc_save_dump(MfocCard* card, const char* mfd_filename, const char* txt_filename);

#endif // MFOC_H
