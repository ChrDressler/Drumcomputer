#include "input/menu.h"

#include <EEPROM.h>

#include "data/banks.h"

void handleMenuSystem(
  Encoder& encoder,
  MenuMode& currentMode,
  int& menuIndex,
  bool& needsRedraw,
  long& oldEncoderPos,
  int& currentBank,
  long& pulseWidth,
  bool& lastBtnState
) {
  bool btnState = digitalRead(selectBtn);

  if (btnState == LOW && lastBtnState == HIGH) {
    if (currentMode == MENU_ROOT) {
      if (menuIndex == 0) currentMode = PLAY;
      else if (menuIndex == 1) currentMode = EDIT_BANK;
      else if (menuIndex == 2) currentMode = EDIT_PULSE;
      else if (menuIndex == 3) currentMode = INFO;
      needsRedraw = true;
      oldEncoderPos = encoder.read();
    }
    else if (currentMode == EDIT_BANK) {
      EEPROM.update(bankAddr, currentBank);

      currentMode = MENU_ROOT;
      oldEncoderPos = encoder.read();
    }
    else {
      currentMode = MENU_ROOT;
      oldEncoderPos = encoder.read();
    }
    needsRedraw = true;
    delay(50);
  }
  lastBtnState = btnState;

  long newPos = encoder.read();

  if (abs(newPos - oldEncoderPos) >= ENCODER_STEPS) {
    int diff = (newPos - oldEncoderPos) / ENCODER_STEPS;

    if (diff != 0) {
      if (currentMode == MENU_ROOT) {
        menuIndex = constrain(menuIndex + diff, 0, 3);
        needsRedraw = true;
      }
      else if (currentMode == EDIT_BANK) {
        currentBank = constrain(currentBank + diff, 0, NUM_BANKS - 1);
        needsRedraw = true;
      }
      else if (currentMode == EDIT_PULSE) {
        long lastPulseWidth = pulseWidth;
        pulseWidth = constrain(pulseWidth - diff * 1000, 1000, 500000);
        if (pulseWidth != lastPulseWidth)
        {
          needsRedraw = true;
        }
      }

      oldEncoderPos = newPos;
    }
  }
}
