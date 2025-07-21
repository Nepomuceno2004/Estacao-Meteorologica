#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

void buzzer_init(uint BUZZER_PIN);
void buzzer_play(uint BUZZER_PIN, uint freq, uint duration_ms);
void buzzer_off(uint BUZZER_PIN);

#endif
