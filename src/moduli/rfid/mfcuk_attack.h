/*MFCUK - MiFare Classic Universal toolKit
  Header per gli attacchi
*/

#ifndef _MFCUK_ATTACK_H_
#define _MFCUK_ATTACK_H_

#include <Arduino.h>
#include "mfcuk_types.h"

// Dichiarazioni per gli attacchi
bool mfcuk_darkside_attack(MfcukConfig* config, uint8_t* key);
bool mfcuk_nested_attack(MfcukConfig* config, uint8_t* key);

// Funzioni di utilità per gli attacchi
bool mfcuk_collect_nonces(uint32_t uid, uint8_t block, MifareKey* known_key, uint8_t key_type, uint32_t* nonces, int max_nonces, int* num_collected);
bool mfcuk_recover_key_darkside(uint32_t uid, uint32_t* nonces, int num_nonces, MifareKey* key);
bool mfcuk_recover_key_nested(uint32_t uid, MifareKey* known_key, uint8_t target_sector, uint8_t target_key_type, uint32_t* distances, int num_distances, MifareKey* recovered_key);

#endif // _MFCUK_ATTACK_H_
