#include <Arduino.h>

volatile bool armed = false; // Флаг для избегания дребезга
volatile bool measured = false; 
volatile unsigned long closeTime = 0;
unsigned long openTime = 0;

class Config {
  public:
    static constexpr int relayInPin = 15;
    static constexpr int relayOutPin = 16;
    static constexpr int interval = 500;
    static constexpr int operateTimes = 10;
};

enum class RelayState {
  ON,
  OFF,
};

void IRAM_ATTR reaction_relay() {
  if(armed) {
    closeTime = micros();
    armed = false;
    measured = true;
  }
}

class Relay {
  public:
    Relay(int inPin, int outPin) : inPin_(inPin), outPin_(outPin) {}

    void init() {
      pinMode(inPin_, OUTPUT);
      pinMode(outPin_, INPUT_PULLUP);

      attachInterrupt(
        digitalPinToInterrupt(outPin_),
        reaction_relay,
        FALLING 
      );
    }

    void set(RelayState state) {
      state_ = state;

      digitalWrite(
        inPin_,
        state_ == RelayState::ON ? HIGH : LOW
      );

      if (state_ == RelayState::ON) {
        openTime = micros();
        measured = false;
        armed = true;     
      } else {
        armed = false;
      }
    }

    void toggle() {
      set(state_ == RelayState::ON ? RelayState::OFF : RelayState::ON);
    }
  private:
    const int inPin_;
    const int outPin_;
    RelayState state_{RelayState::OFF};
};

Relay relay(Config::relayInPin, Config::relayOutPin);

void setup() {
  Serial.begin(115200);
  relay.init();
}

void loop() {
  static unsigned long lastTick = 0;
  static unsigned long operates[Config::operateTimes]; // Массив измерений времени
  static int index = 0; // Счетчик кол-ва измерений времени

  if(millis() - lastTick >= Config::interval) {
    lastTick = millis();
    relay.toggle();
  }

  // Срабатывает когда время срабатывания реле измерено и массив еще не заполнен
  if(measured && index < Config::operateTimes) {
    measured = false;
    unsigned long operateTime = closeTime - openTime;
    operates[index] = operateTime;
    Serial.printf("Operate time: %lu us\n", operateTime);
    
    if(index == Config::operateTimes - 1) {
      unsigned long totalSum = 0;
      unsigned long averageValue;
      
      for(int i = 0; i < Config::operateTimes; i++) {
        totalSum += operates[i];
      }
      
      averageValue = totalSum / Config::operateTimes;
      
      Serial.printf("Average value: %lu us\n", averageValue);
    }

    index++;
  }
}