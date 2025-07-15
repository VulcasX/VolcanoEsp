/**
 * MFCUK - Algoritmi crittografici
 * 
 * Implementazione delle funzioni per la crittografia e il recovery delle chiavi
 * Crypto1 utilizzato da Mifare Classic
 */

#ifndef _MFCUK_CRYPTO_H_
#define _MFCUK_CRYPTO_H_

#include <Arduino.h>

// Struttura per lo stato Crypto1
typedef struct {
    uint32_t odd;     // bit dispari
    uint32_t even;    // bit pari
} Crypto1State;

// Costanti per LFSR (Linear Feedback Shift Register)
#define FEEDBACK_IN_1   0x04
#define FEEDBACK_IN_2   0x08
#define FEEDBACK_OUT    0x02
#define LF_POLY_ODD    0x29CE5C
#define LF_POLY_EVEN   0x870804

// Funzioni principali Crypto1
void crypto1_init(Crypto1State *state, uint64_t key);
void crypto1_byte(Crypto1State *state, uint8_t *in, uint8_t *out, uint8_t is_encrypted);
uint8_t crypto1_bit(Crypto1State *state, uint8_t in, uint8_t is_encrypted);
uint32_t crypto1_filter(uint32_t in);

// Funzioni di gestione dello stato
Crypto1State* crypto1_create(uint64_t key);
void crypto1_destroy(Crypto1State* state);
void update_contribution(Crypto1State *state, uint8_t in);

// Funzioni per PRNG
uint32_t prng_successor(uint32_t x, uint32_t n);

// Funzioni per il recupero delle chiavi
bool darkside_key_recovery(uint32_t uid, uint32_t nonce, uint64_t *key);
bool nested_key_recovery(uint32_t* distances, int numDistances, uint64_t* key);
bool darkside_crack(uint32_t nt, uint32_t nr, uint32_t ar, uint64_t *key);

#endif // _MFCUK_CRYPTO_H_
