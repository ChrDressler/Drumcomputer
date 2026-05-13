#pragma once

#include <Arduino.h>

#include "core/app_config.h"

/**
 * @brief Zentraler Laufzeitzustand der Anwendung (Menue, Timing, Pattern-Status).
 */
struct AppState {
  MenuMode currentMode;            // Aktueller Menümodus
  int menuIndex;                   // Aktuelle Auswahl im Root-Menü
  bool needsRedraw;                // Erzwingt Neuzeichnen des Displays

  long pulseWidth;                 // Trigger-Pulsbreite in Mikrosekunden
  uint8_t probability[8];          // Trigger-Wahrscheinlichkeit pro Kanal in Prozent
  int lastBank;                    // Zuletzt angezeigte Bank (für Display-Update)
  int currentBank;                 // Aktuell ausgewählte Bank

  bool lastBtnState;               // Letzter gelesener Zustand des Select-Buttons
  long oldEncoderPos;              // Letzte Encoderposition zur Delta-Berechnung
  uint32_t menuDebounceUntilTick;  // Sperrzeit fuer Menue-Button-Entprellung
  uint32_t startStopDebounceUntilTick; // Sperrzeit fuer Start/Stop-Entprellung

  int bpm;                         // Aktuelles Tempo in BPM
  int globalStep;                  // Aktueller Step im 64-Step-Pattern
  float swingAmount;               // Swing-Anteil (0.0 bis 0.4)

  bool isRunning = false;             // Gibt an, ob der Sequencer läuft (Start/Stop-Status)
  bool lastStartStopBtnState = HIGH;  // Für die Flankenerkennung
};
