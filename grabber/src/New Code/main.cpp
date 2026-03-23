#include <Arduino.h>
#include <Servo.h>

#define PressurePin 28
#define ServoPin 12
#define LimitPin 29

Servo myservo;  // create Servo object to control a servo

// Make the code for a gripper that closes until it detects pressure from a Force sensitive resistor when receiving a signal to close, if the limit sensor is activated before the normal pressure sensor then the gripper stops closing and sends an error signal, if its closed when receiving the signal just open it completely, once the gripper is closed and the signal sent then read the pressure sensor continuously to check for any changes that would indicate an object slipping and send an error signal if that is the case.
#include <Arduino.h>
#include <Servo.h>

#define PressurePin 28
#define ServoPin 12
//#define LimitPin 29
#define ErrorPin 6
#define RelayPin 3
#define OptocouplerPin 2

Servo myservo;
const int PRESSURE_THRESHOLD = 400;
const int PRESSURE_TOLERANCE = 50;
int lastPressureReading = 0;
bool gripperClosed = false;
int servoPosition = 0;


void setup() {
    Serial.begin(115200);
    pinMode(PressurePin, INPUT);
    pinMode(LimitPin, INPUT_PULLUP);
    pinMode(ErrorPin, OUTPUT);
    pinMode(RelayPin, OUTPUT);
    pinMode(OptocouplerPin, INPUT);
    
    myservo.attach(ServoPin);
    myservo.write(0);
    
    digitalWrite(ErrorPin, LOW);
    digitalWrite(RelayPin, LOW);
}

void sendError() {
    // Send an error signal (e.g., blink an LED, send a message, etc.)
    digitalWrite(ErrorPin, HIGH);
    delay(1000);
    digitalWrite(ErrorPin, LOW);
}

void closeGripper() {
    // Close the gripper until pressure is detected or limit is reached
    Serial.println("Closing gripper...");
    //digitalWrite(CloseRelayPin, HIGH);
    while (digitalRead(OptocouplerPin) == HIGH) {
        int pressure = analogRead(PressurePin);
        
        // // Check limit sensor first
        // if (analogRead(LimitPin) > PRESSURE_THRESHOLD) {
        //     digitalWrite(CloseRelayPin, HIGH);
        //     Serial.println("ERROR: Limit sensor triggered before pressure detected!");
        //     sendError();
        //     digitalWrite(CloseRelayPin, LOW);
        //     return;
        // }
        
        // Check pressure sensor
        if (pressure > PRESSURE_THRESHOLD) {
            digitalWrite(RelayPin, HIGH);
            gripperClosed = true;
            lastPressureReading = pressure;
            if (servoPosition >= 170) {
                Serial.println("ERROR: Servo limit reached without detecting pressure!");
                sendError();
                
            } else {
                Serial.println("Gripper closed successfully.");
            } 
            digitalWrite(RelayPin, LOW);   
            return;
        }
        servoPosition++;
        myservo.write(servoPosition);

        delay(15);
    }
}

void openGripper() {
    // Open the gripper completely
    Serial.println("Opening gripper...");
    servoPosition = 120;
    digitalWrite(RelayPin, HIGH);
    delay(1000);
    digitalWrite(RelayPin, LOW);
    gripperClosed = false;
}

void monitorPressure() {
    // Continuously monitor pressure for slippage
    Serial.println("Monitoring pressure for slippage...");
    
    while (gripperClosed) {
        // Check if optocoupler signal indicates to open the gripper
        if (digitalRead(OptocouplerPin) == HIGH) {
            openGripper();
            return;
        }
        
        int currentPressure = analogRead(PressurePin);
        
        if ((lastPressureReading - currentPressure) > PRESSURE_TOLERANCE) {
            // Pressure drop detected, indicating possible slippage
            sendError();
            Serial.print("ERROR: Pressure drop detected! Last: ");
            Serial.print(lastPressureReading);
            Serial.print(" Current: ");
            Serial.println(currentPressure);
            openGripper();
            return;
        }
        
        lastPressureReading = currentPressure;
        delay(200);
    }
}

void loop() {
    // Check for optocoupler signal to toggle gripper state
    if (digitalRead(OptocouplerPin) == HIGH) {
        if (gripperClosed) {
            // If gripper is closed, open it
            openGripper();
        } else {
            // If gripper is open, attempt to close it
            closeGripper();
        }
        delay(500);
    } else {
        if (gripperClosed) {
            // Monitor pressure for slippage
            Serial.print("Gripper is closed. Last pressure reading: ");
            Serial.println(lastPressureReading);
            monitorPressure();
        }
    }
}



