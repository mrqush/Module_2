#include <stddef.h>
#include "pwm.h"
#include "esp_err.h"
#include "esp_timer.h"

static constexpr gpio_num_t BUZZER_PIN = GPIO_NUM_17;

static constexpr ledc_timer_bit_t BUZZER_RESOLUTION = LEDC_TIMER_10_BIT;
static constexpr uint32_t BUZZER_HALF_DUTY = (1u << static_cast<uint32_t>(BUZZER_RESOLUTION)) / 2u;

static constexpr uint32_t TICK_MS = 50;

struct Note {
    uint32_t frequency_hz;
    uint32_t duration_ticks;
};

static constexpr uint32_t NOTE_G4  = 392;
static constexpr uint32_t NOTE_EB4 = 311;
static constexpr uint32_t NOTE_BB4 = 466;
static constexpr uint32_t NOTE_D5  = 587;
static constexpr uint32_t NOTE_EB5 = 622;
static constexpr uint32_t NOTE_G5  = 784;
static constexpr uint32_t NOTE_FS5 = 740;

static constexpr uint32_t REST = 0;

static constexpr Note imperial_march[] = {
    // Phrase 1: G G G | Eb Bb G | Eb Bb G
    {NOTE_G4,  9}, {REST, 1},
    {NOTE_G4,  9}, {REST, 1},
    {NOTE_G4,  9}, {REST, 1},

    {NOTE_EB4, 6}, {REST, 1},
    {NOTE_BB4, 2}, {REST, 1},
    {NOTE_G4,  9}, {REST, 1},

    {NOTE_EB4, 6}, {REST, 1},
    {NOTE_BB4, 2}, {REST, 1},
    {NOTE_G4, 19}, {REST, 1},

    // Phrase 2: D D D | Eb Bb Gb | Eb Bb G
    {NOTE_D5,  9}, {REST, 1},
    {NOTE_D5,  9}, {REST, 1},
    {NOTE_D5,  9}, {REST, 1},

    {NOTE_EB5, 6}, {REST, 1},
    {NOTE_BB4, 2}, {REST, 1},
    {NOTE_FS5, 9}, {REST, 1},

    {NOTE_EB5, 6}, {REST, 1},
    {NOTE_BB4, 2}, {REST, 1},
    {NOTE_G4, 19}, {REST, 1},
};

static constexpr size_t MELODY_LENGTH = sizeof(imperial_march) / sizeof(imperial_march[0]);

static esp_timer_handle_t player_timer = nullptr;

static size_t current_note_index = 0;
static uint32_t remaining_ticks = 0;
static bool playing = false;

static void start_note(pwm_t *buzzer_pwm, const Note &note) 
{
    // Mute the buzzer before changing notes.
    ESP_ERROR_CHECK(pwm_set_duty(buzzer_pwm, 0));

    remaining_ticks = note.duration_ticks;
    
    if (note.frequency_hz == REST) {
        return;
    }

    ESP_ERROR_CHECK(
        ledc_set_freq(
            LEDC_LOW_SPEED_MODE,
            buzzer_pwm->config.timer,
            note.frequency_hz
        )
    );
    
    buzzer_pwm->config.frequency_hz = note.frequency_hz;

    ESP_ERROR_CHECK(pwm_set_duty(buzzer_pwm, BUZZER_HALF_DUTY));
}

static void stop_player(pwm_t *buzzer_pwm)
{
    ESP_ERROR_CHECK(pwm_set_duty(buzzer_pwm, 0));

    playing = false;
    remaining_ticks = 0;
}

static void next_note(pwm_t *buzzer_pwm)
{
    ++current_note_index;

    if (current_note_index >= MELODY_LENGTH) {
        stop_player(buzzer_pwm);
        return;
    }

    start_note(buzzer_pwm, imperial_march[current_note_index]);
}

static void player_tick(pwm_t *buzzer_pwm)
{
    if (!playing) {
        return;
    }

    if (remaining_ticks > 0) {
        --remaining_ticks;
    }

    if (remaining_ticks == 0) {
        next_note(buzzer_pwm);
    }
}

static void player_timer_callback(void *arg)
{
    pwm_t *buzzer_pwm = static_cast<pwm_t *>(arg);

    player_tick(buzzer_pwm);

    if (!playing) {
        ESP_ERROR_CHECK(esp_timer_stop(player_timer));
    }
}

static void init_player_timer(pwm_t *buzzer_pwm)
{
    esp_timer_create_args_t timer_config = {};

    timer_config.callback = player_timer_callback;
    timer_config.arg = buzzer_pwm;
    timer_config.dispatch_method = ESP_TIMER_TASK;
    timer_config.name = "player_tick";

    ESP_ERROR_CHECK(
        esp_timer_create(&timer_config, &player_timer)
    );
}

static void start_player(pwm_t *buzzer_pwm)
{
    current_note_index = 0;
    playing = true;

    start_note(buzzer_pwm, imperial_march[current_note_index]);
}

static void init_buzzer_pwm(pwm_t *buzzer_pwm)
{
    pwm_config_t config = {};
    config.gpio = BUZZER_PIN;
    config.channel = LEDC_CHANNEL_0;
    config.timer = LEDC_TIMER_0;
    config.frequency_hz = NOTE_G4;
    config.resolution = BUZZER_RESOLUTION;
    config.duty = 0;
    config.inverted = false;

    ESP_ERROR_CHECK(pwm_init(buzzer_pwm, &config));
}

extern "C" void app_main(void)
{
    static pwm_t buzzer_pwm = {};

    init_buzzer_pwm(&buzzer_pwm);
    init_player_timer(&buzzer_pwm);

    start_player(&buzzer_pwm);

    // Start periodic updates: 50 ms = 50,000 microseconds.
    ESP_ERROR_CHECK(esp_timer_start_periodic(player_timer, static_cast<uint64_t>(TICK_MS) * 1000ULL));
}