#pragma once

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "core/app_config.h"

/**
 * @brief Aktualisiert die LCD-Ausgabe entsprechend des aktuellen Menue- und Laufzeitstatus.
 * @param lcd LCD-Instanz.
 * @param currentMode Aktueller Menue-Modus.
 * @param menuIndex Aktuelle Root-Menue-Auswahl.
 * @param needsRedraw [in,out] Flag zum Erzwingen einer Neuzeichnung.
 * @param lastDisplayUpdate [in,out] Zeitmarke des letzten Display-Updates.
 * @param bpm Aktuelles Tempo in BPM.
 * @param swingAmount Aktueller Swing-Anteil.
 * @param pulseWidth Aktuelle Trigger-Pulsbreite.
 * @param currentBank Aktuell selektierte Bank.
 * @param lastBank [in,out] Zuletzt gezeichneter Bank-Index.
 */
void updateDisplay(
  LiquidCrystal_I2C& lcd,
  MenuMode currentMode,
  int menuIndex,
  bool& needsRedraw,
  unsigned long& lastDisplayUpdate,
  int bpm,
  float swingAmount,
  long pulseWidth,
  int currentBank,
  int& lastBank
);
