#include "core/midi_timer.h"
#include "core/midi_drum.h"

#include <avr/interrupt.h>

// --- MIDI-Trigger-Timer (8 Kanäle) ---
// Jeder Kanal bekommt einen eigenen Timer (in Mikrosekunden), der den Pin
// nach der berechneten Pulsbreite automatisch wieder ausschaltet.
// Ein Wert von 0 bedeutet: Kanal ist nicht aktiv (kein Timer läuft).
// Läuft im Timer2-ISR mit 100 µs Auflösung, daher unabhängig von loop()-Blockaden.
static volatile uint32_t gMidiTriggerOffTime[ChMax] = {0};
volatile bool gMidiOhhActive = false;  // True solange OHH-Impuls aktiv ist

void midiTimerArm(int channel, uint32_t pulseWidthUs) {
  if (channel < 0 || channel >= ChMax) return;
  midiTriggerOn(channel);
  gMidiTriggerOffTime[channel] = micros() + pulseWidthUs;
}

void midiTimerDisarm(int channel) {
  if (channel < 0 || channel >= ChMax) return;
  midiTriggerOff(channel);
  gMidiTriggerOffTime[channel] = 0;
}

// Timer2-ISR: Läuft alle 100 µs und prüft die MIDI-Trigger-Timer.
// Der int32_t-Cast beim Vergleich behandelt den micros()-Overflow (~70 Min.) korrekt.
ISR(TIMER2_COMPA_vect) {
  const uint32_t now = micros();

  for (int ch = 0; ch < ChMax; ch++) {
    const uint32_t offTime = gMidiTriggerOffTime[ch];
    if (offTime != 0) {
      if (static_cast<int32_t>(now - offTime) >= 0) {
        midiTriggerOff(ch);
        gMidiTriggerOffTime[ch] = 0;
      }
    }
  }
}

void midiTimerInit() {
  cli();

  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;

  // 100 µs bei 16 MHz, Prescaler 8:
  // (100 µs * 16 MHz / 8) - 1 = 199
  OCR2A = 199;

  TCCR2A |= (1 << WGM21);   // CTC-Modus (Clear Timer on Compare Match)
  TCCR2B |= (1 << CS21);    // Prescaler 8 (CS21 = 1, CS20 = 0, CS22 = 0)
  TIMSK2 |= (1 << OCIE2A);  // Interrupt bei Compare Match A aktivieren

  sei();
}
