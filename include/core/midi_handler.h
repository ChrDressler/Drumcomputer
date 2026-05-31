#pragma once

#include <Arduino.h>
#include <MIDI.h>
#include "core/app_state.h"

/**
 * @brief Initialisiert die MIDI-Callbacks (Note On/Off).
 * @param midiRef Referenz auf die MIDI-Instanz (z.B. MIDI).
 */
void midiHandlerInit(MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>>& midiRef);

// Die MIDI-Trigger-Timer wurden in midi_timer.h/.cpp ausgelagert.
// Statt checkMidiTriggerTimers(), midiTriggerOnTimed() und midiTriggerOffNow()
// werden jetzt midiTimerArm() und midiTimerDisarm() aus midi_timer.h verwendet.
// Der Timer2-ISR (100 µs Auflösung) läuft unabhängig von loop()-Blockaden.

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
