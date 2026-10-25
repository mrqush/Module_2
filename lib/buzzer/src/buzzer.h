#pragma once

#include "driver/gpio.h"

void buzzer_init(gpio_num_t pin);
void buzzer_set_duty(uint32_t duty);
void buzzer_alarm();
