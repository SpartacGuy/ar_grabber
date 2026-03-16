#include <Arduino.h>
#include <Servo.h>

#define PressurePin 28

Servo myservo;  // create Servo object to control a servo
// twelve Servo objects can be created on most boards

const int PRESSURE_THRESHOLD = 20; 

int pos = 0;    // variable to store the servo position
int oldPressure = 0;
int pressure = 0;  
int currentPressure = 0;
int baselinePressure = 0; // Baseline pressure when an object is gripped
int lastDebounceTime = 0;
//int errorTimer = 0;

int mappedPressure = 0;

bool gripped = false;
bool ungripped = false;
bool errorDetected = false;

int errorTime = 0;


void pressureControl();

void initializeServo() {
  Serial.begin(9600);
  myservo.attach(22);  // attaches the servo on pin 29 to the Servo object
  pinMode(PressurePin, INPUT);
}

bool servoControl(bool closeGripper, int errorTimer) {

  

  // // static int errorTimer = millis(); // Timer to track time since last grip/ungrip event
  static int closingTime = 0;
  // errorTimer = errorDetected ? millis() : errorTimer; 


  errorTimer = errorDetected ? millis() : errorTimer; 

  int unmappedPressure = analogRead(PressurePin);
  mappedPressure = map(unmappedPressure, 0, 1023, 0, 255);
  int pressureDiff = mappedPressure - oldPressure;
  currentPressure = pressureDiff;
  currentPressure = (pressureDiff > 1 || pressureDiff < -1) ? pressureDiff : pressure;
  
  
  //Serial.println(currentPressure);

  
  if (millis() - lastDebounceTime >= 10) {
    //Serial.print(currentPressure);
    if (closeGripper && currentPressure >= pressure + PRESSURE_THRESHOLD /* && currentPressure < 100 */) {
      Serial.print("Object Gripped. Pressure Value: ");
      Serial.println(currentPressure);

      pressure = currentPressure;
      baselinePressure = mappedPressure; // Set baseline pressure when an object is gripped
    
      gripped = true;
      ungripped = false;
      
      closingTime = millis() - errorTimer;
      Serial.print("Closing Time: ");
      Serial.println(closingTime);
      if (closingTime > 800) { 
        //Serial.println("Error: Object may be slipping or not fully gripped.");
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



  if (closeGripper) { 
      if (!gripped /* && pos <= 120 */) { // Stop the servo if it takes long to close
        if (millis() - errorTimer > 1000) {
          pos = 90; 
          myservo.write(pos);
          errorDetected = true;
          return true;
          //gripped = true;
        } else {
          pos = 120;
          myservo.write(pos);
          delay(15);
        }
        // pos++;
      } else if (gripped /*&& pos >= 0 */) {
        pos = 90;
        myservo.write(pos);
        delay(15);
        // if (errorDetected) {
        //   //Serial.println("Error detected during gripping. Attempting to re-grip...");
        //   gripped = false; // Reset grip state to allow re-gripping
        //   pos = 60; // Open the gripper to attempt a new grip
        //   myservo.write(pos);
        //   delay(800); // Wait before attempting to grip again
        //   pos = 90; // Attempt to grip again
        //   myservo.write(pos);
        //   //return false; // Indicate that the desired state has not been reached yet
        // } 
        return true;
      }
      
    } else {
      pos = 60;
      myservo.write(pos);
      delay((closingTime > 0 && closingTime <= 1000) ? closingTime : 1000); 
      pos = 90;
      myservo.write(pos);
      errorDetected = false; // Clear error state on ungrip
      return false;
  } 


    return false;

  delay(100);
}

void checkPressure(){
  int pres = analogRead(PressurePin);
  Serial.print("Pressure: ");
  Serial.println(pres);
  int mappedPres = map(pres, 0, 1023, 0, 255);
    if (mappedPres <= baselinePressure - 10) {
      errorDetected = true; // Set error when a significant drop in pressure is detected while gripped
      Serial.println("Error: Object may have slipped or been dropped.");
    } 
}


bool ErrorDetected() {
  if (gripped) {
    checkPressure();
  }
  //checkPressure();
  // Serial.print("Error Detected:");
  // Serial.println(errorDetected);
  return errorDetected;
}