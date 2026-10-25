#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "encoder.h"
#include <esp_log.h>

static constexpr int HIGH_LIMIT = 1000;
static constexpr int LOW_LIMIT = -1000;

static const char *TAG = "ENCODER";

int encoder_get_count(encoder_handle_t handle){

    int count = 0;

    ESP_ERROR_CHECK(pcnt_unit_get_count(handle->unit, &count));

    return count;
}

void encoder_init(encoder_handle_t handle) {
    
    pcnt_unit_config_t unit_cfg = {};

    unit_cfg.high_limit = HIGH_LIMIT;
    unit_cfg.low_limit = LOW_LIMIT;
    unit_cfg.flags.accum_count = true;

    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &handle->unit));

    gpio_config_t io_cfg = {};

    io_cfg.pin_bit_mask = (1ULL << handle->sw_pin);
    io_cfg.mode = GPIO_MODE_INPUT;
    io_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_cfg.intr_type = GPIO_INTR_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    pcnt_glitch_filter_config_t filter_cfg = {};
    filter_cfg.max_glitch_ns = 1000;

    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(handle->unit, &filter_cfg));

    pcnt_chan_config_t cfg_a = {};

    cfg_a.edge_gpio_num = handle->pin_a;
    cfg_a.level_gpio_num = handle->pin_b;

    ESP_ERROR_CHECK(pcnt_new_channel(handle->unit, &cfg_a, &handle->channel_a));

    pcnt_chan_config_t cfg_b = {};

    cfg_b.edge_gpio_num = handle->pin_b;
    cfg_b.level_gpio_num = handle->pin_a;

    ESP_ERROR_CHECK(pcnt_new_channel(handle->unit, &cfg_b, &handle->channel_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        handle->channel_a,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        handle->channel_a,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        handle->channel_b,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE
    ));

    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        handle->channel_b,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE
    ));

    ESP_ERROR_CHECK(gpio_pullup_en(handle->pin_a));
    ESP_ERROR_CHECK(gpio_pullup_en(handle->pin_b));

    // 8. Configure overflow compensation
    ESP_ERROR_CHECK(
        pcnt_unit_add_watch_point(handle->unit, HIGH_LIMIT)
    );

    ESP_ERROR_CHECK(
        pcnt_unit_add_watch_point(handle->unit, LOW_LIMIT)
    );

    // 9. Start PCNT
    ESP_ERROR_CHECK(pcnt_unit_enable(handle->unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(handle->unit));
    ESP_ERROR_CHECK(pcnt_unit_start(handle->unit));

    ESP_LOGI(TAG, "Encoder initialized");
}