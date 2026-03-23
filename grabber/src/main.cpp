#include <Arduino.h>
#include <Servo.h>

#define SERVO_PIN 12
#define PRESSURE_PIN 28

static const uint8_t UR5_IN_PIN  = 2;   // command from UR5
static const uint8_t UR5_OUT_PIN = 3;   // 50 ms pulse back to UR5

static const uint16_t PULSE_MS = 50;

Servo myservo;

enum State {
  OPEN,
  CLOSING,
  CLOSED,
  OPENING,
  ERROR_STATE
};

enum class PulseState : uint8_t {
  Idle,
  High
};

State state = OPEN;
PulseState pulseState = PulseState::Idle;

const int OPEN_POS = 30;
const int CLOSED_POS = 110;
const int STEP_TIME = 15;
const int CLOSE_TIMEOUT = 1800;
const int CONTACT_DELTA = 20;

int pos = OPEN_POS;
int baselinePressure = 0;
bool errorDetected = false;

unsigned long lastStepTime = 0;
unsigned long closeStartTime = 0;
unsigned long pulseStartMs = 0;

void setState(State newState) {
  if (state != newState) {
    state = newState;

    switch (state) {
      case OPEN:
        Serial.println("State -> OPEN");
        break;
      case CLOSING:
        Serial.println("State -> CLOSING");
        break;
      case CLOSED:
        Serial.println("State -> CLOSED");
        break;
      case OPENING:
        Serial.println("State -> OPENING");
        break;
      case ERROR_STATE:
        Serial.println("State -> ERROR_STATE");
        break;
    }
  }
}

void requestPositivePulseOnce() {
  if (pulseState != PulseState::Idle) return;

  digitalWrite(UR5_OUT_PIN, HIGH);
  pulseStartMs = millis();
  pulseState = PulseState::High;
}

void servicePulse() {
  if (pulseState == PulseState::High) {
    if ((unsigned long)(millis() - pulseStartMs) >= PULSE_MS) {
      digitalWrite(UR5_OUT_PIN, LOW);
      pulseState = PulseState::Idle;
    }
  }
}

void initializeServo() {
  Serial.begin(9600);

  pinMode(PRESSURE_PIN, INPUT);
  pinMode(UR5_IN_PIN, INPUT_PULLDOWN);
  pinMode(UR5_OUT_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(UR5_OUT_PIN, LOW);

  myservo.attach(SERVO_PIN);
  myservo.write(OPEN_POS);

  pos = OPEN_POS;
  baselinePressure = analogRead(PRESSURE_PIN);
  errorDetected = false;

  setState(OPEN);
}

bool servoControl(bool closeGripper, int errorTimer) {
  (void)errorTimer;

  int pressure = analogRead(PRESSURE_PIN);
  unsigned long now = millis();

  switch (state) {
    case OPEN:
      errorDetected = false;
      baselinePressure = pressure;

      if (closeGripper) {
        closeStartTime = now;
        setState(CLOSING);
      }
      return false;

    case CLOSING:
      if (now - lastStepTime >= STEP_TIME) {
        lastStepTime = now;

        if (pos < CLOSED_POS) {
          pos++;
          myservo.write(pos);
        }
      }

      if (pressure > baselinePressure + CONTACT_DELTA) {
        errorDetected = false;
        setState(CLOSED);
        requestPositivePulseOnce();
        return true;
      }

      if (pos >= CLOSED_POS || now - closeStartTime >= CLOSE_TIMEOUT) {
        errorDetected = true;
        setState(ERROR_STATE);
        return true;
      }

      return false;

    case CLOSED:
      if (!closeGripper) {
        setState(OPENING);
      }
      return true;

    case OPENING:
      if (now - lastStepTime >= STEP_TIME) {
        lastStepTime = now;

        if (pos > OPEN_POS) {
          pos--;
          myservo.write(pos);
        }
      }

      if (pos <= OPEN_POS) {
        pos = OPEN_POS;
        myservo.write(pos);
        baselinePressure = analogRead(PRESSURE_PIN);
        errorDetected = false;
        setState(OPEN);
        requestPositivePulseOnce();
      }

      return false;

    case ERROR_STATE:
      if (!closeGripper) {
        setState(OPENING);
      }
      return true;
  }

  return false;
}

bool ErrorDetected() {
  return errorDetected;
}

void setup() {
  initializeServo();
}

void loop() {
  servicePulse();

  bool ur5State = digitalRead(UR5_IN_PIN);

  // HIGH from UR5 = close
  // LOW from UR5  = open
  bool closeCommandFromUR5 = ur5State;

  digitalWrite(LED_BUILTIN, ur5State ? HIGH : LOW);

  servoControl(closeCommandFromUR5, 0);
}