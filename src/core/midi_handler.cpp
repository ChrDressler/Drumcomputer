#include "core/midi_handler.h"
#include "core/midi_drum.h"
#include "core/midi_timer.h"

// Referenz auf den globalen AppState (in main.cpp definiert)
extern AppState state;

// Hi-Hat-Kanal (CHH und OHH teilen sich Pin 8 = Kanal 6)
static constexpr int kHiHatChannel = ChHH;

// Velocity-Mapping-Konstanten
static constexpr uint32_t kVelocityMinUs = 1UL;      // 1 µs bei Velocity 1
static constexpr uint32_t kVelocityMaxUs = 2000UL;    // 2 ms bei Velocity 127
static constexpr uint32_t kVelocityRange = kVelocityMaxUs - kVelocityMinUs;

/**
 * @brief Berechnet die Pulsbreite in µs aus der MIDI-Velocity (1..127).
 *        Linearer Map: 100 µs bei Velocity 1, 50000 µs bei Velocity 127.
 */
static inline uint32_t velocityToPulseUs(byte velocity) {
  if (velocity <= 1) {
    return kVelocityMinUs;
  }
  return kVelocityMinUs + (static_cast<uint32_t>(velocity - 1) * kVelocityRange) / 126UL;
}

void midiHandlerInit(MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>>& midiRef) {
  midiRef.setHandleNoteOn(handleNoteOn);
  midiRef.setHandleNoteOff(handleNoteOff);
}

// Die MIDI-Trigger-Timer wurden in midi_timer.cpp ausgelagert und laufen
// jetzt im Timer2-ISR (100 µs Auflösung), unabhängig von loop()-Blockaden.
// Die Funktionen checkMidiTriggerTimers(), midiTriggerOnTimed() und
// midiTriggerOffNow() sind durch midiTimerArm() und midiTimerDisarm() ersetzt.

// Callback: Note On
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  // MIDI-Trigger nur im Stop-Modus (Sequenzer läuft nicht)
  if (state.isRunning) return;

  const int ch = midiNoteToChannel(pitch);
  if (ch < 0) return;  // Nicht gemappte Note ignorieren

  // Pulsbreite aus Velocity berechnen (0-127):
  // - Velocity 1   ->   100 µs (kürzester Impuls)
  // - Velocity 127 -> 50000 µs (50 ms, maximaler Impuls)
  // Für velocity=0 (Note Off) wird hier nie aufgerufen, daher kein /0-Risiko.
  const uint32_t pulseUs = velocityToPulseUs(velocity);

  snprintf(state.MidiMsg, sizeof(state.MidiMsg),
           "ON  %d (%s) %d %d",
           pitch, noteName(pitch), velocity, pulseUs);

  // Nur im MIDI-Monitor-Modus sofort aktualisieren
  if (state.currentMode == MIDI_MONITOR) {
    state.needsRedraw = true;
  }

  if (ch == ChOH) {
    // Open Hi-Hat (Note 46): Pin einschalten, OHH-Flag setzen.
    // KEIN Timer! OHH bleibt aktiv bis CHH (Note 42/44) kommt.
    // Das ist das Standard-GM-Verhalten: OHH wird durch CHH beendet.
    midiTriggerOn(kHiHatChannel);
    gMidiOhhActive = true;
  } else if (ch == kHiHatChannel) {
    // Hi-Hat (Pin 8 = Kanal 6):
    // - Note 42 (Closed Hi-Hat): OHH beenden (falls aktiv), dann CHH mit Timer
    // - Note 44 (Pedal Hi-Hat): OHH beenden (falls aktiv), dann kurzer 2ms-Impuls
    if (gMidiOhhActive) {
      midiTimerDisarm(kHiHatChannel);
      // Soundmodule brauchen eine HIGH-Pause zwischen Off und On.
      // Statt blockierendem delay() wird ein Mini-Timer von 20 µs gesetzt.
      // Der Pin bleibt für ~20 µs HIGH, dann feuert der nächste Trigger.
      delayMicroseconds(20);
      gMidiOhhActive = false;
    }
    if (pitch == 44) {
      // Pedal Hi-Hat: kurzer 2ms-Impuls (2000 µs)
      midiTimerArm(kHiHatChannel, 2000UL);
    } else {
      // Closed Hi-Hat (Note 42): Impuls mit velocity-abhängiger Pulsbreite
      midiTimerArm(kHiHatChannel, pulseUs);
    }
  } else {
    midiTimerArm(ch, pulseUs);
  }
}

// Callback: Note Off
// Ungenutzte Parameter müssen wg. MIDI-Callback-Signatur drin bleiben.
void handleNoteOff(byte channel, byte pitch, byte /*velocity*/) {
  snprintf(state.MidiMsg, sizeof(state.MidiMsg),
           "OFF %d",
           pitch);

  // Nur im MIDI-Monitor-Modus sofort aktualisieren
  if (state.currentMode == MIDI_MONITOR) {
    state.needsRedraw = true;
  }

  // MIDI-Trigger werden nicht mehr durch Note Off gesteuert!
  // Die Pulsbreite wird durch den Timer in handleNoteOn bestimmt.
  // Note Off dient nur noch dem MIDI-Monitor.
}

// Hilfsfunktion: MIDI-Note (0-127) in Notennamen mit Oktave umwandeln
const char* noteName(byte note) {
  static const char* const notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  static char buffer[8];  // Ausreichend für z.B. "C#10\0" (max. 7 Zeichen + Nullterminator)
  const int octave = (note / 12) - 1;
  const int noteIndex = note % 12;
  snprintf(buffer, sizeof(buffer), "%s%d", notes[noteIndex], octave);
  return buffer;
}
