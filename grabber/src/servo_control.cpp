#include <Arduino.h>
#include <Servo.h>

#define PressurePin 28

Servo myservo;  // create Servo object to control a servo
// twelve Servo objects can be created on most boards

const int PRESSURE_THRESHOLD = 50; 

int pos = 0;    // variable to store the servo position
int oldPressure = 0;
int pressure = 0;  
int currentPressure = 0;
int lastDebounceTime = 0;
//int errorTimer = 0;

bool gripped = false;
bool ungripped = false;
bool errorDetected = false;

void initializeServo() {
  Serial.begin(9600);
  myservo.attach(22);  // attaches the servo on pin 29 to the Servo object
  pinMode(PressurePin, INPUT);
}

bool servoControl(bool closeGripper) {

  static int errorTimer = millis(); // Timer to track time since last grip/ungrip event
  static int closingTime = 0;

  int unmappedPressure = analogRead(PressurePin);
  int mappedPressure = map(unmappedPressure, 0, 1023, 0, 255);
  int pressureDiff = mappedPressure - oldPressure;
  currentPressure = pressureDiff;
  currentPressure = (pressureDiff > 1 || pressureDiff < -1) ? pressureDiff : pressure;
  
  
  //Serial.println(currentPressure);

  
  if (millis() - lastDebounceTime >= 10) {
    Serial.print(currentPressure);
    if (currentPressure >= pressure + PRESSURE_THRESHOLD /* && currentPressure < 100 */) {
      Serial.print("Object Gripped. Pressure Value: ");
      Serial.println(currentPressure);

      pressure = currentPressure;
    
      gripped = true;
      ungripped = false;
      
      closingTime = millis() - errorTimer;
      if (closingTime > 2000) { 
        Serial.println("Error: Object may be slipping or not fully gripped.");
        errorDetected = true;
      } else {
        errorDetected = false;
      }

      //oldPressure = mappedPressure;
    } else if (currentPressure <= pressure - PRESSURE_THRESHOLD && pressure > 1) {
      Serial.print("Object Ungripped. Pressure Value: ");
      Serial.println(currentPressure);

      pressure = currentPressure > 0 ? currentPressure : 0;

      gripped = false;
      ungripped = true;
      //delay(100);
      //oldPressure = mappedPressure;
    }
    lastDebounceTime = millis();
    oldPressure = mappedPressure;
  }

  // pressure = currentPressure;

  if (closeGripper && !errorDetected) {
    
        if (!gripped /* && pos <= 120 */) {
            pos = 120;
            myservo.write(pos);
            delay(15);
            // pos++;
        } else if (gripped /*&& pos >= 0 */) {
            pos = 90;
            myservo.write(pos);
            delay(15);
            errorTimer = millis(); // Reset error timer on successful grip
            return true;
          
            // delay(5000); // delay five seconds to simulate transporting the object
            // pos = 0; // open the gripper to the open position after transporting the object
            // myservo.write(pos);
            // delay(2000);
            // pos = 90;
            // myservo.write(pos);
            // // gripped = false;
            // // pos--;
        }
    } else if (!closeGripper || errorDetected) { 
        pos = 60;
        myservo.write(pos);
        if (!closeGripper) {
            delay(closingTime); // delay five seconds to simulate transporting the object
            pos = 90;
            myservo.write(pos);
            errorTimer = millis();
          return true;
        } else {
            delay(2000);
            pos = 90;
            myservo.write(pos);
            errorTimer = millis();
            errorDetected = false; // Reset error state after attempting to ungrip
        }
        
    }

    return false;
  // Serial.print("Gripper Pos: ");
  // Serial.println(pos);




    // for (pos = 0; pos <= 120; pos += 1) { // goes from 0 degrees to 180 degrees
    //   // in steps of 1 degree
    //   myservo.write(pos);              // tell servo to go to position in variable 'pos'
    //   delay(15);                       // waits 15 ms for the servo to reach the position
    // }
 
    // for (pos = 120; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    //   myservo.write(pos);              // tell servo to go to position in variable 'pos'
    //   delay(15);                       // waits 15 ms for the servo to reach the position
    // }


  delay(100);
}
