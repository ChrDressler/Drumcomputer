#pragma once

#include <Arduino.h>
#include <MIDI.h>
#include "core/app_state.h"

/**
 * @brief Initialisiert die MIDI-Callbacks (Note On/Off).
 * @param midiRef Referenz auf die MIDI-Instanz (z.B. MIDI).
 */
void midiHandlerInit(MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>>& midiRef);

/**
 * @brief Prüft, ob MIDI-Trigger-Timer abgelaufen sind, und schaltet die Pins aus.
 * Muss regelmässig in loop() aufgerufen werden.
 */
void checkMidiTriggerTimers();

/**
 * @brief MIDI-Trigger einschalten mit automatischem Timer-Ausschalten.
 * @param channel Kanalindex (0..7).
 * @param pulseWidthUs Pulsbreite in Mikrosekunden.
 */
void midiTriggerOnTimed(int channel, uint32_t pulseWidthUs);

/**
 * @brief MIDI-Trigger sofort ausschalten und Timer löschen.
 * @param channel Kanalindex (0..7).
 */
void midiTriggerOffNow(int channel);

/**
 * @brief Callback für MIDI Note On.
 */
void handleNoteOn(byte channel, byte pitch, byte velocity);

/**
 * @brief Callback für MIDI Note Off.
 */
void handleNoteOff(byte channel, byte pitch, byte velocity);

/**
 * @brief MIDI-Note (0-127) in Notennamen mit Oktave umwandeln.
 */
const char* noteName(byte note);
