#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// ===== Pins =====
const int sensorPin = 2;      // Water level sensor
const int servoPin  = 6;      // Servo motor
const int stepPin1  = 9;      // Stepper motor pins (recommended order)
const int stepPin2  = 11;
const int stepPin3  = 10;
const int stepPin4  = 12;

// ===== Components =====
LiquidCrystal_I2C lcd(0x27,16,2); // LCD
Servo myServo;

// ===== Variables =====
enum SystemState {READY, WATER_CONFIRM, WAIT_TIMER, SERVO_ACTIVE, STEPPER_ACTIVE, COMPLETE};
SystemState currentState = READY;

unsigned long confirmStart = 0;
const unsigned long confirmTime = 3000; // Water must stay HIGH 3 sec
bool waterConfirmed = false;

unsigned long timerStart = 0;
const unsigned long waitTime = 5000;      // Wait time after confirmation
const unsigned long servoDuration = 5000;  // Servo open duration
const unsigned long stepInterval = 10;     // Stepper speed (ms)
const int stepsPerCycle = 600;             // Steps per unclog cycle

int stepNumber = 0;
int stepCycles = 0;
unsigned long lastStepperMillis = 0;

// ===== Setup =====
void setup() {
  Serial.begin(9600);

  pinMode(sensorPin, INPUT);

  pinMode(stepPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(stepPin3, OUTPUT);
  pinMode(stepPin4, OUTPUT);

  myServo.attach(servoPin);
  myServo.write(0); // Close at start

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("System Ready   ");
  lcd.setCursor(0,1);
  lcd.print("Monitoring...  ");
}

// ===== Loop =====
void loop() {
  int sensorValue = digitalRead(sensorPin);

  switch(currentState) {

    case READY:
      waterConfirmed = false;
      confirmStart = 0;
      if(sensorValue == HIGH){
        currentState = WATER_CONFIRM;
        lcd.setCursor(0,0); lcd.print("Water Detected ");
        lcd.setCursor(0,1); lcd.print("Confirming...  ");
      }
      break;

    case WATER_CONFIRM:
      if(sensorValue == HIGH){
        if(confirmStart == 0) confirmStart = millis();
        else if(millis() - confirmStart >= confirmTime){
          waterConfirmed = true;
          timerStart = millis();
          currentState = WAIT_TIMER;
          lcd.setCursor(0,0); lcd.print("Water Confirmed");
          lcd.setCursor(0,1); lcd.print("Starting Timer ");
        }
      } else {
        // Water dropped before confirmation → go back to READY
        currentState = READY;
        lcd.setCursor(0,0); lcd.print("Monitoring...  ");
        lcd.setCursor(0,1); lcd.print("No Water      ");
      }
      break;

    case WAIT_TIMER:
      if(millis() - timerStart >= waitTime){
        currentState = SERVO_ACTIVE;
        timerStart = millis();
        myServo.write(90);           // Open servo
        lcd.setCursor(0,0); lcd.print("Servo Open     ");
        lcd.setCursor(0,1); lcd.print("Releasing Liquid");
      }
      break;

    case SERVO_ACTIVE:
      if(millis() - timerStart >= servoDuration){
        myServo.write(0);             // Close servo
        stepCycles = 0;
        lastStepperMillis = millis();
        currentState = STEPPER_ACTIVE;
        lcd.setCursor(0,0); lcd.print("Stepper Active ");
        lcd.setCursor(0,1); lcd.print("Clearing Clog  ");
      }
      break;

    case STEPPER_ACTIVE:
      if(millis() - lastStepperMillis >= stepInterval){
        stepMotor();
        lastStepperMillis = millis();
        stepCycles++;
        if(stepCycles >= stepsPerCycle){
          currentState = COMPLETE;
          lcd.setCursor(0,0); lcd.print("Process Complete");
          lcd.setCursor(0,1); lcd.print("System Ready    ");
        }
      }
      break;

    case COMPLETE:
      // Reset system after finishing
      currentState = READY;
      break;
  }
}

// ===== Stepper Motor Function (half-step 28BYJ-48) =====
void stepMotor(){
  switch(stepNumber){
    case 0: digitalWrite(stepPin1,HIGH); digitalWrite(stepPin2,LOW);  digitalWrite(stepPin3,LOW);  digitalWrite(stepPin4,LOW);  break;
    case 1: digitalWrite(stepPin1,HIGH); digitalWrite(stepPin2,LOW);  digitalWrite(stepPin3,HIGH); digitalWrite(stepPin4,LOW);  break;
    case 2: digitalWrite(stepPin1,LOW);  digitalWrite(stepPin2,LOW);  digitalWrite(stepPin3,HIGH); digitalWrite(stepPin4,LOW);  break;
    case 3: digitalWrite(stepPin1,LOW);  digitalWrite(stepPin2,HIGH); digitalWrite(stepPin3,HIGH); digitalWrite(stepPin4,LOW);  break;
    case 4: digitalWrite(stepPin1,LOW);  digitalWrite(stepPin2,HIGH); digitalWrite(stepPin3,LOW);  digitalWrite(stepPin4,LOW);  break;
    case 5: digitalWrite(stepPin1,LOW);  digitalWrite(stepPin2,HIGH); digitalWrite(stepPin3,LOW);  digitalWrite(stepPin4,HIGH); break;
    case 6: digitalWrite(stepPin1,LOW);  digitalWrite(stepPin2,LOW);  digitalWrite(stepPin3,LOW);  digitalWrite(stepPin4,HIGH); break;
    case 7: digitalWrite(stepPin1,HIGH); digitalWrite(stepPin2,LOW);  digitalWrite(stepPin3,LOW);  digitalWrite(stepPin4,HIGH); break;
  }

  stepNumber++;
  if(stepNumber > 7) stepNumber = 0;
}