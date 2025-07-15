/**
 * MFCUK - MiFare Classic Universal toolKit
 * File header principale per ESP32
 */

#ifndef _MFCUK_H_
#define _MFCUK_H_

#include <Arduino.h>
#include "mfcuk_types.h"
#include "mfcuk_crypto.h"

// Stati della macchina a stati MFCUK
typedef enum {
    MFCUK_STATE_INIT,        // Stato iniziale
    MFCUK_STATE_CARD_DETECT, // Rilevamento carta
    MFCUK_STATE_DARKSIDE,    // Esecuzione attacco darkside
    MFCUK_STATE_NESTED,      // Esecuzione attacco nested
    MFCUK_STATE_VERIFY,      // Verifica della chiave
    MFCUK_STATE_COMPLETE     // Attacco completato con successo
} mfcuk_state_t;

// Struttura per fingerprinting degli attacchi
typedef struct {
    uint8_t nt[4];  // Nonce del transponder
    uint8_t nr[4];  // Nonce del reader
    uint8_t ar[4];  // Risposta di autenticazione
    uint32_t uid;   // UID della carta
} mfcuk_fingerprint_t;

// Funzioni principali
void mfcuk_menu();
void mfcuk_config();
bool mfcuk_run(MfcukConfig* config, uint8_t* key = nullptr);

// Funzioni UI
void mfcuk_set_key();
void mfcuk_set_key_type();
void mfcuk_set_target();
void mfcuk_set_mode();
void mfcuk_set_timing();
void mfcuk_update_progress(int progress, const char* status);

// Funzioni di configurazione
void mfcuk_set_default_config();
String mfcuk_config_to_string(MfcukConfig* config);
bool mfcuk_parse_config_string(const String& str, MfcukConfig* config);

// Funzioni di gestione file
void mfcuk_save_result(const String& result);
void mfcuk_load_config();
void mfcuk_save_config();

#endif // _MFCUK_H_
