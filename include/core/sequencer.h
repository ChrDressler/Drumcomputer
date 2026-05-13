#pragma once

#include <Arduino.h>

/**
 * @brief Initialisiert den Sequenzer fuer den Start (Reset globalStep, setzt Timer-Anker).
 * @param globalStep [out] Wird auf 0 zurueckgesetzt.
 */
void sequencerStart(int& globalStep);

/**
 * @brief Fuehrt einen Sequencer-Tick aus (Timing, Trigger setzen und ruecksetzen).
 * @param nowTicks Aktueller Scheduler-Tick.
 * @param bpm Aktuelles Tempo in BPM.
 * @param swingAmount Swing-Anteil als Faktor (z. B. 0.0 bis 0.4).
 * @param currentBank Index der aktuell aktiven Pattern-Bank.
 * @param probability Trigger-Wahrscheinlichkeit pro Kanal in Prozent.
 * @param pulseWidth Trigger-Pulsbreite in Mikrosekunden.
 * @param globalStep [in,out] Aktueller Step (0..63).
 * @param nextStepTick [in,out] Geplanter Tick des naechsten Steps.
 * @param nextIdleBeatTick [in,out] Geplanter Tick fuer LED-Blink im Stop-Modus.
 * @param ledOffTick [in,out] Zeitpunkt zum Abschalten der Beat-LED.
 * @param triggerOffTick [in,out] Zeitpunkt zum Ruecksetzen der Trigger-Ausgaenge.
 * @param triggersActive [in,out] Kennzeichen, ob Trigger aktuell aktiv sind.
 */
void runSequencer(
  int bpm,
  float swingAmount,
  int currentBank,
  const uint8_t probability[8],
  long pulseWidth,
  int& globalStep,
  bool isRunning
);

uint32_t computeStepTicks(int bpm, float swingAmount, int globalStep);
