/**
 * MFCUK - Algoritmi crittografici
 * 
 * Implementazione delle funzioni per la crittografia e il recovery delle chiavi
 * Crypto1 utilizzato da Mifare Classic
 */

#include "mfcuk_crypto.h"

// Tabella di lookup per il filtro non lineare
static const uint8_t filter_table[] = {
    0x00, 0x10, 0x20, 0x30, 0x10, 0x00, 0x30, 0x20,
    0x20, 0x30, 0x00, 0x10, 0x30, 0x20, 0x10, 0x00,
};

/**
 * Inizializza uno stato Crypto1 con una chiave a 48 bit
 */
void crypto1_init(Crypto1State *state, uint64_t key) {
    state->odd = state->even = 0;
    
    // Carica la chiave a 48 bit nello stato iniziale
    for (int i = 47; i > 0; i -= 2) {
        state->odd  = (state->odd  << 1) | ((key >> (i - 1)) & 1);
        state->even = (state->even << 1) | ((key >> i) & 1);
    }
}

/**
 * Funzione filtro non lineare per Crypto1
 */
uint32_t crypto1_filter(uint32_t in) {
    uint32_t x = 0;
    
    // Applica il filtro non lineare a specifici bit di input
    x  = filter_table[((in >> 0) & 1) | ((in >> 2) & 2) | ((in >> 4) & 4) | ((in >> 6) & 8)];
    x |= filter_table[((in >> 8) & 1) | ((in >> 10) & 2) | ((in >> 12) & 4) | ((in >> 14) & 8)] << 1;
    x |= filter_table[((in >> 16) & 1) | ((in >> 18) & 2) | ((in >> 20) & 4) | ((in >> 22) & 8)] << 2;
    x |= filter_table[((in >> 24) & 1) | ((in >> 26) & 2) | ((in >> 28) & 4) | ((in >> 30) & 8)] << 3;
    
    return x;
}

/**
 * Aggiorna lo stato Crypto1 con un bit di input
 */
void update_contribution(Crypto1State *state, uint8_t in) {
    int feedback;
    
    // Aggiorna il registro pari
    feedback = (state->even & 1) ^ ((state->even >> 16) & 1) ^ 
               ((state->even >> 19) & 1) ^ ((state->even >> 21) & 1) ^ in;
    state->even = state->even >> 1 | (feedback << 23);
    
    // Aggiorna il registro dispari
    feedback = (state->odd & 1) ^ ((state->odd >> 16) & 1) ^ 
               ((state->odd >> 19) & 1) ^ ((state->odd >> 21) & 1);
    state->odd = state->odd >> 1 | (feedback << 23);
}

/**
 * Processa un singolo bit con Crypto1
 */
uint8_t crypto1_bit(Crypto1State *state, uint8_t in, uint8_t is_encrypted) {
    uint8_t out;
    
    // Calcola il bit di output usando il filtro
    out = crypto1_filter(state->odd) ^ crypto1_filter(state->even);
    
    // Se il bit è criptato, XOR l'input con l'output per decifrarlo
    if (is_encrypted)
        in ^= out;
    
    // Aggiorna lo stato con il bit di input
    update_contribution(state, in);
    
    return out;
}

/**
 * Processa un byte con Crypto1
 */
void crypto1_byte(Crypto1State *state, uint8_t *in, uint8_t *out, uint8_t is_encrypted) {
    *out = 0;
    
    // Processa il byte bit per bit
    for (int i = 0; i < 8; i++) {
        *out |= (crypto1_bit(state, (*in >> i) & 1, is_encrypted) << i);
    }
}

/**
 * Crea un nuovo stato Crypto1 inizializzato con una chiave
 */
Crypto1State* crypto1_create(uint64_t key) {
    Crypto1State* state = (Crypto1State*)malloc(sizeof(Crypto1State));
    if (state) {
        crypto1_init(state, key);
    }
    return state;
}

/**
 * Libera la memoria di uno stato Crypto1
 */
void crypto1_destroy(Crypto1State* state) {
    if (state) {
        free(state);
    }
}

/**
 * Calcola il successore del generatore PRNG
 */
uint32_t prng_successor(uint32_t x, uint32_t n) {
    uint32_t i, nonce = x;
    
    for (i = 0; i < n; i++) {
        nonce = ((nonce << 16) ^ (nonce >> 16)) + nonce;
    }
    
    return nonce;
}

/**
 * Implementazione dell'algoritmo Darkside per il recupero delle chiavi
 * Versione semplificata - in una implementazione completa sarebbe più complesso
 */
bool darkside_key_recovery(uint32_t uid, uint32_t nonce, uint64_t *key) {
    // Questa è una versione semplificata solo per dimostrare la struttura
    // L'implementazione completa richiederebbe l'intero algoritmo di attacco darkside
    
    // In una implementazione reale, si calcolerebbero le chiavi candidate basate su
    // UID e nonce raccolti, e si verificherebbero contro il lettore
    
    // Implementazione di esempio (non funzionante, solo come placeholder)
    uint64_t candidate_key = 0;
    bool found = false;
    
    // Nota: implementazione reale richiede un'analisi più complessa dei dati raccolti
    // e la verifica delle chiavi candidate con autenticazioni di prova
    
    if (found) {
        *key = candidate_key;
        return true;
    }
    
    return false;
}

/**
 * Implementazione dell'algoritmo Nested per il recupero delle chiavi
 * Versione semplificata - in una implementazione completa sarebbe più complesso
 */
bool nested_key_recovery(uint32_t* distances, int numDistances, uint64_t* key) {
    // Questa è una versione semplificata solo per dimostrare la struttura
    // L'implementazione completa richiederebbe la ricerca di stati validi basati
    // sulle distanze tra nonce
    
    // In una implementazione reale, si analizzerebbero le distanze per trovare
    // gli stati interni candidati del PRNG e si ricostruirebbe la chiave
    
    // Implementazione di esempio (non funzionante, solo come placeholder)
    uint64_t candidate_key = 0;
    bool found = false;
    
    // Nota: implementazione reale richiede un'analisi delle distanze tra nonce
    // e una ricerca dello spazio degli stati interni validi
    
    if (found) {
        *key = candidate_key;
        return true;
    }
    
    return false;
}
