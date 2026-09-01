#include <Arduino.h>

class Config {
  public:
    static constexpr uint8_t led1Pin = 15;
    static constexpr uint8_t led2Pin = 16;
    static constexpr uint8_t led3Pin = 17;
    static constexpr uint32_t led1Interval = 200;
    static constexpr uint32_t led2Interval = 500;
    static constexpr uint32_t led3Interval = 1000;
};

enum class LedState {
  LED_ON,
  LED_OFF
};

class Led {
  public:
    Led(int pin, uint32_t interval) : pin_(pin), interval_(interval) {}

  void init() {
    pinMode(pin_, OUTPUT);
  }

  void update() {

    if(millis() - lastToggle_ >= interval_) {
      
      lastToggle_ = millis();

      switch(state_) {
        case LedState::LED_OFF:
          digitalWrite(pin_, HIGH);
          state_ = LedState::LED_ON;
          break;
        case LedState::LED_ON:
          digitalWrite(pin_, LOW);
          state_ = LedState::LED_OFF;
          break;
      }
    }

  }

  private:
    const uint8_t pin_;
    LedState state_{LedState::LED_OFF};
    const uint32_t interval_;
    uint32_t lastToggle_{0};
};

Led led1(Config::led1Pin, Config::led1Interval);
Led led2(Config::led2Pin, Config::led2Interval);
Led led3(Config::led3Pin, Config::led3Interval);

void setup() {
  Serial.begin(115200);
  led1.init();
  led2.init();
  led3.init();
}

void loop() {
  led1.update();
  led2.update();
  led3.update();
}