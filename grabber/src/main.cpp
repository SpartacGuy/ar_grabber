#include <Arduino.h>
#include <Servo.h>

#define SERVO_PIN 12
#define PRESSURE_PIN 28

static const uint8_t UR5_IN_PIN  = 2;   // command from UR5
static const uint8_t UR5_OUT_PIN = 3;   // 50 ms success pulse back to UR5
static const uint8_t ERROR_PIN   = 6;   // 50 ms error pulse back to UR5
static const uint16_t PULSE_MS   = 50;

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

PulseState successPulseState = PulseState::Idle;
PulseState errorPulseState   = PulseState::Idle;

const int OPEN_POS = 30;
const int CLOSED_POS = 110;
const int STEP_TIME = 15;
const int CLOSE_TIMEOUT = 3000;
const int CONTACT_DELTA = 20;

int pos = OPEN_POS;
int baselinePressure = 0;
bool errorDetected = false;

// This stops immediate re-closing after an error.
// UR5 must release command LOW once before close is allowed again.
bool waitForReleaseAfterError = false;

unsigned long lastStepTime = 0;
unsigned long closeStartTime = 0;
unsigned long successPulseStartMs = 0;
unsigned long errorPulseStartMs = 0;

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

void requestSuccessPulseOnce() {
  if (successPulseState != PulseState::Idle) return;

  digitalWrite(UR5_OUT_PIN, HIGH);
  successPulseStartMs = millis();
  successPulseState = PulseState::High;
  Serial.println("Success pulse -> HIGH");
}

void requestErrorPulseOnce() {
  if (errorPulseState != PulseState::Idle) return;

  digitalWrite(ERROR_PIN, HIGH);
  errorPulseStartMs = millis();
  errorPulseState = PulseState::High;
  Serial.println("Error pulse -> HIGH");
}

void servicePulses() {
  unsigned long now = millis();

  if (successPulseState == PulseState::High) {
    if ((unsigned long)(now - successPulseStartMs) >= PULSE_MS) {
      digitalWrite(UR5_OUT_PIN, LOW);
      successPulseState = PulseState::Idle;
      Serial.println("Success pulse -> LOW");
    }
  }

  if (errorPulseState == PulseState::High) {
    if ((unsigned long)(now - errorPulseStartMs) >= PULSE_MS) {
      digitalWrite(ERROR_PIN, LOW);
      errorPulseState = PulseState::Idle;
      Serial.println("Error pulse -> LOW");
    }
  }
}

void initializeServo() {
  Serial.begin(9600);

  pinMode(PRESSURE_PIN, INPUT);
  pinMode(UR5_IN_PIN, INPUT_PULLDOWN);
  pinMode(UR5_OUT_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(UR5_OUT_PIN, LOW);
  digitalWrite(ERROR_PIN, LOW);

  myservo.attach(SERVO_PIN);
  myservo.write(OPEN_POS);

  pos = OPEN_POS;
  baselinePressure = analogRead(PRESSURE_PIN);
  errorDetected = false;
  waitForReleaseAfterError = false;

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

      // After an error, ignore HIGH until UR5 releases LOW once
      if (waitForReleaseAfterError) {
        if (!closeGripper) {
          waitForReleaseAfterError = false;
          Serial.println("Error latch cleared -> UR5 released LOW");
        }
        return false;
      }

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
        requestSuccessPulseOnce();
        return true;
      }

      if (pos >= CLOSED_POS || now - closeStartTime >= CLOSE_TIMEOUT) {
        errorDetected = true;
        waitForReleaseAfterError = true;
        requestErrorPulseOnce();
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

        if (errorDetected) {
          // finished recovery from error, now idle/open
          setState(OPEN);
        } else {
          setState(OPEN);
          requestSuccessPulseOnce();
        }
      }

      return false;

    case ERROR_STATE:
      // Immediately recover by opening once, but keep the latch active
      requestSuccessPulseOnce();
      setState(OPENING);
      
      return false;
  }

  return false;
}

void setup() {
  initializeServo();
}

void loop() {
  servicePulses();

  bool ur5State = digitalRead(UR5_IN_PIN);

  // HIGH from UR5 = close
  // LOW from UR5  = open
  bool closeCommandFromUR5 = ur5State;

  digitalWrite(LED_BUILTIN, ur5State ? HIGH : LOW);

  servoControl(closeCommandFromUR5, 0);
}