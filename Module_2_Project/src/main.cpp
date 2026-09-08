#include <Arduino.h>

#define GREEN_LED 15
#define YELLOW_LED 16
#define RED_LED 17

volatile bool timerFired = false;
hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
  timerFired = true;
}

enum class TrafficLightState {
  GREEN,
  GREEN_BLINKING,
  YELLOW,
  RED,
  RED_AND_YELLOW,
};

void startTimer(uint32_t durationMs, bool repeating) {
  if(!timer) {
    return;
  }

  timerFired = false;

  timerWrite(timer, 0);
  timerAlarm(timer, durationMs * 1000, repeating, 0);
}

class TrafficLight {
  public:
    TrafficLight() : state_(TrafficLightState::GREEN), blinkCounter_(0){}

    void update() {

      // Blinking phase for GREEN_BLINKING state
      if(state_ == TrafficLightState::GREEN_BLINKING) {
        digitalWrite(GREEN_LED, !digitalRead(GREEN_LED));

        blinkCounter_++;

        // 6 * 500 ms = 3 seconds
        if (blinkCounter_ >= 6) {
          blinkCounter_ = 0;

          state_ = getNextState();

          applyState();
        }

        return;
      }

      state_ = getNextState();
      applyState();
    }

    TrafficLightState getNextState() {
      switch(state_) {
        case TrafficLightState::GREEN:
          return TrafficLightState::GREEN_BLINKING;

        case TrafficLightState::GREEN_BLINKING:
          return TrafficLightState::YELLOW;

        case TrafficLightState::YELLOW:
          return TrafficLightState::RED;

        case TrafficLightState::RED:
          return TrafficLightState::RED_AND_YELLOW;

        case TrafficLightState::RED_AND_YELLOW:
          return TrafficLightState::GREEN;

        default:
          return TrafficLightState::GREEN;
      }
    }

    void applyState() {
      switch(state_) {
        case TrafficLightState::GREEN:

          digitalWrite(GREEN_LED, HIGH);
          digitalWrite(YELLOW_LED, LOW);
          digitalWrite(RED_LED, LOW);

          startTimer(5000, false);

          Serial.println("Green light for 5 sec");
          
          break;
        
        case TrafficLightState::GREEN_BLINKING:

          digitalWrite(GREEN_LED, LOW);
          digitalWrite(YELLOW_LED, LOW);
          digitalWrite(RED_LED, LOW);

          blinkCounter_ = 0;
  
          startTimer(500, true);
          
          Serial.println("Green light blinking for 3 sec");
          
          break;

        case TrafficLightState::YELLOW:

          digitalWrite(GREEN_LED, LOW);
          digitalWrite(YELLOW_LED, HIGH);
          digitalWrite(RED_LED, LOW);

          startTimer(2000, false);
          
          Serial.println("Yellow light for 2 sec");
          
          break;

        case TrafficLightState::RED:

          digitalWrite(GREEN_LED, LOW);
          digitalWrite(YELLOW_LED, LOW);
          digitalWrite(RED_LED, HIGH);

          startTimer(5000, false);
          
          Serial.println("RED light for 5 sec");
          
          break;

        case TrafficLightState::RED_AND_YELLOW:

          digitalWrite(GREEN_LED, LOW);
          digitalWrite(YELLOW_LED, HIGH);
          digitalWrite(RED_LED, HIGH);

          startTimer(2000, false);
          
          Serial.println("Red and Yellow light for 5 sec");
          
          break;
      }
    }

    TrafficLightState getState() {
      return state_;
    }

  private:
    TrafficLightState state_;
    uint8_t blinkCounter_;
};

TrafficLight trafficLight;

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  timer = timerBegin(1000000);
  timerAttachInterrupt(timer, &onTimer);

  trafficLight.applyState();
}

void loop() {

  if(timerFired) {
    timerFired = false;
    trafficLight.update();
  }

  delay(10);
}