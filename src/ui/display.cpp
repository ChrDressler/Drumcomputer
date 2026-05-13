#include "ui/display.h"

#include <string.h>

#include "data/banks.h"

namespace {

const char* kMenuItems[] = {
  "Play: BPM, Swing",
  "Pattern",
  "Pulsbreite",
  "Info"
};

const int kNumMenuItems = 4;
const int kArrowChar = 126;

bool shouldSkipUpdate(MenuMode currentMode, bool needsRedraw, bool playRefreshDue) {
  if (currentMode == PLAY) {
    return !needsRedraw && !playRefreshDue;
  }

  return !needsRedraw;
}

void fillLineWithSpaces(LiquidCrystal_I2C& lcd, int fromCol, int lineWidth = 20) {
  for (int col = fromCol; col < lineWidth; col++) {
    lcd.print(" ");
  }
}

void drawRootMenuLine(
  LiquidCrystal_I2C& lcd,
  int line,
  int selectedIndex,
  int currentBank,
  bool isRunning
) {
  lcd.setCursor(0, line);
  if (selectedIndex == line) {
    lcd.write(kArrowChar);
  } else {
    lcd.print(" ");
  }

  lcd.print(" ");

  if (line == 1) {
    lcd.write(isRunning ? 1 : 0);
    lcd.print(" ");

    char currentBankName[19];
    getBankName((uint8_t)currentBank, currentBankName, sizeof(currentBankName));
    lcd.print(currentBankName);

    const int len = (int)strlen(currentBankName) + 5;
    fillLineWithSpaces(lcd, len);
    return;
  }

  lcd.print(kMenuItems[line]);
  const int len = (int)strlen(kMenuItems[line]) + 2;
  fillLineWithSpaces(lcd, len);
}

void drawMenuRoot(
  LiquidCrystal_I2C& lcd,
  int menuIndex,
  int currentBank,
  bool isRunning,
  bool& needsRedraw,
  int& lastMenuIndex
) {
  if (needsRedraw) {
    for (int i = 0; i < kNumMenuItems; i++) {
      drawRootMenuLine(lcd, i, menuIndex, currentBank, isRunning);
    }
    lastMenuIndex = menuIndex;
    needsRedraw = false;
    return;
  }

  if (menuIndex != lastMenuIndex) {
    lcd.setCursor(0, lastMenuIndex);
    lcd.print("  ");
    lcd.setCursor(0, menuIndex);
    lcd.print("> ");
    lastMenuIndex = menuIndex;
  }
}

void drawPlayMode(LiquidCrystal_I2C& lcd, int bpm, float swingAmount, bool& needsRedraw) {
  lcd.setCursor(0, 0);
  lcd.print("- PLAY MODE -");
  lcd.setCursor(0, 1);
  lcd.print("BPM:   ");
  lcd.print(bpm);
  lcd.print("  ");
  lcd.setCursor(0, 2);
  lcd.print("Swing: ");
  lcd.print((int)(swingAmount * 100));
  lcd.print("% ");
  needsRedraw = false;
}

void drawEditPulseMode(LiquidCrystal_I2C& lcd, long pulseWidth, bool& needsRedraw) {
  lcd.setCursor(0, 0);
  lcd.print("- EDIT Pulsbreite");
  lcd.setCursor(0, 1);
  lcd.print("Pulsbreite: ");
  lcd.print((int)(pulseWidth / 1000));
  lcd.print(" ms     ");
  needsRedraw = false;
}

void drawInfoMode(LiquidCrystal_I2C& lcd, bool& needsRedraw) {
  if (!needsRedraw) {
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("*** Drumcomputer ***");
  lcd.setCursor(0, 1);
  lcd.print("Christoph Dressler  ");
  lcd.setCursor(0, 2);
  lcd.print("Soundmodule: 09/1998");
  lcd.setCursor(0, 3);
  lcd.print("Ansteuerung: 2026   ");
  needsRedraw = false;
}

void drawEditBankMode(
  LiquidCrystal_I2C& lcd,
  int currentBank,
  int& lastBank,
  bool& needsRedraw
) {
  if (currentBank == lastBank && !needsRedraw) {
    return;
  }

  int startIdx = (currentBank >= 4) ? (currentBank - 3) : 0;
  char bankName[19];

  for (int i = 0; i < 4; i++) {
    int bankIdx = startIdx + i;

    lcd.setCursor(0, i);
    if (bankIdx == currentBank) {
      lcd.write(kArrowChar);
    } else {
      lcd.print(" ");
    }
    lcd.print(" ");

    if (bankIdx < NUM_BANKS) {
      getBankName((uint8_t)bankIdx, bankName, sizeof(bankName));
      lcd.print(bankName);
      int remainingChars = 18 - (int)strlen(bankName);
      for (int j = 0; j < remainingChars; j++) {
        lcd.print(" ");
      }
    } else {
      lcd.print("                  ");
    }
  }

  lastBank = currentBank;
  needsRedraw = false;
}

} // namespace

void updateDisplay(
  LiquidCrystal_I2C& lcd,
  MenuMode currentMode,
  int menuIndex,
  bool& needsRedraw,
  bool playRefreshDue,
  int bpm,
  float swingAmount,
  long pulseWidth,
  int currentBank,
  int& lastBank,
  bool isRunning
) {
  static int lastMenuIndex = -1;
  static MenuMode lastMode = (MenuMode)-1;

  if (shouldSkipUpdate(currentMode, needsRedraw, playRefreshDue)) {
    return;
  }

  if (currentMode != lastMode) {
    lcd.clear();
    lastMode = currentMode;
    needsRedraw = true;
  }

  if (currentMode == MENU_ROOT) {
    drawMenuRoot(lcd, menuIndex, currentBank, isRunning, needsRedraw, lastMenuIndex);
  }
  else if (currentMode == PLAY) {
    drawPlayMode(lcd, bpm, swingAmount, needsRedraw);
  }
  else if (currentMode == EDIT_PULSE) {
    drawEditPulseMode(lcd, pulseWidth, needsRedraw);
  }
  else if (currentMode == INFO) {
    drawInfoMode(lcd, needsRedraw);
  }
  else if (currentMode == EDIT_BANK) {
    drawEditBankMode(lcd, currentBank, lastBank, needsRedraw);
  }
}
