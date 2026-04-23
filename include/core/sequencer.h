#pragma once

#include <Arduino.h>

/**
 * @brief Fuehrt einen Sequencer-Tick aus (Timing, Trigger setzen und ruecksetzen).
 * @param now Aktuelle Zeit in Mikrosekunden (typisch aus micros()).
 * @param bpm Aktuelles Tempo in BPM.
 * @param swingAmount Swing-Anteil als Faktor (z. B. 0.0 bis 0.4).
 * @param currentBank Index der aktuell aktiven Pattern-Bank.
 * @param probability Trigger-Wahrscheinlichkeit pro Kanal in Prozent.
 * @param pulseWidth Trigger-Pulsbreite in Mikrosekunden.
 * @param globalStep [in,out] Aktueller Step (0..63).
 * @param lastBeatTime [in,out] Zeitmarke des letzten Beats.
 * @param ledOffTime [in,out] Zeitpunkt zum Abschalten der Beat-LED.
 * @param lastStepTime [in,out] Zeitmarke des letzten Step-Ticks.
 * @param triggerOffTime [in,out] Zeitpunkt zum Ruecksetzen der Trigger-Ausgaenge.
 * @param triggersActive [in,out] Kennzeichen, ob Trigger aktuell aktiv sind.
 */
void runSequencer(
  unsigned long now,
  int bpm,
  float swingAmount,
  int currentBank,
  const uint8_t probability[8],
  long pulseWidth,
  int& globalStep,
  unsigned long& lastBeatTime,
  unsigned long& ledOffTime,
  unsigned long& lastStepTime,
  unsigned long& triggerOffTime,
  bool& triggersActive
);
