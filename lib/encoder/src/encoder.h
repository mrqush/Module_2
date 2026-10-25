#include "driver/gpio.h"
#include "driver/pulse_cnt.h"

struct encoder_context_t {
    gpio_num_t pin_a;
    gpio_num_t pin_b;
    gpio_num_t sw_pin;

    pcnt_unit_handle_t unit;
    pcnt_channel_handle_t channel_a;
    pcnt_channel_handle_t channel_b;
};

using encoder_handle_t = encoder_context_t*;

int encoder_get_count(encoder_handle_t handle);
void encoder_init(encoder_handle_t handle);