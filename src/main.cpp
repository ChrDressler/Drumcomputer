/*
 * 8-Kanal Drum-Computer (Arduino Nano Version) 
 
D2 ... D9 Trigger8 mal Trigger, Kanäle siehe Banks.cpp
D11, D12 Rotary Encoder für Menü-Navigation
D10 Select-Button für Menü-Auswahl
D13 Onboard-LED als Metronom-Visualisierung (blinkt im Takt
A0 Start/Stop
A3 Frei 
A4/A5 I2C für das Display, 
A1/A2 Potis.
*/

#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <EEPROM.h>
#include <MIDI.h>
#include "core/app_config.h"
#include "core/app_state.h"
#include "data/banks.h"
#include "ui/display.h"
#include "input/menu.h"
#include "core/sequencer.h"
#include "core/timer_scheduler.h"
#include "core/midi_drum.h"

LiquidCrystal_I2C MenuDisplay(0x27, 20, 4);
Encoder MenuEncoder(12, 11);

// MIDI auf HardwareSerial (Pin 0 = RX, Pin 1 = TX)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);

// Forward declaration der Callback-Funktionen
void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
const char* noteName(byte note);
static void checkMidiTriggerTimers();
static void midiTriggerOnTimed(int channel, uint32_t pulseWidthUs);
static void midiTriggerOffNow(int channel);

static AppState makeDefaultAppState() {
  AppState s{};
  s.currentMode = MENU_ROOT;
  s.menuIndex = 0;
  s.needsRedraw = true;

  s.pulseWidth = 10000;
  s.probability[0] = 100;
  s.probability[1] = 100;
  s.probability[2] = 100;
  s.probability[3] = 100;
  s.probability[4] = 100;
  s.probability[5] = 100;
  s.probability[6] = 100;
  s.probability[7] = 100;
  s.lastBank = 0;
  s.currentBank = 0;

  s.lastBtnState = HIGH;
  s.oldEncoderPos = 0;
  s.menuDebounceUntilTick = 0;
  s.startStopDebounceUntilTick = 0;

  s.isRunning = false;
  s.lastStartStopBtnState = HIGH;  
  s.globalStep = 0; 
  s.bpm = 120;
  s.swingAmount = 0.0;
  return s;
}

// Definition eines Pause-Symbols (zwei Balken)
byte pauseChar[8] = {
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b11011,
  0b00000
};

// Ein gefülltes Play-Dreieck
byte playChar[8] = {
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11100,
  0b11000,
  0b10000,
  0b00000
};

AppState state = makeDefaultAppState();

void setup() {
  // Pin-Initialisierung
  DDRD |= 0b11111100; PORTD |= 0b11111100;
  DDRB |= 0b00000011; PORTB |= 0b00000011;
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(selectBtn, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);

  MenuDisplay.init();
  MenuDisplay.backlight();
  MenuDisplay.createChar(0, pauseChar);
  MenuDisplay.createChar(1, playChar);
  randomSeed(analogRead(A2)); 

  timerSchedulerInit();

  // Bank aus EEPROM lesen
  state.currentBank = EEPROM.read(bankAddr);
  if (state.currentBank < 0 || state.currentBank >= NUM_BANKS) {
    state.currentBank = 0;
  }

  // Pulsbreite aus EEPROM lesen
  long storedPulseWidth;
  EEPROM.get(pulseWidthAddr, storedPulseWidth);
  if (storedPulseWidth >= 1000 && storedPulseWidth <= 500000) {
    state.pulseWidth = storedPulseWidth;
  }

  Serial.begin(31250);
   // MIDI-Callbacks registrieren
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  // MIDI initialisieren
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop() {
  uint32_t nowTicks = timerSchedulerNowTicks();
  bool potDue = timerSchedulerConsumePotDue();
  bool displayDue = timerSchedulerConsumeDisplayDue();

  handleStartStop(nowTicks, state.isRunning, state.lastStartStopBtnState, 
                  state.globalStep, state.needsRedraw, state.startStopDebounceUntilTick);
  
  handleMenuSystem(MenuEncoder, nowTicks, state.currentMode, state.menuIndex,
                   state.needsRedraw, state.oldEncoderPos, state.currentBank,
                   state.pulseWidth, state.lastBtnState, state.menuDebounceUntilTick);

  if (potDue) {
    state.bpm = (int)map(analogRead(potPinBPM), 0, 1023, 40, 200);
    state.swingAmount = (analogRead(potPinSwing) / 1023.0) * 0.4;

    // gStepTicks auch im Stop-Zustand setzen (für LED-Metronom + initiales Timing).
    // Waehrend des Abspielens setzt runSequencer() beide Werte neu.
    if (!state.isRunning) {
      gStepTicks = computeStepTicks(state.bpm, state.swingAmount, state.globalStep);
    }
  }

  // Eingehende MIDI-Nachrichten verarbeiten (ruft handleNoteOn/handleNoteOff auf)
  MIDI.read();

  // MIDI-Trigger-Timer prüfen: Schaltet Pins nach state.pulseWidth automatisch aus
  checkMidiTriggerTimers();

  runSequencer(
    state.bpm,
    state.swingAmount,
    state.currentBank,
    state.probability,
    state.pulseWidth,
    state.globalStep,
    state.isRunning
  );

  updateDisplay(MenuDisplay, state.currentMode, state.menuIndex, state.needsRedraw,
                displayDue, state.bpm, state.swingAmount, state.pulseWidth,
                state.currentBank, state.lastBank, state.isRunning,
                state.MidiMsg);                
}

// --- MIDI-Trigger-Timer (8 Kanäle + OHH) ---
// Jeder Kanal bekommt einen eigenen Timer (in Mikrosekunden), der den Pin
// nach state.pulseWidth automatisch wieder ausschaltet.
// Ein Wert von 0 bedeutet: Kanal ist nicht aktiv (kein Timer läuft).
static uint32_t gMidiTriggerOffTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static bool gMidiOhhActive = false;  // True solange OHH-Impuls aktiv ist

/**
 * @brief Prüft, ob MIDI-Trigger-Timer abgelaufen sind, und schaltet die Pins aus.
 * Wird in loop() aufgerufen.
 */
static void checkMidiTriggerTimers() {
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

/**
 * @brief MIDI-Trigger einschalten mit automatischem Timer-Ausschalten.
 * @param channel Kanalindex (0..7).
 * @param pulseWidthUs Pulsbreite in Mikrosekunden.
 */
static void midiTriggerOnTimed(int channel, uint32_t pulseWidthUs) {
  if (channel < 0 || channel > 7) return;
  midiTriggerOn(channel);
  gMidiTriggerOffTime[channel] = micros() + pulseWidthUs;
}

/**
 * @brief MIDI-Trigger sofort ausschalten und Timer löschen.
 * @param channel Kanalindex (0..7).
 */
static void midiTriggerOffNow(int channel) {
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
    // - Note 44 (Pedal Hi-Hat): OHH beenden (falls aktiv), dann kurzer 5ms-Impuls
    if (gMidiOhhActive) {
      midiTriggerOffNow(6);
      delay(20); // Sonst triggert dei Closed HH nicht richtig
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