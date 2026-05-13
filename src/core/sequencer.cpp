#include "core/sequencer.h"
#include "core/app_config.h"
#include "core/timer_scheduler.h"
#include "data/banks.h"

void sequencerStart(int& globalStep) {
  globalStep = 0;

  uint32_t startAnchor = timerSchedulerNowTicks() + 5;
  gNextStepTick = startAnchor; // "Reset" für den Sequenzer
  gStepTriggered = true; // Zwingt den Sequenzer zur ersten Berechnung

  // LED-SYNC: Der Blitz soll EXAKT gleichzeitig mit Step 0 passieren
  gNextLedOnTick = startAnchor;

  // LED-STATUS: Sicherstellen, dass die LED aus ist, bevor der Blitz kommt
  digitalWrite(13, LOW);
}

uint32_t computeStepTicks(int bpm, float swingAmount, int globalStep) {
    uint32_t quarterUs = 60000000UL / (uint32_t)bpm;
    uint32_t baseStepUs = quarterUs / 4UL;
    uint32_t swingOffsetUs = (uint32_t)(baseStepUs * swingAmount);

    bool isSecondStep = (globalStep % 2) == 1;
    
    uint32_t stepUs = isSecondStep 
      ? (baseStepUs + swingOffsetUs) 
      : (baseStepUs - swingOffsetUs);
    
      return timerSchedulerUsToTicks(stepUs);
}

void runSequencer(
  int bpm,
  float swingAmount,
  int currentBank,
  const uint8_t probability[8],
  long pulseWidth,
  int& globalStep,
  bool isRunning
) {
  if (!isRunning) {
    gNextStepTick = 0xFFFFFFFF;
    return;
  }

  if (gStepTriggered) {
    gStepTriggered = false;

    uint8_t stepsPerBar = getBankStepsPerBar((uint8_t)currentBank);
    int totalSteps = stepsPerBar * 4;     // 64 für 4/4, 48 für 3/4

    // --- 1. MASKIERUNG FÜR DEN AKTUELLEN STEP ---
    uint8_t d_mask = 0;
    uint8_t b_mask = 0;
    int currentBar = globalStep / stepsPerBar;
    int localStep = globalStep % stepsPerBar;

    // CHH (Kanal 6, Index 6) und OHH (Index 8) vorab prüfen für HH-Logik
    uint16_t chhWord = getBankPatternWord((uint8_t)currentBank, 6, (uint8_t)currentBar);
    uint16_t ohhWord = getBankPatternWord((uint8_t)currentBank, 8, (uint8_t)currentBar);
    bool chhHit = ((chhWord >> (15 - localStep)) & 1) && (random(0, 101) <= probability[6]);
    bool ohhHit = ((ohhWord >> (15 - localStep)) & 1) && (random(0, 101) <= probability[6]);

    // Open HH-Logik:
    // - OHH-Step: langen Impuls starten (Flag setzen, Pin in Maske aufnehmen)
    // - CHH-Step: kurzen Impuls ausgeben UND langen OHH-Impuls beenden (Flag loeschen)
    // - Kein HH-Step: OHH-Zustand unveraendert lassen
    if (ohhHit) {
      // Langen Impuls starten: HH-Pin in Maske aufnehmen, Flag setzen
      b_mask |= kHiHatMaskB;
      gHiHatLongPulseActive = true;
    } else if (chhHit) {
      // CHH beendet OHH: kurzer Impuls, Flag loeschen
      b_mask |= kHiHatMaskB;
      gHiHatLongPulseActive = false;
    }
    // Wenn weder CHH noch OHH: HH-Pin nicht in Maske – OHH-Zustand bleibt erhalten

    for (int ch = 0; ch < 8; ch++) {
      if (ch == 6) continue; // HH bereits oben behandelt
      uint16_t patternWord = getBankPatternWord((uint8_t)currentBank, (uint8_t)ch, (uint8_t)currentBar);
      if ((patternWord >> (15 - localStep)) & 1) {
        if (random(0, 101) <= probability[ch]) {
          int pin = trigPins[ch];
          if (pin <= 7) d_mask |= (1 << pin);
          else b_mask |= (1 << (pin - 8));
        }
      }
    }

    gNextMaskD = d_mask;
    gNextMaskB = b_mask;
    gPulseWidthTicks = timerSchedulerUsToTicks((uint32_t)pulseWidth);

    // --- 2. VORBEREITUNG FÜR DEN NÄCHSTEN SCHLAG ---
    globalStep = (globalStep + 1) % totalSteps;

    // gStepTicks: mit 16tel-Swing für die Trigger-Timings
    gStepTicks = computeStepTicks(bpm, swingAmount, globalStep);

    // gMetronomeTicks: immer straight für die LED
    uint32_t quarterUs = 60000000UL / (uint32_t)bpm;
    uint32_t baseStepUs = quarterUs / 4UL;
    gMetronomeTicks = timerSchedulerUsToTicks(baseStepUs);
  }
}
