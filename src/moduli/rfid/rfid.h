#ifndef _RFID_H_
#define _RFID_H_

#include "extended_pn532.h"
#include "mfoc.h"

extern Extended_PN532 nfc;

// Funzioni principali RFID
void rfid();
void rfid_read();
void dump();

// Funzionalità MFCUK
void mfcuk_menu();

// Funzionalità MFOC
void mfoc_menu();
void mfoc_menu_mod();
void mfoc_key_manager();

// Menu avanzato RFID
void rfid_advanced_menu();

// Funzioni di utilità
bool rfid_get_uid(uint32_t* uid);
bool rfid_wait_for_tag(uint32_t timeout_ms);
uint8_t get_block_number_by_sector(uint8_t sector, uint8_t block_in_sector);

#endif // _RFID_H_