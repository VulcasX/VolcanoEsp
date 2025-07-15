/**
 * MFCUK - Utilità comuni
 * 
 * Funzioni di utilità per il progetto MFCUK
 */

#ifndef _MFCUK_UTILS_H_
#define _MFCUK_UTILS_H_

#include <Arduino.h>
#include "mfcuk_types.h"

// Funzioni di conversione
uint8_t hex_to_byte(char c);
void hex_to_bytes(const char* hex, uint8_t* bytes, size_t len);
void bytes_to_hex(const uint8_t* bytes, char* hex, size_t len);
uint32_t bytes_to_uint32(const uint8_t* bytes);
void uint32_to_bytes(uint32_t value, uint8_t* bytes);

// Funzioni di visualizzazione
void print_hex(const uint8_t* data, size_t len);
void print_hex_array(const uint8_t* data, size_t len);
void print_mifare_key(const MifareKey* key);
void print_mifare_uid(uint32_t uid);
void dump_mifare_sector(const MifareSector* sector, uint8_t sector_index);
void dump_mifare_card(const MifareCard* card);

// Funzioni di utilità per le schede Mifare
uint8_t get_block_number_by_sector(uint8_t sector, uint8_t block_in_sector);
uint8_t get_sector_by_block(uint8_t block);
bool is_trailer_block(uint8_t block);
uint8_t get_trailer_block_for_sector(uint8_t sector);
void calculate_block_access_bits(uint8_t* block_data, uint8_t block_type, bool read_a, bool write_a, bool read_b, bool write_b);

// Funzioni per il salvataggio e caricamento dei dati
bool save_keys_to_file(const char* filename, const MifareKey keys[MIFARE_MAXSECTOR][2], const bool key_found[MIFARE_MAXSECTOR][2]);
bool load_keys_from_file(const char* filename, MifareKey keys[MIFARE_MAXSECTOR][2], bool key_found[MIFARE_MAXSECTOR][2]);
bool save_card_to_file(const char* filename, const MifareCard* card);
bool load_card_from_file(const char* filename, MifareCard* card);

// Funzioni di configurazione
void set_default_config(MfcukConfig* config);
String config_to_string(const MfcukConfig* config);
bool parse_config_string(const String& str, MfcukConfig* config);
void save_config_to_fs(const MfcukConfig* config);
bool load_config_from_fs(MfcukConfig* config);

#endif // _MFCUK_UTILS_H_
