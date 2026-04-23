/*
 * 8-Kanal Drum-Computer (Nano Version) - FINAL MIT BANK-NAMEN
 * Kanal 1 Bassdrum
 * Kanal 2 Low Tom
 * Kanal 3 High Tom
 * Kanal 4 Claves
 * Kanal 5 Sare
 * Kanal 6 Cymbal
 * Kanal 7 Hihat
 * Kanal 8 (nicht benutzen, später Cowbell)
 * ------------------------------------------
 */

/*
Trigger: Pins 2 bis 9 (das sind 8 Pins)
Frei Deine freien Pins sind: A0 und A3. 
A4/A5 sind I2C für das Display, 
A1/A2 sind Potis.
todo: Start/Stop-Button auf Pin A0
*/

#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <EEPROM.h>
#include "core/app_config.h"
#include "core/app_state.h"
#include "data/banks.h"
#include "ui/display.h"
#include "input/menu.h"
#include "core/sequencer.h"

LiquidCrystal_I2C MenuDisplay(0x27, 20, 4);
Encoder MenuEncoder(12, 11);

static AppState makeDefaultAppState() {
  AppState s{};
  s.currentMode = MENU_ROOT;
  s.menuIndex = 0;
  s.needsRedraw = true;

  s.pulseWidth = 10000;
  s.probability[0] = 100;
  s.probability[1] = 82;
  s.probability[2] = 78;
  s.probability[3] = 100;
  s.probability[4] = 92;
  s.probability[5] = 100;
  s.probability[6] = 80;
  s.probability[7] = 100;
  s.lastBank = 0;
  s.currentBank = 0;

  s.lastBtnState = HIGH;
  s.oldEncoderPos = 0;

  s.lastStepTime = 0;
  s.lastPotRead = 0;
  s.lastBeatTime = 0;
  s.ledOffTime = 0;
  s.triggerOffTime = 0;
  s.lastDisplayUpdate = 0;

  s.bpm = 120;
  s.globalStep = 0;
  s.swingAmount = 0.0;
  s.triggersActive = false;
  return s;
}

AppState state = makeDefaultAppState();

void setup() {
  DDRD |= 0b11111100; PORTD |= 0b11111100;
  DDRB |= 0b00000011; PORTB |= 0b00000011;
  pinMode(13, OUTPUT);
  pinMode(selectBtn, INPUT_PULLUP);

  MenuDisplay.init();
  MenuDisplay.backlight();
  randomSeed(analogRead(A2)); 

  // Bank aus EEPROM lesen
  state.currentBank = EEPROM.read(bankAddr);

  // Sicherheit: Wenn der Wert ungültig ist (z.B. bei einem leeren EEPROM steht oft 255 drin),
  // setze ihn auf einen Standardwert (0).
  if (state.currentBank < 0 || state.currentBank >= NUM_BANKS) {
    state.currentBank = 0;
  }  

  Serial.begin(115200);
}

void loop() {
  unsigned long now = micros();

  // 1. Eingaben (Nicht blockierend, da nur bei Flanken aktiv)
  handleMenuSystem(
    MenuEncoder,
    state.currentMode,
    state.menuIndex,
    state.needsRedraw,
    state.oldEncoderPos,
    state.currentBank,
    state.pulseWidth,
    state.lastBtnState
  );

  // 2. Potis (Nur alle 30ms, sehr wichtig!)
  if (millis() - state.lastPotRead > 30) {
    state.lastPotRead = millis();
    state.bpm = (int)map(analogRead(potPinBPM), 0, 1023, 40, 200);
    state.swingAmount = (analogRead(potPinSwing) / 1023.0) * 0.4;
  }

  // 3. SEQUENZER
  runSequencer(
    now,
    state.bpm,
    state.swingAmount,
    state.currentBank,
    state.probability,
    state.pulseWidth,
    state.globalStep,
    state.lastBeatTime,
    state.ledOffTime,
    state.lastStepTime,
    state.triggerOffTime,
    state.triggersActive
  );

  // 4. DISPLAY (Erst ganz am Ende, wenn alles andere erledigt ist)
  updateDisplay(
    MenuDisplay,
    state.currentMode,
    state.menuIndex,
    state.needsRedraw,
    state.lastDisplayUpdate,
    state.bpm,
    state.swingAmount,
    state.pulseWidth,
    state.currentBank,
    state.lastBank
  );

  // static long lastTestPos = 0;
  // long currentTestPos = myEnc.read();
  // if (currentTestPos != lastTestPos) {
  //   Serial.print("Encoder Position: ");
  //   Serial.println(currentTestPos);
  //   lastTestPos = currentTestPos;
  // }
}