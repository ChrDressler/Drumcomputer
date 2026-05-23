#include "ui/display.h"

#include <string.h>

#include "data/banks.h"

namespace {

const char* kMenuItems[] = {
  "Play: BPM, Swing",
  "Pattern",
  "Pulsbreite",
  "Info",
  "MIDI Monitor"
};

const int kNumMenuItems = 5;
const int kArrowChar = 126;

bool shouldSkipUpdate(MenuMode currentMode, bool needsRedraw, bool playRefreshDue) {
  if (currentMode == PLAY) {
    return !needsRedraw && !playRefreshDue;
  }

  // MIDI_MONITOR und alle anderen Modi: Nur bei Bedarf (needsRedraw) aktualisieren,
  // nicht periodisch. Das verhindert Flackern durch staendige Refreshs.
  return !needsRedraw;
}

void fillLineWithSpaces(LiquidCrystal_I2C& lcd, int fromCol, int lineWidth = 20) {
  for (int col = fromCol; col < lineWidth; col++) {
    lcd.print(" ");
  }
}

const int kLcdLines = 4;

void drawRootMenuLine(
  LiquidCrystal_I2C& lcd,
  int lcdLine,
  int itemIndex,
  int selectedIndex,
  int currentBank,
  bool isRunning
) {
  lcd.setCursor(0, lcdLine);
  if (itemIndex == selectedIndex) {
    lcd.write(kArrowChar);
  } else {
    lcd.print(" ");
  }

  lcd.print(" ");

  if (itemIndex == 1) {
    lcd.write(isRunning ? 1 : 0);
    lcd.print(" ");

    char currentBankName[19];
    getBankName((uint8_t)currentBank, currentBankName, sizeof(currentBankName));
    lcd.print(currentBankName);

    const int len = (int)strlen(currentBankName) + 5;
    fillLineWithSpaces(lcd, len);
    return;
  }

  lcd.print(kMenuItems[itemIndex]);
  const int len = (int)strlen(kMenuItems[itemIndex]) + 2;
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
  // Sichtbaren Ausschnitt berechnen: menuIndex soll in der Mitte sein,
  // aber nicht über den Anfang/das Ende hinausragen
  static int scrollOffset = 0;
  int newScrollOffset = constrain(menuIndex - 1, 0, kNumMenuItems - kLcdLines);

  if (needsRedraw || newScrollOffset != scrollOffset) {
    scrollOffset = newScrollOffset;
    lcd.clear();
    for (int i = 0; i < kLcdLines; i++) {
      int itemIndex = scrollOffset + i;
      if (itemIndex < kNumMenuItems) {
        drawRootMenuLine(lcd, i, itemIndex, menuIndex, currentBank, isRunning);
      } else {
        // Leere Zeile am Ende, wenn nicht genug Items vorhanden
        lcd.setCursor(0, i);
        fillLineWithSpaces(lcd, 0);
      }
    }
    lastMenuIndex = menuIndex;
    needsRedraw = false;
    return;
  }

  if (menuIndex != lastMenuIndex) {
    // Nur den Pfeil aktualisieren – alte Position löschen, neue setzen
    int oldLcdLine = lastMenuIndex - scrollOffset;
    int newLcdLine = menuIndex - scrollOffset;
    if (oldLcdLine >= 0 && oldLcdLine < kLcdLines) {
      lcd.setCursor(0, oldLcdLine);
      lcd.print("  ");
    }
    if (newLcdLine >= 0 && newLcdLine < kLcdLines) {
      lcd.setCursor(0, newLcdLine);
      lcd.print("> ");
    }
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

void drawMidiMonitorMode(LiquidCrystal_I2C& lcd, const char* midiMsg, bool& needsRedraw) {
  lcd.setCursor(0, 0);
  lcd.print("--- MIDI Monitor ---");
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 1);
  if (midiMsg != nullptr && strlen(midiMsg) > 0) {
    lcd.print(midiMsg);
  }
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
  bool isRunning,
  const char* midiMsg
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
  else if (currentMode == MIDI_MONITOR) {
    drawMidiMonitorMode(lcd, midiMsg, needsRedraw);
  }
}
