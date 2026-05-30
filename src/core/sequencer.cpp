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

    // --- 1. AKTIVE KANÄLE FÜR DEN AKTUELLEN STEP BESTIMMEN ---
    // gNextPinOn[] wird pro Kanal gesetzt (true = Pin soll auslösen)
    for (int ch = 0; ch < 8; ch++) {
      gNextPinOn[ch] = false;
    }

    int currentBar = globalStep / stepsPerBar;
    int localStep = globalStep % stepsPerBar;

    // CHH (Kanal 6) und OHH (Index 8) vorab prüfen für HH-Logik
    // Pattern-Word: Bits 0-15 = Hit-Maske, Bits 16-31 = Dead-Node-Maske
    uint32_t chhWord = getBankPatternWord((uint8_t)currentBank, 6, (uint8_t)currentBar);
    uint32_t ohhWord = getBankPatternWord((uint8_t)currentBank, 8, (uint8_t)currentBar);
    bool chhHit = ((chhWord >> (15 - localStep)) & 1) && (random(0, 101) <= probability[6]);
    bool ohhHit = ((ohhWord >> (15 - localStep)) & 1) && (random(0, 101) <= probability[6]);

    // Open HH-Logik:
    // - OHH-Step: langen Impuls starten (Flag setzen, Kanal 6 aktivieren)
    // - CHH-Step: kurzen Impuls ausgeben UND langen OHH-Impuls beenden (Flag loeschen)
    // - Kein HH-Step: OHH-Zustand unveraendert lassen
    if (ohhHit) {
      gNextPinOn[6] = true;
      gHiHatLongPulseActive = true;
    } else if (chhHit) {
      gNextPinOn[6] = true;
      gHiHatLongPulseActive = false;
    }
    // Wenn weder CHH noch OHH: Kanal 6 nicht aktiv – OHH-Zustand bleibt erhalten

    for (int ch = 0; ch < 8; ch++) {
      if (ch == 6) continue; // HH bereits oben behandelt
      uint32_t patternWord = getBankPatternWord((uint8_t)currentBank, (uint8_t)ch, (uint8_t)currentBar);
      // Bits 0-15 = Hit-Maske
      if ((patternWord >> (15 - localStep)) & 1) {
        if (random(0, 101) <= probability[ch]) {
          gNextPinOn[ch] = true;
        }
      }
    }

    // --- 2. PULSBREITEN PRO KANAL SETZEN ---
    // gPwTicks[ch] wird für jeden aktiven Kanal gesetzt.
    // Die ISR addiert dann gPwTicks[ch] zum aktuellen Scheduler-Tick,
    // um den absoluten Off-Tick zu berechnen.
    //
    // Dead-Node-Maske ist in Bits 16-31 jedes Kanal-Pattern-Worts
    // Jeder Kanal kann einzeln ein O haben und bekommt dann gDeadNoteTicks[ch]
    uint32_t pwDefaultTicks = timerSchedulerUsToTicks((uint32_t)pulseWidth);
    uint32_t pwChhTicks = 4;  // 2000us / 500us = 4 Ticks

    for (int ch = 0; ch < 8; ch++) {
      if (!gNextPinOn[ch]) {
        gPwTicks[ch] = 0;
        continue;
      }

      if (ch == 6 && gHiHatLongPulseActive) {
        // OHH: Pulsbreite = Standard (wird aber von der ISR ignoriert,
        // weil gHiHatLongPulseActive=true → kein Off-Timer)
        gPwTicks[6] = pwDefaultTicks;
      } else if (ch == 6) {
        // CHH-Sonderfall: Dead Note aus dem eigenen Pattern-Wort
        uint32_t chhWordLocal = getBankPatternWord((uint8_t)currentBank, 6, (uint8_t)currentBar);
        bool chhDead = (chhWordLocal >> (31 - localStep)) & 1;  // Bits 16-31
        gPwTicks[6] = chhDead ? gDeadNoteTicks[6] : pwChhTicks;
      } else {
        uint32_t patWord = getBankPatternWord((uint8_t)currentBank, (uint8_t)ch, (uint8_t)currentBar);
        bool isDead = (patWord >> (31 - localStep)) & 1;  // Bits 16-31 = Dead-Node-Maske
        gPwTicks[ch] = isDead ? gDeadNoteTicks[ch] : pwDefaultTicks;
      }
    }

    // --- 3. VORBEREITUNG FÜR DEN NÄCHSTEN SCHLAG ---
    globalStep = (globalStep + 1) % totalSteps;

    // gStepTicks: mit 16tel-Swing für die Trigger-Timings
    gStepTicks = computeStepTicks(bpm, swingAmount, globalStep);

    // gMetronomeTicks: immer straight für die LED
    uint32_t quarterUs = 60000000UL / (uint32_t)bpm;
    uint32_t baseStepUs = quarterUs / 4UL;
    gMetronomeTicks = timerSchedulerUsToTicks(baseStepUs);
  }
}
