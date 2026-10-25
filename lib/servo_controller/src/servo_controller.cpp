
#include "servo_controller.h"

#include <cstdint>
#include "driver/ledc.h"
#include "esp_log.h"

namespace {

constexpr char TAG[] = "SERVO";

constexpr uint32_t SERVO_HZ = 50;
constexpr uint32_t PERIOD_US = 1000000u / SERVO_HZ;
constexpr ledc_timer_bit_t SERVO_RESOLUTION = LEDC_TIMER_13_BIT;
constexpr uint32_t PWM_COUNTS = 1u << static_cast<uint32_t>(SERVO_RESOLUTION);
constexpr ledc_mode_t SERVO_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t SERVO_TIMER = LEDC_TIMER_0;
constexpr ledc_channel_t SERVO_CHANNEL = LEDC_CHANNEL_0;

constexpr uint32_t SERVO_LEFT_US = 500;
constexpr uint32_t SERVO_RIGHT_US = 2400;

bool servo_initialized = false;

uint32_t pulse_to_duty(uint32_t pulse_us)
{
    return static_cast<uint32_t>((static_cast<uint64_t>(pulse_us) * PWM_COUNTS + PERIOD_US / 2u) / PERIOD_US);
}

// Internal function for changing servo position
esp_err_t servo_set_pulse(uint32_t pulse_us)
{
    if (!servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (pulse_us < SERVO_LEFT_US ||
        pulse_us > SERVO_RIGHT_US) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ledc_set_duty(
        SERVO_MODE,
        SERVO_CHANNEL,
        pulse_to_duty(pulse_us)
    );

    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(
        SERVO_MODE,
        SERVO_CHANNEL
    );
}

}

esp_err_t servo_init(gpio_num_t signal_pin)
{
    if (servo_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ledc_timer_config_t timer = {};

    timer.speed_mode = SERVO_MODE;
    timer.timer_num = SERVO_TIMER;
    timer.duty_resolution = SERVO_RESOLUTION;
    timer.freq_hz = SERVO_HZ;
    timer.clk_cfg = LEDC_AUTO_CLK;

    esp_err_t err = ledc_timer_config(&timer);

    if (err != ESP_OK) {
        return err;
    }

    ledc_channel_config_t channel = {};

    channel.gpio_num = signal_pin;
    channel.speed_mode = SERVO_MODE;
    channel.channel = SERVO_CHANNEL;
    channel.timer_sel = SERVO_TIMER;
    channel.intr_type = LEDC_INTR_DISABLE;

    // Initial position: LEFT
    channel.duty = pulse_to_duty(SERVO_LEFT_US);
    channel.hpoint = 0;

    err = ledc_channel_config(&channel);

    if (err != ESP_OK) {
        return err;
    }

    servo_initialized = true;

    ESP_LOGI(TAG, "Servo initialized at LEFT");

    return ESP_OK;
}

esp_err_t servo_set_left()
{
    esp_err_t err = servo_set_pulse(SERVO_LEFT_US);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Servo -> LEFT");
    }

    return err;
}

esp_err_t servo_set_right()
{
    esp_err_t err = servo_set_pulse(SERVO_RIGHT_US);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Servo -> RIGHT");
    }

    return err;
}
