#include <stdint.h>
#include <stdio.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "pwm.h"

#define LED_PIN GPIO_NUM_16
#define MOTOR_PWM_PIN GPIO_NUM_17

#define LED_POT_CHANNEL ADC_CHANNEL_3
#define MOTOR_POT_CHANNEL ADC_CHANNEL_4

static constexpr int ADC_LOW_LIMIT = 40;
static constexpr int ADC_HIGH_LIMIT = 4055;

static constexpr uint32_t LED_FREQUENCY_HZ = 1000;
static constexpr uint32_t MOTOR_FREQUENCY_HZ = 20000;

static constexpr ledc_timer_bit_t LED_RESOLUTION = LEDC_TIMER_8_BIT;
static constexpr ledc_timer_bit_t MOTOR_RESOLUTION = LEDC_TIMER_10_BIT;

static constexpr uint32_t LED_MAX_DUTY = (1u << static_cast<uint32_t>(LED_RESOLUTION)) - 1u;
static constexpr uint32_t MOTOR_MAX_DUTY = (1u << static_cast<uint32_t>(MOTOR_RESOLUTION)) - 1u;

static constexpr uint32_t UPDATE_INTERVAL_MS = 20;

static adc_oneshot_unit_handle_t init_adc()
{
    adc_oneshot_unit_handle_t adc_handle = nullptr;

    adc_oneshot_unit_init_cfg_t unit_config = {};
    unit_config.unit_id = ADC_UNIT_1;
    unit_config.ulp_mode = ADC_ULP_MODE_DISABLE;

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(&unit_config, &adc_handle)
    );

    adc_oneshot_chan_cfg_t channel_config = {};
    channel_config.bitwidth = ADC_BITWIDTH_12;
    channel_config.atten = ADC_ATTEN_DB_12;

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            LED_POT_CHANNEL,
            &channel_config
        )
    );

    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            MOTOR_POT_CHANNEL,
            &channel_config
        )
    );

    return adc_handle;
}

static uint32_t adc_to_duty(int reading, uint32_t max_duty)
{
    if (reading <= ADC_LOW_LIMIT) {
        return 0;
    }

    if (reading >= ADC_HIGH_LIMIT) {
        return max_duty;
    }

    const uint32_t input = static_cast<uint32_t>(reading - ADC_LOW_LIMIT);

    const uint32_t input_range = ADC_HIGH_LIMIT - ADC_LOW_LIMIT;

    return (input * max_duty) / input_range;
}

static void init_led_pwm(pwm_t *led_pwm)
{
    pwm_config_t config = {};

    config.gpio = LED_PIN;
    config.channel = LEDC_CHANNEL_0;
    config.timer = LEDC_TIMER_0;
    config.frequency_hz = LED_FREQUENCY_HZ;
    config.resolution = LED_RESOLUTION;
    config.duty = 0;
    config.inverted = false;

    ESP_ERROR_CHECK(pwm_init(led_pwm, &config));
}

static void init_motor_pwm(pwm_t *motor_pwm)
{
    pwm_config_t config = {};

    config.gpio = MOTOR_PWM_PIN;
    config.channel = LEDC_CHANNEL_1;
    config.timer = LEDC_TIMER_1;
    config.frequency_hz = MOTOR_FREQUENCY_HZ;
    config.resolution = MOTOR_RESOLUTION;
    config.duty = 0;
    config.inverted = false;

    ESP_ERROR_CHECK(pwm_init(motor_pwm, &config));
}

extern "C" void app_main(void)
{
    pwm_t led_pwm = {};
    pwm_t motor_pwm = {};

    init_motor_pwm(&motor_pwm);
    init_led_pwm(&led_pwm);

    adc_oneshot_unit_handle_t adc_handle = init_adc();

    while (true) {
        int led_adc_raw = 0;
        int motor_adc_raw = 0;

        const esp_err_t led_error = adc_oneshot_read(
            adc_handle,
            LED_POT_CHANNEL,
            &led_adc_raw
        );

        const esp_err_t motor_error = adc_oneshot_read(
            adc_handle,
            MOTOR_POT_CHANNEL,
            &motor_adc_raw
        );

        // If a reading fails, turn off its corresponding output.
        uint32_t led_duty = 0;
        uint32_t motor_duty = 0;

        if (led_error == ESP_OK) {
            led_duty = adc_to_duty(
                led_adc_raw,
                LED_MAX_DUTY
            );
        }

        if (motor_error == ESP_OK) {
            motor_duty = adc_to_duty(
                motor_adc_raw,
                MOTOR_MAX_DUTY
            );
        }

        ESP_ERROR_CHECK(
            pwm_set_duty(&led_pwm, led_duty)
        );

        ESP_ERROR_CHECK(
            pwm_set_duty(&motor_pwm, motor_duty)
        );

        vTaskDelay(pdMS_TO_TICKS(UPDATE_INTERVAL_MS));
    }
}