#pragma once

#include <Arduino.h>

enum MenuMode { MENU_ROOT, PLAY, EDIT_BANK, EDIT_PULSE, INFO, MIDI_MONITOR };

// Channel-Konstanten (8 Kanäle + OHH als Index 8)
static const int ChMax = 8;  // Anzahl der physikalischen Kanäle
static const int ChBD = 0;   // Bassdrum
static const int ChLT = 1;   // Low Tom
static const int ChHT = 2;   // High Tom
static const int ChCL = 3;   // Claves
static const int ChSN = 4;   // Snare
static const int ChCY = 5;   // Cymbal
static const int ChHH = 6;   // Hi-Hat (CHH + OHH teilen sich Pin 8)
static const int ChCB = 7;   // Cowbell
static const int ChOH = 8;   // Open Hi-Hat (Spezial-Index, teilt Pin mit ChHH)

struct Bank {
  // WICHTIG: Der Bankname wird direkt in PROGMEM als festes char-Array gespeichert.
  // Früher war hier ein const char* und der Zugriff über PROGMEM-Pointer war fehleranfällig.
  // Mit einem festen Array lassen sich die Namen sicher mit strncpy_P() auslesen.
  const char name[19];
  uint8_t stepsPerBar;            // Schritte pro Takt: 16 für 4/4, 12 für 3/4
  uint32_t patterns[9][4];  // 0..7 = Kanaele, 8 = Open Hi-Hat
                            // Jedes Pattern ist 32 Bit:
                            // - Bits 0-15 = Hit-Maske (X oder O = 1)
                            // - Bits 16-31 = Dead-Node-Maske (nur O = 1)
};

static const int bankAddr = 0;
static const int pulseWidthAddr = 1;
static const int selectBtn = 10;
static const int trigPins[ChMax] = {2, 3, 4, 5, 6, 7, 8, 9};
static const int potPinBPM = A2;
static const int potPinSwing = A1;
static const int ENCODER_STEPS = 4;

// µs pro Minute (für BPM-Berechnung: 60.000.000 µs = 1 Minute)
static const uint32_t US_PER_MINUTE = 60000000UL;

// Port-Masken für Trigger-Pins D2-D9
// D2-D7 liegen auf PORTD (Bits 2-7), D8-D9 auf PORTB (Bits 0-1)
static const uint8_t kPortDMask = 0b11111100;
static const uint8_t kPortBMask = 0b00000011;
