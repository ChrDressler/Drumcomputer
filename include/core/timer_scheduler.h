#pragma once

#include <Arduino.h>

// Global scheduler tick (Timer1 CTC) in microseconds.
static const uint16_t TIMER_SCHEDULER_TICK_US = 500;

extern volatile uint32_t gNextStepTick;
extern volatile bool     gNextPinOn[8];  // Welcher Pin soll beim nächsten Step auslösen
extern volatile bool gStepTriggered;
extern volatile uint32_t gStepTicks;

// Per-Channel Pulsbreiten (in Ticks), Dead-Node-Ticks und Off-Timer
extern volatile uint32_t gPinPulseWidthTicks[8];
extern volatile uint32_t gPinDeadNoteTicks[8];  // Dead-Node-Pulsbreite pro Kanal
extern volatile uint32_t gPinOffTick[8];
extern volatile bool     gPinActive[8];

extern volatile uint32_t gNextLedTick;
extern volatile uint32_t gNextLedOnTick;
extern volatile bool gLedState;

// Metronom-Ticks (LED Pin 13): Nutzt den ungeswungenen Step-Wert,
// damit das Metronom unabhaengig vom Pattern-Swing stabil viertelnotig laeuft.
extern volatile uint32_t gMetronomeTicks;

// Open Hi-Hat: Wenn true, bleibt der HH-Pin (Pin 8) nach dem Trigger-Off-Tick LOW,
// d.h. der lange Impuls wird aufrechterhalten bis ein CHH-Step ihn beendet.
extern volatile bool gHiHatLongPulseActive;

// Bit-Maske des HH-Pins im PORTB-Register (Pin 8 = PORTB Bit 0)
static const uint8_t kHiHatMaskB = (1 << (8 - 8));  // = 0x01

/**
 * @brief Initialisiert Timer1 als periodischen Scheduler-Tick.
 */
void timerSchedulerInit();

/**
 * @brief Liefert den aktuellen Scheduler-Tick (atomar gelesen).
 */
uint32_t timerSchedulerNowTicks();

/**
 * @brief Wandelt Mikrosekunden auf Scheduler-Ticks (aufgerundet).
 *
 * INLINE: Wird sehr haeufig aus verschiedenen .cpp-Dateien aufgerufen
 * (mehrere Male pro loop()-Durchlauf). Triviale Division + Addition,
 * der Funktionscall-Overhead waere groesser als der Rechenaufwand.
 */
inline uint32_t timerSchedulerUsToTicks(uint32_t us) {
  return (us + TIMER_SCHEDULER_TICK_US - 1UL) / TIMER_SCHEDULER_TICK_US;
}

/**
 * @brief Wandelt Millisekunden auf Scheduler-Ticks (aufgerundet).
 *
 * INLINE: Wird ebenfalls oft aufgerufen (z. B. Entprell-Timer im Menu).
 * Ruft timerSchedulerUsToTicks() auf, daher lohnt sich das Inlining doppelt:
 * Der Aufruf von UsToTicks wird dann auch noch wegoptimiert.
 */
inline uint32_t timerSchedulerMsToTicks(uint16_t ms) {
  return timerSchedulerUsToTicks((uint32_t)ms * 1000UL);
}

/**
 * @brief Liefert und quittiert das periodische Poti-Update-Flag.
 */
bool timerSchedulerConsumePotDue();

/**
 * @brief Liefert und quittiert das periodische Display-Update-Flag.
 */
bool timerSchedulerConsumeDisplayDue();
