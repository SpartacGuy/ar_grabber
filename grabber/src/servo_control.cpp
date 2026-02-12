#include <Arduino.h>
#include <Servo.h>

#define PressurePin A7

Servo myservo;  // create Servo object to control a servo
// twelve Servo objects can be created on most boards

int pos = 0;    // variable to store the servo position
int oldPressure = 0;
int pressure = 0;
int currentPressure = 0;
int lastDebounceTime = 0;

bool gripped = false;
bool ungripped = false;

void setup() {
  Serial.begin(9600);
  myservo.attach(9);  // attaches the servo on pin 9 to the Servo object
  pinMode(PressurePin, INPUT);
}

void loop() {

  int unmappedPressure = analogRead(PressurePin);
  int mappedPressure = map(unmappedPressure, 0, 1023, 0, 255);
  int pressureDiff = mappedPressure - oldPressure;
  currentPressure = pressureDiff;
  currentPressure = (pressureDiff > 1 || pressureDiff < -1) ? pressureDiff : pressure;
  
  
  //Serial.println(currentPressure);

  
  if (millis() - lastDebounceTime >= 10) {

    if (currentPressure >= pressure + 3 && currentPressure < 100) {
      Serial.print("Object Gripped. Pressure Value: ");
      Serial.println(currentPressure);

      pressure = currentPressure;

      gripped = true;
      ungripped = false;
      //oldPressure = mappedPressure;
    } else if (currentPressure <= pressure - 3 && pressure > 1) {
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

  
  if (ungripped /* && pos <= 120 */) {
    pos = 120;
    myservo.write(pos);
    delay(15);
    // pos++;
  } else if (gripped /*&& pos >= 0 */) {
    pos = 90;
    myservo.write(pos);
    delay(15);
    // pos--;
  }

  Serial.print("Gripper Pos: ");
  Serial.println(pos);




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

