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
            return 0;

        // Low Tom
        case 41:  // Low Floor Tom
        case 43:  // High Floor Tom
            return 1;

        // High Tom
        case 45:  // Low Tom
        case 47:  // Low-Mid Tom
        case 48:  // High-Mid Tom
        case 50:  // High Tom
            return 2;

        // Claves
        case 54:  // Claves. Im GM 75, ist aber zu weit weg
            return 3;

        // Snare
        case 38:  // Acoustic Snare
        case 40:  // Electric Snare
            return 4;

        // Crash Cymbal
        case 49:  // Crash Cymbal 1
        case 57:  // Crash Cymbal 2
            return 5;

        // Closed Hi-Hat (gleicher Pin wie OHH, Pin 8)
        case 42:  // Closed Hi-Hat
        case 44:  // Pedal Hi-Hat
            return 6;

        // Cowbell
        case 56:  // Cowbell
            return 7;

        // Open Hi-Hat (gleicher Pin wie CHH, Pin 8)
        case 46:  // Open Hi-Hat
            return 8;  // Spezial-Index für OHH

        default:
            return -1;  // Nicht gemappt
    }
}

/**
 * @brief Setzt einen Trigger-Pin auf LOW (aktiv).
 * @param channel Kanalindex (0..7).
 */
inline void midiTriggerOn(int channel) {
    if (channel < 0 || channel > 7) return;
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
    if (channel < 0 || channel > 7) return;
    int pin = trigPins[channel];
    if (pin <= 7) {
        PORTD |= (1 << pin);
    } else {
        PORTB |= (1 << (pin - 8));
    }
}
