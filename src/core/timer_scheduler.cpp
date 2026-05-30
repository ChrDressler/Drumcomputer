#include "core/timer_scheduler.h"
#include "core/app_config.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

volatile uint32_t gNextStepTick = 0xFFFFFFFF;
volatile bool     gNextPinOn[8] = {false};
volatile uint32_t gPwTicks[8] = {0};
volatile uint32_t gDeadNoteTicks[8] = {0};
volatile uint32_t gNoteOffTick[8] = {0};
volatile bool     gPinActive[8] = {false};
volatile bool gStepTriggered = false;
volatile bool gHiHatLongPulseActive = false;
volatile uint32_t gStepTicks = 2000;

// Metronom-Ticks (separat von gStepTicks!): Immer straight (ungeswungen),
// damit die LED unabhaengig vom Pattern-Swing exakt im Viertel-Takt blinkt.
volatile uint32_t gMetronomeTicks = 2000;

volatile bool gPotDue = false;
volatile bool gDisplayDue = false;
volatile uint16_t gPotDivider = 0;
volatile uint16_t gDisplayDivider = 0;

const uint16_t kPotPeriodTicks = 60;      // 30 ms @ 500 us tick
const uint16_t kDisplayPeriodTicks = 200; // 100 ms @ 500 us tick

volatile uint32_t gNextLedTick = 0;
volatile bool gLedState = false;
const uint32_t kLedFlashDurationTicks = 70; // Kurzer Blitz (ca. 5ms bei 500us Tick)
volatile uint32_t gNextLedOnTick = 10; // Startet fast sofort nach dem Booten
static uint32_t gNextLedOffTick = 0xFFFFFFFF;

// Interne Variablen für die ISR
namespace {
    volatile uint32_t gSchedulerTicks = 0;
}

// Hilfstabelle: Pins für Kanal 0..7 (aus app_config.h trigPins[8])
static const uint8_t kChannelPin[8] = {2, 3, 4, 5, 6, 7, 8, 9};

ISR(TIMER1_COMPA_vect) {
  gSchedulerTicks++;

  // --- AUTARKES METRONOM (LED_BUILTIN = Pin 13) ---
  // Nutzt gMetronomeTicks (immer straight), NICHT gStepTicks!
  // Dadurch bleibt die LED auch bei Achtel- oder 16tel-Swing stabil.
  if (gSchedulerTicks >= gNextLedOnTick) {
      digitalWrite(LED_BUILTIN, HIGH);
      gNextLedOffTick = gSchedulerTicks + kLedFlashDurationTicks;
      gNextLedOnTick = gSchedulerTicks + (gMetronomeTicks * 4);
  }

  if (gSchedulerTicks >= gNextLedOffTick) {
      digitalWrite(LED_BUILTIN, LOW);
      gNextLedOffTick = 0xFFFFFFFF;
  }

  // 1. SOUND AUSLÖSEN (Nutzt gStepTicks mit Swing)
  // Baut dynamisch die Portmasken aus gNextPinOn[] und schaltet alle gleichzeitig
  if (gSchedulerTicks >= gNextStepTick) {
    uint8_t dynMaskD = 0;
    uint8_t dynMaskB = 0;

    for (int ch = 0; ch < 8; ch++) {
      if (!gNextPinOn[ch]) continue;

      uint8_t pin = kChannelPin[ch];
      if (pin <= 7) {
        dynMaskD |= (1 << pin);
      } else {
        dynMaskB |= (1 << (pin - 8));
      }

      // Off-Tick für diesen Kanal setzen
      // gPwTicks[ch] wurde von sequencer.cpp bereits gesetzt
      gPinActive[ch] = true;
      gNoteOffTick[ch] = gSchedulerTicks + gPwTicks[ch];
    }

    // Alle Pins gleichzeitig auf LOW – parallele Ausgabe wie gewünscht
    PORTD &= ~dynMaskD;
    PORTB &= ~dynMaskB;
    
    gNextStepTick += gStepTicks; 

    gStepTriggered = true;
  }

  // 2. TRIGGER AUSSCHALTEN (pro Kanal einzeln)
  for (int ch = 0; ch < 8; ch++) {
    if (!gPinActive[ch]) continue;
    if (gSchedulerTicks < gNoteOffTick[ch]) continue;
    
    // Hi-Hat-Sonderfall: OHH-Impuls (Kanal ChHH) nicht vorzeitig beenden
    if (ch == ChHH && gHiHatLongPulseActive) continue;
    
    uint8_t pin = kChannelPin[ch];
    if (pin <= 7) {
      PORTD |= (1 << pin);
    } else {
      PORTB |= (1 << (pin - 8));
    }
    gPinActive[ch] = false;
  }
  
  gPotDivider++;
  if (gPotDivider >= kPotPeriodTicks) {
    gPotDivider = 0;
    gPotDue = true;
  }

  gDisplayDivider++;
  if (gDisplayDivider >= kDisplayPeriodTicks) {
    gDisplayDivider = 0;
    gDisplayDue = true;
  }
}

void timerSchedulerInit() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  const uint32_t ticksPerSecond = 1000000UL / TIMER_SCHEDULER_TICK_US;
  const uint16_t compareMatch = (uint16_t)((F_CPU / 8UL / ticksPerSecond) - 1UL);
  OCR1A = compareMatch;

  // CTC mode, prescaler 8
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11);

  TIMSK1 |= (1 << OCIE1A);

  // Standardwert: 120 BPM entsprechen ca. 125ms pro Step
  gStepTicks = 250;  // 125000us / 500us = 250 Ticks
  gMetronomeTicks = gStepTicks; 
  gNextLedOnTick = gSchedulerTicks + 10;

  // Dead-Node-Ticks initialisieren (in Ticks à 500us)
  gDeadNoteTicks[ChBD] = 1;
  gDeadNoteTicks[ChLT] = 1;
  gDeadNoteTicks[ChHT] = 1;
  gDeadNoteTicks[ChCL] = 1;
  gDeadNoteTicks[ChSN] = 1;
  gDeadNoteTicks[ChCY] = 1;
  gDeadNoteTicks[ChHH] = 1;
  gDeadNoteTicks[ChCB] = 2;
  
  sei(); // Set Enable Interrupts
}

uint32_t timerSchedulerNowTicks() {
  uint32_t value = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    value = gSchedulerTicks;
  }
  return value;
}

bool timerSchedulerConsumePotDue() {
  bool due = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    due = gPotDue;
    gPotDue = false;
  }
  return due;
}

bool timerSchedulerConsumeDisplayDue() {
  bool due = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    due = gDisplayDue;
    gDisplayDue = false;
  }
  return due;
}
