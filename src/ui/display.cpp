#include "ui/display.h"

#include <string.h>

#include "data/banks.h"

void updateDisplay(
  LiquidCrystal_I2C& lcd,
  MenuMode currentMode,
  int menuIndex,
  bool& needsRedraw,
  unsigned long& lastDisplayUpdate,
  int bpm,
  float swingAmount,
  long pulseWidth,
  int currentBank,
  int& lastBank
) {
  static int lastMenuIndex = -1;
  static MenuMode lastMode = (MenuMode)-1;
  const char* menuItems[] = {
    "Play: BPM, Swing",
    "Pattern",
    "Pulsbreite",
    "Info"
  };
  const int numMenuItems = 4; // Anzahl der Einträge
  const int pfeilZeichen = 126; // ASCII-Code für das Pfeilzeichen (>) auf dem LCD

  if (currentMode == PLAY)
  {
    if (millis() - lastDisplayUpdate < 250) return;     
  }
  else
  {
    if (!needsRedraw) return;  
  }
  
  if (currentMode != lastMode) {
    lcd.clear();
    lastMode = currentMode;
    needsRedraw = true;
  }

  lastDisplayUpdate = millis();

  if (currentMode == MENU_ROOT) {
    if (needsRedraw) {
      for (int i = 0; i < numMenuItems; i++) {
        lcd.setCursor(0, i);
        if (menuIndex == i) lcd.write(pfeilZeichen);
        else lcd.print(" ");

        lcd.print(" "); 
        lcd.print(menuItems[i]);
      }
      lastMenuIndex = menuIndex;
      needsRedraw = false;
    }
    else if (menuIndex != lastMenuIndex) {
      lcd.setCursor(0, lastMenuIndex); lcd.print("  ");
      lcd.setCursor(0, menuIndex);     lcd.print("> ");
      lastMenuIndex = menuIndex;
    }
  }
  else if (currentMode == PLAY) {
    lcd.setCursor(0, 0); lcd.print("- PLAY MODE ---   ");
    lcd.setCursor(0, 1); lcd.print("BPM:   "); lcd.print(bpm);
    lcd.setCursor(0, 2); lcd.print("Swing: "); lcd.print((int)(swingAmount * 100));
    lcd.print("%   ");
    needsRedraw = false;
  }
  else if (currentMode == EDIT_PULSE) {
    lcd.setCursor(0, 0); lcd.print("- EDIT Pulsbreite");
    lcd.setCursor(0, 1); lcd.print("Pulsbreite: ");
    lcd.print((int)(pulseWidth / 1000));
    lcd.print(" ms     ");
    needsRedraw = false;

    Serial.print("pw: ");
    Serial.println(pulseWidth);
  }
  else if (currentMode == INFO) {
    if (needsRedraw) {
      lcd.setCursor(0, 0); lcd.print("*** Drumcomputer ***");
      lcd.setCursor(0, 1); lcd.print("Christoph Dressler  ");
      lcd.setCursor(0, 2); lcd.print("Soundmodule: 09/1998");
      lcd.setCursor(0, 3); lcd.print("Ansteuerung: 2026   ");
      needsRedraw = false;
    }
  }
  else if (currentMode == EDIT_BANK) {
    if (currentBank != lastBank || needsRedraw) {
      int startIdx = (currentBank >= 4) ? (currentBank - 3) : 0;
      char bankName[19];
      for (int i = 0; i < 4; i++) {
        int bankIdx = startIdx + i;
        
        lcd.setCursor(0, i);
        if (bankIdx == currentBank) lcd.write(pfeilZeichen);
        else lcd.print(" ");
        lcd.print(" ");

        if (bankIdx < NUM_BANKS) {
          // Fetch bank label from PROGMEM into a small stack buffer for LCD output.
          getBankName((uint8_t)bankIdx, bankName, sizeof(bankName));
          lcd.print(bankName);
          int remainingChars = 18 - strlen(bankName);
          for (int j = 0; j < remainingChars; j++) lcd.print(" ");
        }
        else {
          lcd.print("                  ");
        }
      }
      lastBank = currentBank;
      needsRedraw = false;
    }
  }
}
