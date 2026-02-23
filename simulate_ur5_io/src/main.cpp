#include <Arduino.h>

// Pin Definitions
const int PIN_BUTTON = 2;   
const int PIN_OUTPUT = 21;  
const int PIN_INPUT  = 3;   

// Timing Constants
const unsigned long INTERVAL_MS = 2000; 
int debounceTime = 0;
bool buttonState = false;

// State Machine States
enum SystemState {
  STATE_IDLE,           
  STATE_WAIT_1,         
  STATE_ACTION_1,       
  STATE_WAIT_2,         
  STATE_ACTION_2,       
  STATE_WAIT_3          
};

// Global Variables
SystemState currentState = STATE_IDLE;
unsigned long stateStartTime = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("System Initialized");

  // Input Setup
  pinMode(PIN_BUTTON, INPUT_PULLUP); 
  pinMode(PIN_INPUT, INPUT); 

  // Output Setup
  pinMode(PIN_OUTPUT, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT); 

  // Initial State
  digitalWrite(PIN_OUTPUT, LOW); 
  digitalWrite(LED_BUILTIN, LOW); 
}

void loop() {
  unsigned long currentMillis = millis();

  switch (currentState) {
    
    // 1. Waiting for Button Press
    case STATE_IDLE:
    // Debounce button  
    bool buttonPressed = (digitalRead(PIN_BUTTON) == HIGH);
 
    if (millis() - debounceTime > 50) {
      if (buttonPressed != buttonState) {
        buttonState = buttonPressed;
      }
      debounceTime = millis(); // Reset debounce time
    }
      if (buttonState == HIGH) {
        Serial.println("Button Pressed. Starting Sequence.");
        stateStartTime = currentMillis; 
        currentState = STATE_WAIT_1;
      }
      break;

    // 2. Wait 2 seconds
    case STATE_WAIT_1:
      if (currentMillis - stateStartTime >= INTERVAL_MS) {
        Serial.println("Wait 1 Complete.");
        currentState = STATE_ACTION_1;
      }
      break;

    // 3. Write HIGH on Pin 21 AND Builtin LED, Expect HIGH on Pin 3
    case STATE_ACTION_1:
      digitalWrite(PIN_OUTPUT, HIGH);
      digitalWrite(LED_BUILTIN, HIGH); 
      
      if (digitalRead(PIN_INPUT) == HIGH) {
        Serial.println("Pin 3 Detected HIGH. Moving to Wait 2.");
        stateStartTime = currentMillis; 
        currentState = STATE_WAIT_2;
      }
      break;

    // 4. Wait 2 seconds again
    case STATE_WAIT_2:
      if (currentMillis - stateStartTime >= INTERVAL_MS) {
        Serial.println("Wait 2 Complete.");
        currentState = STATE_ACTION_2;
      }
      break;

    // 5. Write LOW on Pin 21 AND Builtin LED, Expect HIGH again on Pin 3
    case STATE_ACTION_2:
      digitalWrite(PIN_OUTPUT, LOW);
      digitalWrite(LED_BUILTIN, LOW);

      if (digitalRead(PIN_INPUT) == HIGH) {
        Serial.println("Pin 3 Detected HIGH (2nd time). Moving to Wait 3.");
        stateStartTime = currentMillis; 
        currentState = STATE_WAIT_3;
      }
      break;

    // 6. Wait two seconds again
    case STATE_WAIT_3:
      if (currentMillis - stateStartTime >= INTERVAL_MS) {
        Serial.println("Sequence Complete. Returning to IDLE.");
        
        // Ensure everything is reset
        digitalWrite(PIN_OUTPUT, LOW); 
        digitalWrite(LED_BUILTIN, LOW);

        currentState = STATE_IDLE;
      }
      break;
  }
}