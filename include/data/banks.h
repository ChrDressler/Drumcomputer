#pragma once

#include <stddef.h>

#include "core/app_config.h"

extern const int NUM_BANKS;

/**
 * @brief Liest ein 16-Bit-Patternwort aus PROGMEM.
 * @param bankIndex Index der Bank.
 * @param channel Kanalindex (0..7).
 * @param bar Taktindex innerhalb der Bank (0..3).
 * @return Das gelesene 16-Bit-Patternwort.
 */
uint16_t getBankPatternWord(uint8_t bankIndex, uint8_t channel, uint8_t bar);

/**
 * @brief Kopiert den Banknamen aus PROGMEM in einen RAM-Puffer.
 * @param bankIndex Index der Bank.
 * @param outName Zielpuffer im RAM.
 * @param outSize Groesse des Zielpuffers in Byte.
 */
void getBankName(uint8_t bankIndex, char* outName, size_t outSize);
