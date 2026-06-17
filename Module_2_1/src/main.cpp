#include <Arduino.h>

class Config {
  public:
    static constexpr int ledPin = LED_BUILTIN;
    static constexpr int interval = 1000;
};

enum class LEDState {
  ON,
  OFF,
};

class Led {
  public:
    Led(int pin) : pin_(pin) {}

    void init() {
      pinMode(pin_, INPUT_PULLUP);
    }

    void set(LEDState state) {
      state_ = state;

      digitalWrite(
        pin_,
        state_ == LEDState::ON ? HIGH : LOW
      );
    }

    void toggle() {
      set(state_ == LEDState::ON ? LEDState::OFF : LEDState::ON);
    }

    LEDState getState() const {
      return state_;
    }

  private:
    const int pin_;
    LEDState state_{LEDState::OFF};
};

Led led(Config::ledPin);

void setup() {
  Serial.begin(115200);
  led.init();
}

void loop() {
  static int iterationCount = 0;
  static int startTime;
  static int lastBlink = 0;

  if(iterationCount == 0) {
    startTime = micros();
  }

  iterationCount++;
  
  if(iterationCount == 1000) {

    iterationCount = 0;
    unsigned long elapsedMs = micros() - startTime;

    Serial.print("Time spent for 1000 iterations: ");
    Serial.print(elapsedMs);
    Serial.print(" microseconds\n");
  }

  if(millis() - lastBlink >= Config::interval) {
    lastBlink = millis();
    led.toggle();
  }

}