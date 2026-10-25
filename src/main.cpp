#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "driver/pulse_cnt.h"
#include <esp_log.h>
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo_controller.h"
#include "buzzer.h"
#include "encoder.h"

static const char *TAG = "SAFE";

static constexpr gpio_num_t ENC_A_PIN = GPIO_NUM_16;
static constexpr gpio_num_t ENC_B_PIN = GPIO_NUM_17;
static constexpr gpio_num_t ENC_SW_PIN = GPIO_NUM_18;
static constexpr gpio_num_t BUZZER_PIN = GPIO_NUM_21;
static constexpr gpio_num_t SERVO_PIN = GPIO_NUM_4;

static constexpr uint16_t code = 3215;
static constexpr int CODE_LENGTH = 4;
static constexpr int COUNTS_PER_CLICK = 4;
static constexpr int ATTEMPTS = 3;

static constexpr TickType_t BUTTON_DEBOUNCE_TICKS = pdMS_TO_TICKS(30) > 0 ? pdMS_TO_TICKS(30) : 1;

static bool button_was_pressed(gpio_num_t pin) {
    static int previous_level = 1;
    static TickType_t last_press_tick = 0;

    const TickType_t now = xTaskGetTickCount();

    if (static_cast<TickType_t>(now - last_press_tick) < BUTTON_DEBOUNCE_TICKS) {
        return false;
    }

    const int level = gpio_get_level(pin);
    const bool pressed = previous_level == 1 && level == 0;

    previous_level = level;

    if (pressed) {
        last_press_tick = now;
    }

    return pressed;
}

static encoder_context_t encoder = {
    ENC_A_PIN,
    ENC_B_PIN,
    ENC_SW_PIN,
    nullptr,
    nullptr,
    nullptr
};

extern "C" void app_main(void)
{
    encoder_init(&encoder);
    buzzer_init(BUZZER_PIN);

    ESP_ERROR_CHECK(servo_init(SERVO_PIN));

    int previous_count = encoder_get_count(&encoder);
    int raw_remainder = 0;
    int previous_direction = 0;

    int current_digit = 0;
    int digit_index = 0;

    int remainig_attempts = ATTEMPTS;

    int entered_code[CODE_LENGTH] = {};

    bool code_complete = false;

    ESP_LOGI(TAG, "Remaining attempts: %d", remainig_attempts);
    std::printf("Current digit: 0");
    std::fflush(stdout);

    while (true) {

        if (button_was_pressed(encoder.sw_pin)) {

            if (!code_complete) {
                std::printf("\n");
                std::fflush(stdout);
            }

            remainig_attempts--;

            if(remainig_attempts <= 0) {
                ESP_LOGI(TAG, "No attempts remaining.");
                buzzer_alarm();
                return;
            }

            for (int &digit : entered_code) {
                digit = 0;
            }

            digit_index = 0;
            current_digit = 0;
            previous_direction = 0;
            raw_remainder = 0;
            code_complete = false;

            previous_count = encoder_get_count(&encoder);

            ESP_LOGI(TAG, "Code reset. Remainig attempts: %d", remainig_attempts);
            std::printf("Current digit: 0");
            std::fflush(stdout);
        }

        int count = encoder_get_count(&encoder);

        int delta = count - previous_count;
        previous_count = count;

        if (!code_complete) {
            raw_remainder += delta;
        }

        while (!code_complete &&
               (raw_remainder >= COUNTS_PER_CLICK ||
                raw_remainder <= -COUNTS_PER_CLICK)) {

            int direction = (raw_remainder > 0) ? 1 : -1;

            raw_remainder -= direction * COUNTS_PER_CLICK;

            if (previous_direction != 0 &&
                direction != previous_direction) {

                entered_code[digit_index] = current_digit;

                std::printf("\n");
                std::fflush(stdout);
                ESP_LOGI(TAG, "Digit %d confirmed: %d", digit_index + 1, current_digit);

                if (digit_index == CODE_LENGTH - 1) {

                    uint32_t entered_pin = 0;

                    for (int digit : entered_code) {
                        entered_pin = entered_pin * 10 + digit;
                    }

                    ESP_LOGI(TAG, "Entered PIN: %04lu", static_cast<unsigned long>(entered_pin));

                    if (entered_pin == code) {
                        code_complete = true;
                        ESP_LOGI(TAG, "Access granted!");
                        ESP_ERROR_CHECK(servo_set_right());
                    } else {
                        ESP_LOGI(TAG, "Access denied! Incorrect PIN.");
                        remainig_attempts--;
                        ESP_LOGI(TAG, "Remaining attempts: %d", remainig_attempts);

                        if (remainig_attempts <= 0) {
                            ESP_LOGI(TAG, "No attempts remaining.");
                            buzzer_alarm();
                            return;
                        }

                        digit_index = 0;
                        current_digit = 0;

                        std::printf("Current digit: 0");
                        std::fflush(stdout);
                    }

                } else {
                    digit_index++;

                    current_digit = 0;

                    std::printf("Current digit: 0");
                    std::fflush(stdout);
                }

                previous_direction = direction;
                continue;
            }

            previous_direction = direction;

            current_digit = (current_digit + 1) % 10;

            std::printf("\b%d", current_digit);
            std::fflush(stdout);
        }

        vTaskDelay(1);
    }
}
