#pragma once

#include <Arduino.h>

/**
 * @brief Entprellt einen Taster mit Flankenerkennung und stabiler Zustandsprüfung.
 *
 * Erkennt eine fallende Flanke (HIGH->LOW), wartet eine kurze Zeit und prüft
 * dann ob der Taster immer noch gedrückt ist. Das verhindert Fehlauslösungen
 * durch Prellen oder EMV-Einstreuungen.
 *
 * @param pin GPIO-Pin des Tasters (INPUT_PULLUP).
 * @param lastState [in,out] Letzter gelesener Zustand (für Flankenerkennung).
 * @param debounceUntil [in,out] Tick-Zeitpunkt bis zu dem gesperrt ist.
 * @param nowTicks Aktueller Tick-Zähler (z.B. von timerSchedulerNowTicks()).
 * @param debounceMs Entprellzeit in Millisekunden (z.B. 300ms).
 * @param stableCheckMs Wartezeit für die stabile Zustandsprüfung (z.B. 5ms).
 * @return true wenn der Taster sauber gedrückt wurde (entprellt + stabil).
 */
inline bool debounceButton(
  int pin,
  bool& lastState,
  uint32_t& debounceUntil,
  uint32_t nowTicks,
  uint32_t debounceMs,
  uint32_t stableCheckMs = 5
) {
  bool currentState = digitalRead(pin);

  // Fallende Flanke erkannt UND Entprellzeit abgelaufen?
  if (currentState == LOW && lastState == HIGH && (int32_t)(nowTicks - debounceUntil) >= 0) {
    
    // Stabile Zustandsprüfung: Kurz warten und nochmal prüfen
    delay(stableCheckMs);
    if (digitalRead(pin) == LOW) {
      lastState = currentState;
      debounceUntil = nowTicks + (debounceMs * 1000UL / 500UL); // ms in Ticks umrechnen
      return true;
    }
  }

  lastState = currentState;
  return false;
}
