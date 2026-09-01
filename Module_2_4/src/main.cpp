/////////////////////////////////////////////////////////////////////////////////////////////////
// Task 1
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

#define BUTTON_PIN 15

volatile uint32_t counter = 0;
volatile bool gpioEventFlag = false;

void IRAM_ATTR interruptFunc() {
  counter++;
  gpioEventFlag = true; 
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), interruptFunc, RISING); 
}

void loop() {
  if(gpioEventFlag) {
    Serial.println(counter);
    gpioEventFlag = false;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Task 2
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

volatile uint32_t counter = 0;
volatile bool gpioEventFlag = false;

class Config {
  public:
    static constexpr uint8_t buttonPin = 15;
    static constexpr uint32_t interval = 50;
};

void processCounter() {
  counter++;
  Serial.println(counter);
}

void IRAM_ATTR interruptFunc() {
  gpioEventFlag = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(Config::buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Config::buttonPin), interruptFunc, FALLING);
}

void loop() {
  static uint32_t lastTick = 0;

  if(gpioEventFlag) {
    if(millis() - lastTick >= Config::interval) {
      lastTick = millis();
      gpioEventFlag = false;
      processCounter();
    } else {
      gpioEventFlag = false;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Task 3
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

volatile bool gpioEventFlag = false;

class Config {
  public:
    static constexpr uint8_t buttonPin = 15;
};

enum class ButtonState {
  PRESSED,
  RELEASED,
};

void IRAM_ATTR interruptFunc() {
  gpioEventFlag = true;
}

class Button {
  public:
    Button(uint8_t pin) : pin_(pin) {}

    void handleEvent() {

      bool isPressed = digitalRead(pin_) == HIGH;

      // If button pressed right now and the state is RELEASED -> count this pressing
      if(isPressed && state_ == ButtonState::RELEASED) {
        state_ = ButtonState::PRESSED;
        counter_++;
        Serial.println(counter_);
      // If button not pressed right now and the state is PRESSED -> change state to RELEASED
      } else if(!isPressed && state_ == ButtonState::PRESSED) {
        state_ = ButtonState::RELEASED;
      }
      // Ignoring other scenarious - bounce
    }

  private:
    uint8_t pin_;
    ButtonState state_{ButtonState::RELEASED};
    uint32_t counter_ = 0;
};

Button button(Config::buttonPin);

void setup() {
  Serial.begin(115200);
  pinMode(Config::buttonPin, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(Config::buttonPin), interruptFunc, CHANGE);
}

void loop() {

  if(gpioEventFlag) {
    gpioEventFlag = false;
    button.handleEvent();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Task 4
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>

class Config {
  public:
    static constexpr uint8_t buttonPin = 15;
    static constexpr uint32_t interval = 10;
};

enum class ButtonState {
  PRESSED,
  RELEASED,
};

class Button {
  public:
    Button(uint8_t pin) : pin_(pin) {}

    void checkButton() {
      bool isPressed = digitalRead(pin_) == HIGH;

      // If button pressed right now and the state is RELEASED -> count this pressing
      if(isPressed && state_ == ButtonState::RELEASED) {
        state_ = ButtonState::PRESSED;
        counter_++;
        Serial.println(counter_);
      // If button not pressed right now and the state is PRESSED -> change state to RELEASED
      } else if(!isPressed && state_ == ButtonState::PRESSED) {
        state_ = ButtonState::RELEASED;
      }
    }

  private:
    uint8_t pin_;
    ButtonState state_{ButtonState::RELEASED};
    uint32_t counter_ = 0;
};

Button button(Config::buttonPin);

void setup() {
  Serial.begin(115200);
  pinMode(Config::buttonPin, INPUT_PULLDOWN);
}

void loop() {
  static uint32_t lastTick = 0;

  if(millis() - lastTick >= Config::interval) {
    lastTick = millis();
    button.checkButton();
  }
}