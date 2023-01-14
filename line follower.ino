#include <QTRSensors.h>
const int m11Pin = 7;
const int m12Pin = 6;
const int m21Pin = 5;
const int m22Pin = 4;
const int m1Enable = 11;
const int m2Enable = 10;

int m1Speed = 0;
int m2Speed = 0;


float kp = 0.0783;
float ki = 0;
float kd = 0.28457;
float factor = 1;


float p = 0;
int i = 0;
int d = 0;

int error = 0;
int lastError = 0;

const int maxSpeed = 255;
const int minSpeed = -255;

const int baseSpeed = 201;

QTRSensors qtr;

const int sensorCount = 6;
int sensorValues[sensorCount];
int sensors[sensorCount] = {0, 0, 0, 0, 0, 0};

const byte leftSignalPin = 9;
const byte rightSignalPin = 8;

unsigned long calibrationDuration = 5000;
unsigned long startTime = 0;
bool calibrationDirection = 0;
int calibrationSpeed = 177;
bool needsChange = true;
const unsigned long noChangeInterval = 400;
unsigned long changedTime = 0;

int m2SpeedCopy = 0;

bool carState = 0;
const bool calibrationState = 0;
const bool runState = 1;

void setup() {

  Serial.begin(9600);

  // pinMode setup
  pinMode(m11Pin, OUTPUT);
  pinMode(m12Pin, OUTPUT);
  pinMode(m21Pin, OUTPUT);
  pinMode(m22Pin, OUTPUT);
  pinMode(m1Enable, OUTPUT);
  pinMode(m2Enable, OUTPUT);
  pinMode(leftSignalPin, OUTPUT);
  pinMode(rightSignalPin, OUTPUT);
  
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5}, sensorCount);

  // delay for turning on the battery
  delay(500);
  startTime = millis();
  digitalWrite(leftSignalPin, HIGH);
  digitalWrite(rightSignalPin, HIGH);
}

void loop() {
  if(carState == calibrationState) {
    // calibration for the sensor
    calibration();
  }
  else if(carState == runState) {
    // following the line
    run();
  }
}

// this function calibrates the values for the sensor dynamically based on real-time reading values
void calibration() {
  qtr.calibrate();
  qtr.read(sensorValues);

  if(millis() - changedTime > noChangeInterval) {
    needsChange = true;
    for(int i = 0; i < 6; i++) {
      if(sensorValues[i] > 500) {
        needsChange = false;
      }
    }
    Serial.println("primu");
  }
  
  if(needsChange) {
    calibrationDirection = !calibrationDirection;
    needsChange = false;
    changedTime = millis();
  }

  if(calibrationDirection == 0) {
    setMotorSpeed(calibrationSpeed, -calibrationSpeed + 10);
  }
  else {
    setMotorSpeed(-calibrationSpeed - 10, calibrationSpeed);
  }

  if(millis() - startTime >= calibrationDuration) {
    carState = runState;
    setMotorSpeed(0, 0);
  }
}

void run() {
  float error = map(qtr.readLineBlack(sensorValues), 0, 5000, -2500, 2500);

  p = error;
  i = i + error;
  d = error - lastError;
  lastError = error;

  int motorSpeed = abs(kp * p + ki * i + kd * d);
  
  m1Speed = baseSpeed;
  m2Speed = baseSpeed;

  if (error < 0) {
    m1Speed -= motorSpeed;
    m2Speed += motorSpeed;
  }
  else if (error > 0) {
    m2Speed -= motorSpeed;
    m1Speed += motorSpeed;
  }

  normalizeSpeed();
  m1Speed = constrain(m1Speed, minSpeed, maxSpeed);
  m2Speed = constrain(m2Speed, minSpeed, maxSpeed);

  setMotorSpeed(m1Speed, m2Speed);
}

// this function reduces speed errors by extracting the excess value speed of a motor and reduces it from the other one
void normalizeSpeed() {
  m2SpeedCopy = m2Speed;

  if(m1Speed > 255) {
    m2Speed -= (m1Speed - 255) * factor;
  }
  if(m2SpeedCopy > 255) {
    m1Speed -= (m2SpeedCopy - 255) * factor;
  }
}

// each arguments takes values between -255 and 255. The negative values represent the motor speed in reverse.
void setMotorSpeed(int motor1Speed, int motor2Speed) {
   motor2Speed = -motor2Speed;
  if (motor1Speed == 0) {
    digitalWrite(m11Pin, LOW);
    digitalWrite(m12Pin, LOW);
    analogWrite(m1Enable, motor1Speed);
  }
  else {
    if (motor1Speed > 0) {
      digitalWrite(m11Pin, HIGH);
      digitalWrite(m12Pin, LOW);
      analogWrite(m1Enable, motor1Speed);
    }
    if (motor1Speed < 0) {
      digitalWrite(m11Pin, LOW);
      digitalWrite(m12Pin, HIGH);
      analogWrite(m1Enable, -motor1Speed);
    }
  }
  if (motor2Speed == 0) {
    digitalWrite(m21Pin, LOW);
    digitalWrite(m22Pin, LOW);
    analogWrite(m2Enable, motor2Speed);
  }
  else {
    if (motor2Speed > 0) {
      digitalWrite(m21Pin, HIGH);
      digitalWrite(m22Pin, LOW);
      analogWrite(m2Enable, motor2Speed);
    }
    if (motor2Speed < 0) {
      digitalWrite(m21Pin, LOW);
      digitalWrite(m22Pin, HIGH);
      analogWrite(m2Enable, -motor2Speed);
    }
  }
}