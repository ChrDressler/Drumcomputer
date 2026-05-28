#include "core/midi_handler.h"
#include "core/midi_drum.h"

// Referenz auf den globalen AppState (in main.cpp definiert)
extern AppState state;

// --- MIDI-Trigger-Timer (8 Kanäle + OHH) ---
// Jeder Kanal bekommt einen eigenen Timer (in Mikrosekunden), der den Pin
// nach state.pulseWidth automatisch wieder ausschaltet.
// Ein Wert von 0 bedeutet: Kanal ist nicht aktiv (kein Timer läuft).
static uint32_t gMidiTriggerOffTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static bool gMidiOhhActive = false;  // True solange OHH-Impuls aktiv ist

void midiHandlerInit(MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>>& midiRef) {
  midiRef.setHandleNoteOn(handleNoteOn);
  midiRef.setHandleNoteOff(handleNoteOff);
}

void checkMidiTriggerTimers() {
  uint32_t now = micros();
  
  for (int ch = 0; ch < 8; ch++) {
    if (gMidiTriggerOffTime[ch] != 0) {
      // Timer abgelaufen?
      if ((int32_t)(now - gMidiTriggerOffTime[ch]) >= 0) {
        midiTriggerOff(ch);
        gMidiTriggerOffTime[ch] = 0;
      }
    }
  }
}

void midiTriggerOnTimed(int channel, uint32_t pulseWidthUs) {
  if (channel < 0 || channel > 7) return;
  midiTriggerOn(channel);
  gMidiTriggerOffTime[channel] = micros() + pulseWidthUs;
}

void midiTriggerOffNow(int channel) {
  if (channel < 0 || channel > 7) return;
  midiTriggerOff(channel);
  gMidiTriggerOffTime[channel] = 0;
}

// Callback: Note On
void handleNoteOn(byte channel, byte pitch, byte velocity) {
  snprintf(state.MidiMsg, sizeof(state.MidiMsg),
           "ON  %d (%s)",
           pitch, noteName(pitch));

  // Nur im MIDI-Monitor-Modus sofort aktualisieren
  if (state.currentMode == MIDI_MONITOR) {
    state.needsRedraw = true;
  }

  // MIDI-Trigger nur im Stop-Modus (Sequenzer läuft nicht)
  if (state.isRunning) return;

  int ch = midiNoteToChannel(pitch);
  if (ch < 0) return;  // Nicht gemappte Note ignorieren

  uint32_t pulseUs = (uint32_t)state.pulseWidth;

  if (ch == 8) {
    // Open Hi-Hat (Note 46): Pin einschalten, OHH-Flag setzen.
    // KEIN Timer! OHH bleibt aktiv bis CHH (Note 42/44) kommt.
    // Das ist das Standard-GM-Verhalten: OHH wird durch CHH beendet.
    midiTriggerOn(6);
    gMidiOhhActive = true;
  } else if (ch == 6) {
    // Hi-Hat (Pin 8 = Kanal 6):
    // - Note 42 (Closed Hi-Hat): OHH beenden (falls aktiv), dann CHH mit Timer
    // - Note 44 (Pedal Hi-Hat): OHH beenden (falls aktiv), dann kurzer 2ms-Impuls
    if (gMidiOhhActive) {
      midiTriggerOffNow(6);
      delay(20); // Soundmodule brauchen eine HIGH-Pause zwischen Off und On
      gMidiOhhActive = false;
    }
    if (pitch == 44) {
      // Pedal Hi-Hat: kurzer 2ms-Impuls
      midiTriggerOnTimed(6, 2000UL);
    } else {
      // Closed Hi-Hat (Note 42): Impuls mit state.pulseWidth
      midiTriggerOnTimed(6, pulseUs);
    }
  } else {
    midiTriggerOnTimed(ch, pulseUs);
  }
}

// Callback: Note Off
void handleNoteOff(byte channel, byte pitch, byte velocity) {
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
  (void)channel;
  (void)pitch;
  (void)velocity;
}

// Hilfsfunktion: MIDI-Note (0-127) in Notennamen mit Oktave umwandeln
const char* noteName(byte note) {
  const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  static char buffer[12];
  int octave = (note / 12) - 1;
  int noteIndex = note % 12;
  sprintf(buffer, "%s%d", notes[noteIndex], octave);
  return buffer;
}
