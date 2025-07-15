/**
 * MFCUK - Definizioni dei tipi di dati comuni
 * 
 * Questo file contiene le definizioni dei tipi di dati comuni
 * utilizzati nell'implementazione di MFCUK
 */

#ifndef _MFCUK_TYPES_H_
#define _MFCUK_TYPES_H_

#include <Arduino.h>
#include "mfcuk_crypto.h"

// Dimensioni per le schede Mifare Classic
#define MIFARE_1K_MAXSECTOR   16
#define MIFARE_4K_MAXSECTOR   40
#define MIFARE_MAXSECTOR      MIFARE_4K_MAXSECTOR

// Numero di blocchi e chiavi per settore
#define MIFARE_BLOCKS_PER_SECTOR   4
#define MIFARE_KEYS_PER_SECTOR     2

// Dimensioni delle chiavi e dei dati
#define MIFARE_KEY_SIZE            6      // 48 bit
#define MIFARE_UID_SIZE            4      // 32 bit
#define MIFARE_BLOCK_SIZE         16      // 16 byte

// Tipi di chiavi
enum MifareKeyType {
    KEY_A = 0,
    KEY_B = 1
};

// Struttura per le chiavi
typedef struct {
    uint8_t bytes[MIFARE_KEY_SIZE];
} MifareKey;

// Struttura per i settori di una carta
typedef struct {
    bool     authenticated_a;                        // Settore autenticato con KEY A
    bool     authenticated_b;                        // Settore autenticato con KEY B
    MifareKey key_a;                                 // KEY A per il settore
    MifareKey key_b;                                 // KEY B per il settore
    uint8_t   block_data[MIFARE_BLOCKS_PER_SECTOR][MIFARE_BLOCK_SIZE]; // Dati dei blocchi
} MifareSector;

// Struttura per una carta Mifare Classic completa
typedef struct {
    uint32_t    uid;                      // ID univoco della carta
    uint8_t     sak;                      // SAK (Select Acknowledge)
    uint8_t     atqa[2];                  // ATQA (Answer To Request Type A)
    uint8_t     ats[32];                  // ATS (Answer To Select)
    uint8_t     ats_len;                  // Lunghezza ATS
    uint8_t     num_sectors;              // Numero di settori (16 per 1K, 40 per 4K)
    MifareSector sectors[MIFARE_MAXSECTOR]; // Dati dei settori
} MifareCard;

// Modalità di attacco
enum MfcukAttackMode {
    ATTACK_MODE_NONE = 0,
    ATTACK_MODE_DARKSIDE,
    ATTACK_MODE_NESTED
};

// Stato dell'attacco
enum MfcukAttackState {
    ATTACK_STATE_IDLE = 0,
    ATTACK_STATE_RUNNING,
    ATTACK_STATE_FINISHED,
    ATTACK_STATE_FAILED
};

// Configurazione dell'attacco
typedef struct {
    MfcukAttackMode mode;                // Modalità di attacco
    uint8_t target_sector;               // Settore target
    uint8_t target_key_type;             // Tipo di chiave target (KEY_A o KEY_B)
    uint8_t known_sector;                // Settore con chiave nota
    uint8_t known_key_type;              // Tipo di chiave nota (KEY_A o KEY_B)
    MifareKey known_key;                 // Chiave nota
    uint32_t max_iterations;             // Numero massimo di iterazioni
    uint16_t sleepTime;                  // Tempo di attesa tra tentativi (ms)
    bool verbose;                        // Modalità verbose
} MfcukConfig;

// Risultati dell'attacco
typedef struct {
    MfcukAttackState state;              // Stato dell'attacco
    uint32_t iterations;                 // Numero di iterazioni eseguite
    uint32_t elapsed_time;               // Tempo trascorso (ms)
    uint8_t recovered_keys;              // Numero di chiavi recuperate
    MifareKey keys[MIFARE_MAXSECTOR][2]; // Chiavi recuperate (KEY_A e KEY_B per ogni settore)
    bool key_found[MIFARE_MAXSECTOR][2]; // Flag per chiavi trovate
} MfcukResult;

#endif // _MFCUK_TYPES_H_
