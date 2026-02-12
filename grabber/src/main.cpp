#include <Arduino.h>
#include "servo_control.h"

// Pins
static const uint8_t UR5_IN_PIN  = 2;   // input from UR5
static const uint8_t UR5_OUT_PIN = 3;   // output to UR5/relay input

static const uint16_t PULSE_MS = 50;

// Pulse state machine
enum class PulseState : uint8_t { Idle, High };
static PulseState pulseState = PulseState::Idle;
static uint32_t pulseStartMs = 0;

static void requestPositivePulseOnce()
{
  // If a pulse is already active, ignore new requests (prevents double-firing).
  // If you prefer "restart pulse timing on each event", I can show that variant too.
  if (pulseState != PulseState::Idle) return;

  digitalWrite(UR5_OUT_PIN, HIGH);
  pulseStartMs = millis();
  pulseState = PulseState::High;
}

static void servicePulse()
{
  if (pulseState == PulseState::High)
  {
    // Use millis() time comparison safe for rollover
    if ((uint32_t)(millis() - pulseStartMs) >= PULSE_MS)
    {
      digitalWrite(UR5_OUT_PIN, LOW);
      pulseState = PulseState::Idle;
    }
  }
}

// Edge detection
static bool lastInState = false;

bool closeGripper = false;
bool gripperClosed = false;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  // Use INPUT_PULLDOWN if the line could float. If UR5 output drives strongly, INPUT is OK.
  pinMode(UR5_IN_PIN, INPUT_PULLDOWN);

  pinMode(UR5_OUT_PIN, OUTPUT);
  digitalWrite(UR5_OUT_PIN, LOW);

  lastInState = digitalRead(UR5_IN_PIN);
  digitalWrite(LED_BUILTIN, lastInState ? HIGH : LOW); // optional: reflect state at boot

  // Initialize servo and pressure sensor
  initializeServo();
}

void loop()
{
  servicePulse();

  bool inState = digitalRead(UR5_IN_PIN);

  // React only on edges
  if (inState != lastInState)
  {
    if (inState)
    {
      // Rising edge: LED ON (called once per rising edge)
      digitalWrite(LED_BUILTIN, HIGH);

      closeGripper = !closeGripper; // Toggle gripper state on each rising edge
      Serial.print(closeGripper);
      while (!servoControl(closeGripper)) { // Wait until the servo control indicates the gripper has reached the desired state delay(10); // Small delay to prevent busy-waiting }
        Serial.print("Gripper "); Serial.println(closeGripper ? "Closing" : "Opening"); 
      }

      Serial.print("Gripper state:"); Serial.println(closeGripper ? "Closed" : "Open"); 

      // Send positive pulse once
      requestPositivePulseOnce();
    }
    else
    {
      // Falling edge: LED OFF (called once per falling edge)
      digitalWrite(LED_BUILTIN, LOW);
      if (gripperClosed) {
        closeGripper = false;
        gripperClosed = servoControl(closeGripper);
      }
      // Send positive pulse once
      requestPositivePulseOnce();
    }

    lastInState = inState;
  }

}


