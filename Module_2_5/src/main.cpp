#include <Arduino.h>
#include <esp_task_wdt.h>

volatile bool timerFired = false;
hw_timer_t *timer = NULL;

constexpr uint32_t WDT_TIMEOUT_SECONDS = 10;

void IRAM_ATTR onTimer() {
  timerFired = true;
}

enum class FanState {
  ON,
  OFF,
};

class Fan {
  public:
    Fan(uint8_t fanPin, uint8_t ledOnPin, uint8_t ledOffPin) : fanPin_(fanPin), ledOnPin_(ledOnPin), ledOffPin_(ledOffPin) {}

    void init() {
      pinMode(ledOnPin_, OUTPUT);
      pinMode(ledOffPin_, OUTPUT);
      pinMode(fanPin_, OUTPUT);
      digitalWrite(ledOnPin_, LOW);
      digitalWrite(ledOffPin_, LOW);
    }

    void turnOn() {
      if(!timer) {
        return;
      }
      
      state_ = FanState::ON;

      digitalWrite(ledOnPin_, HIGH);
      digitalWrite(ledOffPin_, LOW);
      digitalWrite(fanPin_, HIGH);
      
      timerFired = false;

      timerWrite(timer, 0);
      timerAlarm(timer, 3000000, false, 0);

      Serial.println("Turned on for 3 sec");
    }

    void turnOff() {
      if(!timer) {
        return;
      }
      
      state_ = FanState::OFF;

      digitalWrite(ledOnPin_, LOW);
      digitalWrite(ledOffPin_, HIGH);
      digitalWrite(fanPin_, LOW);
      
      timerFired = false;

      timerWrite(timer, 0);
      timerAlarm(timer, 5000000, false, 0);

      Serial.println("Turned off for 5 sec");
    }

    FanState getState() {
      return state_;
    }


    private:
    FanState state_;

    uint8_t fanPin_;
    uint8_t ledOnPin_;
    uint8_t ledOffPin_;
};

Fan fan(15, 17, 18);

void setup() {
  Serial.begin(115200);

  esp_task_wdt_config_t wdtConfig = {
        .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
  };

  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL);

  // set timer frequency to 1Mhz
  timer = timerBegin(1000000);

  timerAttachInterrupt(timer, &onTimer);

  fan.init();
  fan.turnOn();
}

void loop() {

  if(timerFired) {
    timerFired = false;

    switch(fan.getState()) {

      case FanState::OFF:
        fan.turnOn();
        break;
      case FanState::ON:
        fan.turnOff();
        break;

    }

  }

  esp_task_wdt_reset();

  delay(10);
}