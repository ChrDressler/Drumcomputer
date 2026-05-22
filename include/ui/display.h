#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "core/app_config.h"

/**
 * @brief Aktualisiert die LCD-Ausgabe entsprechend des aktuellen Menue- und Laufzeitstatus.
 * @details
 * Die Implementierung in src/ui/display.cpp ist in modusspezifische Render-Methoden
 * aufgeteilt (Root-Menue, Play, Edit Pulsbreite, Info, Edit Bank). Diese Funktion
 * uebernimmt nur die zentrale Steuerung:
 * - Redraw-/Timing-Gating (PLAY ueber periodisches Due-Flag, sonst ereignisbasiert)
 * - Moduswechsel-Erkennung inklusive Display-Clear
 * - Delegation an den passenden Renderer je nach MenuMode
 * @param lcd LCD-Instanz.
 * @param currentMode Aktueller Menue-Modus.
 * @param menuIndex Aktuelle Root-Menue-Auswahl.
 * @param needsRedraw [in,out] Flag zum Erzwingen einer Neuzeichnung.
 * @param playRefreshDue True, wenn PLAY fuer ein periodisches Refresh faellig ist.
 * @param bpm Aktuelles Tempo in BPM.
 * @param swingAmount Aktueller Swing-Anteil.
 * @param pulseWidth Aktuelle Trigger-Pulsbreite.
 * @param currentBank Aktuell selektierte Bank.
 * @param lastBank [in,out] Zuletzt gezeichneter Bank-Index.
 * @param isRunning Flag zum Anzeigen des Wiedergabestatus.
 */
void updateDisplay(
  LiquidCrystal_I2C& lcd,
  MenuMode currentMode,
  int menuIndex,
  bool& needsRedraw,
  bool playRefreshDue,
  int bpm,
  float swingAmount,
  long pulseWidth,
  int currentBank,
  int& lastBank,
  bool isRunning,
  const char* midiMsg
);
