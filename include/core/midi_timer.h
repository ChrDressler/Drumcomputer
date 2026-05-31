#pragma once

#include <Arduino.h>
#include "core/app_config.h"

/**
 * @brief Initialisiert Timer2 für MIDI-Trigger-Timer (100 µs Auflösung).
 *        Läuft unabhängig vom Sequenzer-Timer (Timer1) und garantiert,
 *        dass MIDI-Trigger-Pins auch bei blockierenden Display-Updates
 *        rechtzeitig ausgeschaltet werden.
 */
void midiTimerInit();

/**
 * @brief MIDI-Trigger einschalten und Timer für automatisches Ausschalten setzen.
 * @param channel Kanalindex (0..7).
 * @param pulseWidthUs Pulsbreite in Mikrosekunden.
 */
void midiTimerArm(int channel, uint32_t pulseWidthUs);

/**
 * @brief MIDI-Trigger sofort ausschalten und Timer löschen.
 * @param channel Kanalindex (0..7).
 */
void midiTimerDisarm(int channel);

/**
 * @brief True solange OHH-Impuls aktiv ist (Open Hi-Hat).
 *        Wird von midi_handler.cpp gesetzt/gelesen.
 */
extern volatile bool gMidiOhhActive;
