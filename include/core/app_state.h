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

  unsigned long lastStepTime;      // Zeitmarke des letzten Sequencer-Schritts
  unsigned long lastPotRead;       // Zeitmarke der letzten Poti-Abfrage
  unsigned long lastBeatTime;      // Zeitmarke des letzten Beat-Starts
  unsigned long ledOffTime;        // Zeitpunkt zum Abschalten der Beat-LED
  unsigned long triggerOffTime;    // Zeitpunkt zum Abschalten der Trigger-Ausgänge
  unsigned long lastDisplayUpdate; // Zeitmarke des letzten Display-Updates

  int bpm;                         // Aktuelles Tempo in BPM
  int globalStep;                  // Aktueller Step im 64-Step-Pattern
  float swingAmount;               // Swing-Anteil (0.0 bis 0.4)
  bool triggersActive;             // True, solange Trigger-Ausgänge aktiv sind
};
