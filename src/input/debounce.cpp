#include "input/debounce.h"

DebounceButton::DebounceButton(int pin)
  : m_pin(pin), m_lastState(HIGH), m_debounceUntil(0) {}

bool DebounceButton::update(uint32_t nowTicks) {
  bool currentState = digitalRead(m_pin);

  // Fallende Flanke erkannt UND Entprellzeit abgelaufen?
  if (currentState == LOW && m_lastState == HIGH && (int32_t)(nowTicks - m_debounceUntil) >= 0) {
    
    // Stabile Zustandsprüfung: 5ms warten und nochmal prüfen
    delay(5);
    if (digitalRead(m_pin) == LOW) {
      m_lastState = currentState;
      // 300ms Sperre (in Ticks: 300ms * 1000µs/ms / 500µs/Tick = 600 Ticks)
      m_debounceUntil = nowTicks + 600UL;
      return true;
    }
  }

  m_lastState = currentState;
  return false;
}
