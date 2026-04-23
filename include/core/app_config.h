#pragma once

#include <Arduino.h>

enum MenuMode { MENU_ROOT, PLAY, EDIT_BANK, EDIT_PULSE, INFO };

struct Bank {
  // WICHTIG: Der Bankname wird direkt in PROGMEM als festes char-Array gespeichert.
  // Früher war hier ein const char* und der Zugriff über PROGMEM-Pointer war fehleranfällig.
  // Mit einem festen Array lassen sich die Namen sicher mit strncpy_P() auslesen.
  const char name[19];
  uint16_t patterns[8][4];
};

static const int bankAddr = 0;
static const int selectBtn = 10;
static const int trigPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
static const int potPinBPM = A2;
static const int potPinSwing = A1;
static const int ENCODER_STEPS = 4;