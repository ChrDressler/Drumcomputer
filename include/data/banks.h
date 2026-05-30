#pragma once

#include <stddef.h>

#include "core/app_config.h"

extern const int NUM_BANKS;

/**
 * @brief Liest die Schritte-pro-Takt-Einstellung einer Bank aus PROGMEM.
 * @param bankIndex Index der Bank.
 * @return stepsPerBar (z. B. 16 fuer 4/4, 12 fuer 3/4).
 */
uint8_t getBankStepsPerBar(uint8_t bankIndex);

/**
 * @brief Liest ein 32-Bit-Patternwort aus PROGMEM.
 * @param bankIndex Index der Bank.
 * @param channel Kanalindex (0..7).
 * @param bar Taktindex innerhalb der Bank (0..3).
 * @return Das gelesene 32-Bit-Patternwort.
 *         Bits 0-15 = Hit-Maske (X oder O = 1)
 *         Bits 16-31 = Dead-Node-Maske (nur O = 1)
 */
uint32_t getBankPatternWord(uint8_t bankIndex, uint8_t channel, uint8_t bar);

/**
 * @brief Kopiert den Banknamen aus PROGMEM in einen RAM-Puffer.
 * @param bankIndex Index der Bank.
 * @param outName Zielpuffer im RAM.
 * @param outSize Groesse des Zielpuffers in Byte.
 */
void getBankName(uint8_t bankIndex, char* outName, size_t outSize);
