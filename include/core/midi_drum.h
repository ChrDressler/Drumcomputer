#pragma once

#include <Arduino.h>
#include "core/app_config.h"

/**
 * @brief General MIDI Percussion Key-to-Channel Mapping.
 *
 * GM-Standard (Kanal 10): Jeder Note ist ein bestimmtes Instrument zugeordnet.
 * Hier wird auf die 8 Drum-Kanäle (0..7) gemappt.
 *
 * Rückgabe: Kanalindex (0..7) oder -1 wenn die Note nicht gemappt ist.
 */
inline int midiNoteToChannel(byte pitch) {
    switch (pitch) {
        // Bass Drum
        case 35:  // Acoustic Bass Drum
        case 36:  // Bass Drum 1
            return ChBD;

        // Low Tom
        case 41:  // Low Floor Tom
        case 43:  // High Floor Tom
            return ChLT;

        // High Tom
        case 45:  // Low Tom
        case 47:  // Low-Mid Tom
        case 48:  // High-Mid Tom
        case 50:  // High Tom
            return ChHT;

        // Claves
        case 54:  // Claves. Im GM 75, ist aber zu weit weg
            return ChCL;

        // Snare
        case 38:  // Acoustic Snare
        case 40:  // Electric Snare
            return ChSN;

        // Crash Cymbal
        case 49:  // Crash Cymbal 1
        case 57:  // Crash Cymbal 2
            return ChCY;

        // Closed Hi-Hat (gleicher Pin wie OHH, Pin 8)
        case 42:  // Closed Hi-Hat
        case 44:  // Pedal Hi-Hat
            return ChHH;

        // Cowbell
        case 56:  // Cowbell
            return ChCB;

        // Open Hi-Hat (gleicher Pin wie CHH, Pin 8)
        case 46:  // Open Hi-Hat
            return ChOH;  // Spezial-Index für OHH

        default:
            return -1;  // Nicht gemappt
    }
}

/**
 * @brief Setzt einen Trigger-Pin auf LOW (aktiv).
 * @param channel Kanalindex (0..7).
 */
inline void midiTriggerOn(int channel) {
    if (channel < 0 || channel >= ChMax) return;
    int pin = trigPins[channel];
    if (pin <= 7) {
        PORTD &= ~(1 << pin);
    } else {
        PORTB &= ~(1 << (pin - 8));
    }
}

/**
 * @brief Setzt einen Trigger-Pin auf HIGH (inaktiv).
 * @param channel Kanalindex (0..7).
 */
inline void midiTriggerOff(int channel) {
    if (channel < 0 || channel >= ChMax) return;
    int pin = trigPins[channel];
    if (pin <= 7) {
        PORTD |= (1 << pin);
    } else {
        PORTB |= (1 << (pin - 8));
    }
}
