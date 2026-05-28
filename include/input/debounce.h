#pragma once

#include <Arduino.h>

/**
 * @brief Entprellt einen Taster mit Flankenerkennung und stabiler Zustandsprüfung.
 *
 * Einfach eine Instanz pro Taster anlegen und in loop() regelmässig update() aufrufen.
 * Erkennt eine fallende Flanke (HIGH->LOW), wartet 5ms und prüft dann ob der
 * Taster immer noch gedrückt ist. Nach erfolgreicher Erkennung wird für 300ms
 * gesperrt.
 *
 * Beispiel:
 * @code
 * DebounceButton startStopBtn(A0);
 * 
 * void loop() {
 *   if (startStopBtn.update(nowTicks)) {
 *     // Taster wurde sauber gedrückt
 *   }
 * }
 * @endcode
 */
class DebounceButton {
public:
  /**
   * @param pin GPIO-Pin des Tasters (INPUT_PULLUP, muss extern konfiguriert sein).
   */
  DebounceButton(int pin);

  /**
   * @brief Muss regelmässig aufgerufen werden (z.B. in loop()).
   * @param nowTicks Aktueller Tick-Zähler (z.B. von timerSchedulerNowTicks()).
   * @return true wenn der Taster sauber gedrückt wurde (entprellt + stabil).
   */
  bool update(uint32_t nowTicks);

private:
  int m_pin;
  bool m_lastState;
  uint32_t m_debounceUntil;
};
