#include "input/menu.h"
#include "input/debounce.h"

#include <EEPROM.h>

#include "core/sequencer.h"
#include "core/timer_scheduler.h"
#include "data/banks.h"

// Entprell-Instanzen für die beiden Taster
static DebounceButton g_selectBtn(selectBtn);
static DebounceButton g_startStopBtn(A0);

void handleMenuSystem(
  Encoder& encoder,
  uint32_t nowTicks,
  MenuMode& currentMode,
  int& menuIndex,
  bool& needsRedraw,
  long& oldEncoderPos,
  int& currentBank,
  long& pulseWidth
) {
  if (g_selectBtn.update(nowTicks)) {
    if (currentMode == MENU_ROOT) {
      if (menuIndex == 0) currentMode = PLAY;
      else if (menuIndex == 1) currentMode = EDIT_BANK;
      else if (menuIndex == 2) currentMode = EDIT_PULSE;
      else if (menuIndex == 3) currentMode = INFO;
      else if (menuIndex == 4) currentMode = MIDI_MONITOR;
      needsRedraw = true;
      oldEncoderPos = encoder.read();
    }
    else if (currentMode == EDIT_BANK) {
      EEPROM.update(bankAddr, currentBank);

      currentMode = MENU_ROOT;
      oldEncoderPos = encoder.read();
    }
    else if (currentMode == EDIT_PULSE) {
      EEPROM.put(pulseWidthAddr, pulseWidth);

      currentMode = MENU_ROOT;
      oldEncoderPos = encoder.read();
    }
    else {
      currentMode = MENU_ROOT;
      oldEncoderPos = encoder.read();
    }
    needsRedraw = true;
  }

  long newPos = encoder.read();

  if (abs(newPos - oldEncoderPos) >= ENCODER_STEPS) {
    int diff = (newPos - oldEncoderPos) / ENCODER_STEPS;

    if (diff != 0) {
      if (currentMode == MENU_ROOT) {
        menuIndex = constrain(menuIndex + diff, 0, 4);
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

void handleStartStop(
  uint32_t nowTicks,
  bool& isRunning,
  int& globalStep,
  bool& needsRedraw
) {
  if (g_startStopBtn.update(nowTicks)) {
    isRunning = !isRunning;
    needsRedraw = true;

    if (isRunning) {
      sequencerStart(globalStep);
    }
  }
}
