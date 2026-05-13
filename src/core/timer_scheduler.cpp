#include "core/timer_scheduler.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

volatile uint32_t gNextStepTick = 0xFFFFFFFF; 
volatile uint8_t gNextMaskD = 0;
volatile uint8_t gNextMaskB = 0;
volatile uint32_t gPulseWidthTicks = 20;
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
    volatile uint32_t gTriggerOffTick = 0;
    volatile bool gTriggersActive = false;
}

ISR(TIMER1_COMPA_vect) {
  gSchedulerTicks++;

  // --- AUTARKES METRONOM (LED PIN 13) ---
  // Nutzt gMetronomeTicks (immer straight), NICHT gStepTicks!
  // Dadurch bleibt die LED auch bei Achtel- oder 16tel-Swing stabil.
  if (gSchedulerTicks >= gNextLedOnTick) {
      digitalWrite(13, HIGH);
      gNextLedOffTick = gSchedulerTicks + kLedFlashDurationTicks;
      gNextLedOnTick = gSchedulerTicks + (gMetronomeTicks * 4);
  }

  if (gSchedulerTicks >= gNextLedOffTick) {
      digitalWrite(13, LOW);
      gNextLedOffTick = 0xFFFFFFFF;
  }

  // 1. SOUND AUSLÖSEN (Nutzt gStepTicks mit Swing)
  if (gSchedulerTicks >= gNextStepTick) {
    PORTD &= ~gNextMaskD;
    PORTB &= ~gNextMaskB;
    
    gNextStepTick += gStepTicks; 

    gTriggerOffTick = gSchedulerTicks + gPulseWidthTicks;
    gTriggersActive = true;

    gStepTriggered = true;
  }

  // 2. TRIGGER AUSSCHALTEN
  if (gTriggersActive && gSchedulerTicks >= gTriggerOffTick) {
      PORTD |= 0b11111100;
      PORTB |= 0b00000011;
      // Open HH: HH-Pin (Pin 8 = PORTB Bit 0) LOW halten, solange OHH aktiv ist.
      // Der lange Impuls bleibt bestehen bis ein CHH-Step das Flag loescht.
      if (gHiHatLongPulseActive) {
          PORTB &= ~kHiHatMaskB;
      }
      gTriggersActive = false;
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
  gStepTicks = timerSchedulerUsToTicks(125000UL);
  gMetronomeTicks = gStepTicks; 
  gNextLedOnTick = gSchedulerTicks + 10;
  
  sei();
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