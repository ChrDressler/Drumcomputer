#include "core/sequencer.h"

#include "core/app_config.h"
#include "data/banks.h"

void runSequencer(
  unsigned long now,
  int bpm,
  float swingAmount,
  int currentBank,
  const uint8_t probability[8],
  long pulseWidth,
  int& globalStep,
  unsigned long& lastBeatTime,
  unsigned long& ledOffTime,
  unsigned long& lastStepTime,
  unsigned long& triggerOffTime,
  bool& triggersActive
) {
  unsigned long t = 60000000UL / bpm;
  unsigned long base_step = t / 4;
  unsigned long swing_offset = base_step * swingAmount;
  bool is_second_step = (globalStep % 2 == 1);
  unsigned long step_duration = is_second_step ? (base_step + swing_offset) : (base_step - swing_offset);

  if (now - lastBeatTime >= t) {
    lastBeatTime = now;
    digitalWrite(13, HIGH);
    ledOffTime = now + 20000;
  }
  if (now >= ledOffTime) digitalWrite(13, LOW);

  if (now - lastStepTime >= step_duration) {
    lastStepTime = now;
    int current_bar = globalStep / 16;
    int local_step = globalStep % 16;
    uint8_t active_mask_d = 0;
    uint8_t active_mask_b = 0;

    for (int ch = 0; ch < 8; ch++) {
      // Read the current step pattern from flash (banks table is in PROGMEM).
      uint16_t patternWord = getBankPatternWord((uint8_t)currentBank, (uint8_t)ch, (uint8_t)current_bar);
      if ((patternWord >> (15 - local_step)) & 1) {
        if (random(0, 101) <= probability[ch]) {
          int pin = trigPins[ch];
          if (pin <= 7) active_mask_d |= (1 << pin);
          else active_mask_b |= (1 << (pin - 8));
        }
      }
    }

    if (active_mask_d || active_mask_b) {
      PORTD &= ~active_mask_d;
      PORTB &= ~active_mask_b;
      triggerOffTime = now + pulseWidth;
      triggersActive = true;
    }
    globalStep = (globalStep + 1) % 64;
  }

  if (triggersActive && now >= triggerOffTime) {
    PORTD |= 0b11111100;
    PORTB |= 0b00000011;
    triggersActive = false;
  }
}
