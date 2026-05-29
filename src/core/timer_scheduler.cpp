#include "core/timer_scheduler.h"

#include <avr/interrupt.h>
#include <util/atomic.h>

volatile uint32_t gNextStepTick = 0xFFFFFFFF;
volatile bool     gNextPinOn[8] = {false};
volatile uint32_t gPinPulseWidthTicks[8] = {0};
volatile uint32_t gPinDeadNoteTicks[8] = {0};
volatile uint32_t gPinOffTick[8] = {0};
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
      gPinActive[ch] = true;
      gPinOffTick[ch] = gSchedulerTicks + gPinPulseWidthTicks[ch];
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
    if (gSchedulerTicks < gPinOffTick[ch]) continue;
    
    // Hi-Hat-Sonderfall: OHH-Impuls (Kanal 6) nicht vorzeitig beenden
    if (ch == 6 && gHiHatLongPulseActive) continue;
    
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
  gStepTicks = timerSchedulerUsToTicks(125000UL);
  gMetronomeTicks = gStepTicks; 
  gNextLedOnTick = gSchedulerTicks + 10;

  // Dead-Node-Ticks initialisieren
  gPinDeadNoteTicks[0] = timerSchedulerUsToTicks(500UL);  // 500us
  gPinDeadNoteTicks[1] = timerSchedulerUsToTicks(1UL);    // LT
  gPinDeadNoteTicks[2] = timerSchedulerUsToTicks(1UL);    // HT
  gPinDeadNoteTicks[3] = timerSchedulerUsToTicks(500UL);
  gPinDeadNoteTicks[4] = timerSchedulerUsToTicks(1UL);    // SN
  gPinDeadNoteTicks[5] = timerSchedulerUsToTicks(500UL);
  gPinDeadNoteTicks[6] = timerSchedulerUsToTicks(100UL);  // CL HH
  gPinDeadNoteTicks[7] = timerSchedulerUsToTicks(500UL);
  
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