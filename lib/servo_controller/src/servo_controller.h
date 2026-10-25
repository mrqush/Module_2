#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

// Initialize PWM and set servo to LEFT
esp_err_t servo_init(gpio_num_t signal_pin);

// Move to calibrated positions
esp_err_t servo_set_left();
esp_err_t servo_set_right();
