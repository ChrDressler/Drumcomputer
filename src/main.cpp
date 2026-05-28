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
#include "core/midi_handler.h"

LiquidCrystal_I2C MenuDisplay(0x27, 20, 4);
Encoder MenuEncoder(12, 11);

// MIDI auf HardwareSerial (Pin 0 = RX, Pin 1 = TX)
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);

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

  // MIDI-Callbacks registrieren (ausgelagert in midi_handler)
  midiHandlerInit(MIDI);

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
