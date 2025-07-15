#ifndef MFOC_KEYS_H
#define MFOC_KEYS_H

#include "mfoc.h"

/**
 * Salva le chiavi trovate durante l'attacco in un file
 * @param filename Nome del file in cui salvare
 * @param card Struttura contenente le chiavi
 * @return true se il salvataggio è riuscito, false altrimenti
 */
bool mfoc_save_keys(const char* filename, MfocCard* card);

/**
 * Carica le chiavi da un file
 * @param filename Nome del file da cui caricare
 * @param card Struttura in cui salvare le chiavi caricate
 * @return true se il caricamento è riuscito, false altrimenti
 */
bool mfoc_load_keys_file(const char* filename, MfocCard* card);

/**
 * Visualizza l'interfaccia per la gestione delle chiavi
 */
void mfoc_key_manager();

/**
 * Visualizza le chiavi memorizzate
 * @param card Struttura contenente le chiavi
 */
void mfoc_view_keys(MfocCard* card);

/**
 * Interfaccia utente per salvare le chiavi
 * @param card Struttura contenente le chiavi
 */
void mfoc_save_keys_ui(MfocCard* card);

/**
 * Interfaccia utente per caricare le chiavi
 * @param card Struttura contenente le chiavi
 */
void mfoc_load_keys_ui(MfocCard* card);

#endif // MFOC_KEYS_H
