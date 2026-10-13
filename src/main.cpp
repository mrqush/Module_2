#include <stdint.h>
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>

static const char *TAG = "servo_3_5";

static constexpr gpio_num_t SERVO_PIN = GPIO_NUM_17;
static constexpr adc_channel_t POT_CHANNEL = ADC_CHANNEL_3; // GPIO4
static constexpr uint32_t SERVO_HZ = 50;
static constexpr uint32_t PERIOD_US = 1000000 / SERVO_HZ;
static constexpr ledc_timer_bit_t RESOLUTION = LEDC_TIMER_13_BIT;
static constexpr uint32_t COUNTS = 1u << static_cast<uint32_t>(RESOLUTION);
static constexpr ledc_timer_t TIMER = LEDC_TIMER_0;
static constexpr ledc_channel_t CHANNEL = LEDC_CHANNEL_0;

static constexpr uint32_t SERVO_LEFT_US = 500;
static constexpr uint32_t SERVO_RIGHT_US = 2400;

static constexpr uint32_t RAW_LEFT = 0;
static constexpr uint32_t RAW_RIGHT = 4095;

static_assert(
    RAW_RIGHT > RAW_LEFT,
    "RAW_RIGHT must be greater than RAW_LEFT"
);

static constexpr uint32_t UPDATE_MS = 20;
static constexpr uint32_t SAMPLE_COUNT = 8;

static uint32_t pulse_to_duty(uint32_t pulse_us)
{
    return static_cast<uint32_t>((static_cast<uint64_t>(pulse_us) * COUNTS + PERIOD_US / 2) / PERIOD_US);
}

static void init_servo()
{
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.timer_num = TIMER;
    timer.duty_resolution = RESOLUTION;
    timer.freq_hz = SERVO_HZ;
    timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {};
    channel.gpio_num = SERVO_PIN;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = CHANNEL;
    channel.intr_type = LEDC_INTR_DISABLE;
    channel.timer_sel = TIMER;
    channel.duty = 0;
    channel.hpoint = 0;
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

static adc_oneshot_unit_handle_t init_adc()
{
    adc_oneshot_unit_handle_t adc_handle = nullptr;
    adc_oneshot_unit_init_cfg_t adc_unit = {};
    adc_unit.unit_id = ADC_UNIT_1;
    adc_unit.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit, &adc_handle));

    adc_oneshot_chan_cfg_t channel = {};
    channel.bitwidth = ADC_BITWIDTH_12;
    channel.atten = ADC_ATTEN_DB_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, POT_CHANNEL, &channel));
    return adc_handle;
}

static esp_err_t read_pot(adc_oneshot_unit_handle_t adc, int *average)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i)
    {
        int raw = 0;
        esp_err_t error = adc_oneshot_read(adc, POT_CHANNEL, &raw);
        if (error != ESP_OK)
            return error;
        sum += raw;
    }
    *average = sum / SAMPLE_COUNT;
    return ESP_OK;
}

static int set_servo_from_raw(int raw)
{
    float position = static_cast<float>(raw - RAW_LEFT) / (RAW_RIGHT - RAW_LEFT);
    
    if (position < 0.0f) {
        position = 0.0f;
    }
    
    if (position > 1.0f) {
        position = 1.0f;
    }

    float angle = position * 180.0f;

    const float pulse = SERVO_LEFT_US + position * (static_cast<float>(SERVO_RIGHT_US) - SERVO_LEFT_US);
    const int pulse_us = static_cast<int>(pulse + 0.5f);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNEL, pulse_to_duty(pulse_us)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNEL));

    ESP_LOGI(
        TAG,
        "Raw: %d, Target angle: %.2f deg, Position: %.3f, Pulse: %d us",
        raw,
        angle,
        position,
        pulse_us
    );

    return pulse_us;
}

extern "C" void app_main(void)
{
    adc_oneshot_unit_handle_t adc = init_adc();
    init_servo();

    while (true)
    {
        int raw = 0;
        esp_err_t error = read_pot(adc, &raw);
        if (error == ESP_OK)
        {
            set_servo_from_raw(raw);
        }

        vTaskDelay(pdMS_TO_TICKS(UPDATE_MS));
    }
}
