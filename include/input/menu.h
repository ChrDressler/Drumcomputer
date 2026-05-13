#pragma once

#include <Arduino.h>
#include <Encoder.h>

#include "core/app_config.h"

/**
 * @brief Verarbeitet Button- und Encoder-Eingaben fuer das Menuesystem.
 * @param encoder Encoder-Instanz fuer Navigation.
 * @param currentMode [in,out] Aktueller Menue-Modus.
 * @param menuIndex [in,out] Auswahlindex im Root-Menue.
 * @param needsRedraw [in,out] Flag zum Erzwingen eines Display-Redraws.
 * @param oldEncoderPos [in,out] Vorherige Encoderposition zur Delta-Berechnung.
 * @param currentBank [in,out] Aktuell selektierte Bank.
 * @param pulseWidth [in,out] Pulsbreite im Edit-Modus.
 * @param lastBtnState [in,out] Letzter gelesener Zustand des Select-Buttons.
 */
void handleMenuSystem(
  Encoder& encoder,
  uint32_t nowTicks,
  MenuMode& currentMode,
  int& menuIndex,
  bool& needsRedraw,
  long& oldEncoderPos,
  int& currentBank,
  long& pulseWidth,
  bool& lastBtnState,
  uint32_t& menuDebounceUntilTick
);

void handleStartStop( 
  uint32_t nowTicks,
  bool& isRunning,
  bool& lastStartStopBtnState, 
  int& globalStep,
  bool& needsRedraw,
  uint32_t& startStopDebounceUntilTick
);
