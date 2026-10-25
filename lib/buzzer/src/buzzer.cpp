#include "driver/ledc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr ledc_mode_t BUZZER_MODE = LEDC_LOW_SPEED_MODE;
static constexpr ledc_timer_t BUZZER_TIMER = LEDC_TIMER_1;
static constexpr ledc_channel_t BUZZER_CHANNEL = LEDC_CHANNEL_2;
static constexpr ledc_timer_bit_t BUZZER_RESOLUTION = LEDC_TIMER_10_BIT;
static constexpr uint32_t BUZZER_DUTY = (1u << static_cast<uint32_t>(BUZZER_RESOLUTION)) / 2;

void buzzer_init(gpio_num_t pin)
{
    ledc_timer_config_t timer = {};

    timer.speed_mode = BUZZER_MODE;
    timer.timer_num = BUZZER_TIMER;
    timer.duty_resolution = BUZZER_RESOLUTION;
    timer.freq_hz = 1000;
    timer.clk_cfg = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {};

    channel.gpio_num = pin;
    channel.speed_mode = BUZZER_MODE;
    channel.channel = BUZZER_CHANNEL;
    channel.timer_sel = BUZZER_TIMER;
    channel.duty = 0;
    channel.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

void buzzer_set_duty(uint32_t duty)
{
    ESP_ERROR_CHECK(
        ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, duty)
    );

    ESP_ERROR_CHECK(
        ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL)
    );
}


void buzzer_alarm()
{
    // Enable sound at 50% duty cycle
    buzzer_set_duty(BUZZER_DUTY);

    for (int i = 0; i < 3; i++) {

        // Rising frequency
        for (int freq = 700; freq <= 1700; freq += 100) {

            ESP_ERROR_CHECK(
                ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq)
            );

            vTaskDelay(pdMS_TO_TICKS(30));
        }

        // Falling frequency
        for (int freq = 1600; freq >= 700; freq -= 100) {

            ESP_ERROR_CHECK(
                ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, freq)
            );

            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }

    // Disable sound
    buzzer_set_duty(0);
}