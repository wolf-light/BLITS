// Libraries
#include <Adafruit_TiCoServo.h>
#include <avr/power.h>
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "HX711.h"

// Pin definitions
#define TC_DO_PIN   3
#define TC_CS_PIN   4
#define TC_CLK_PIN  5

#define LC_DAT_PIN  6
#define LC_CLK_PIN  7

#define PS1_PIN  A7
#define PS2_PIN  A6
#define PS3_PIN  A5

#define HURTS_PIN A3
#define SOL_PIN   A1
#define SRVO_PIN  9

#include <SoftwareSerial.h>
#define DATA_SERIAL Serial
#define DATA_BAUDRATE 115200

#define CONTROL_SERIAL Serial1
#define CONTROL_BAUDRATE 115200

SoftwareSerial Serial1(10,11);

// Object declarations
Adafruit_MAX31855 ThermoCouple(TC_CLK_PIN, TC_CS_PIN, TC_DO_PIN);
HX711 LoadCell;
Adafruit_TiCoServo Servo;

// Variable declarations
unsigned long time = millis();
unsigned long startTime = 0;
bool armState = false;
int fireTime = 4000;
int Buffer = 1000;
int servoSpeed = 11;
int openAngle = 90;
float pos = 0;
String entry = "zero";

// Function declarations
void sensorRead();
void stopCheck();
void stop();
void zero();
void check();
void setFireTime();
void setServoSpeed();
void setOpenAngle();
void setZero();
void setArmState();
void fire();

bool serial_setup() {
  CONTROL_SERIAL.begin(CONTROL_BAUDRATE);
  DATA_SERIAL.begin(DATA_BAUDRATE);
  CONTROL_SERIAL.println("serial ports initialized");
}


void setup() {
  serial_setup();
  pinMode(SOL_PIN, OUTPUT);
  pinMode(HURTS_PIN, OUTPUT);
  LoadCell.begin(LC_DAT_PIN, LC_CLK_PIN);
  LoadCell.set_scale(4883);              // found with HX_set_persistent example code
  LoadCell.tare();
  #if (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1);
  #endif
  Servo.attach(SRVO_PIN);
}

void loop() {

  if (CONTROL_SERIAL.available()>0){
    entry= CONTROL_SERIAL.readString();
    entry.trim();

    if (entry == "s"){
      stop();
    }
    if (entry == "zero"){
      zero();
    }
    if (entry == "tare"){
      LoadCell.tare();
      entry = " ";
    }
    if (entry == "delay"){
      setFireTime();
    }
    if (entry == "speed"){
      setServoSpeed();
    }
    if (entry == "angle"){
      setOpenAngle();
    }
    if (entry == "read"){
      while (entry == "read") {
        sensorRead();
        check();
      }
      entry = " ";
    }
    if (entry == "start"){
      setArmState();
      entry = "go";
      while (entry == "go"){
        check();
      }
      if (entry == "fire"){
        fire();
      } else {
        CONTROL_SERIAL.println("none fire reset");
      }
    }
    entry = " ";
  }
}
void sensorRead() {
  Serial.print(millis());
  Serial.print(",");

  double c = ThermoCouple.readCelsius();
  Serial.print(c);
  Serial.print(",");

  float m = LoadCell.get_units(0);
  Serial.print(m);
  Serial.print(",");

  float p = analogRead(PS1_PIN);
  Serial.print(p);
  Serial.print(",");

  p = analogRead(PS2_PIN);
  Serial.print(p);
  Serial.print(",");

  p = analogRead(PS3_PIN);
  Serial.print(p);
  Serial.print(",");

  //Serial.print(pos);
  //Servo.write(pos);
  Serial.println();
  
}

void stopCheck() {
  if (CONTROL_SERIAL.available()>0){
    entry = CONTROL_SERIAL.readString();
    entry.trim();
    if (entry =! "fire"){
      stop();
    }
  }
}

void stop() {
  CONTROL_SERIAL.println("fire aborted type resume");
  while (entry != "resume"){
    Servo.write(0);
    if (CONTROL_SERIAL.available()>0){
      entry = CONTROL_SERIAL.readString();
      entry.trim();
    }
  }
  entry = "s";
}

void zero() {
  Servo.write(0);
  delay(15);
  entry = " ";
}

void check() {
  if (CONTROL_SERIAL.available()>0){
    entry = CONTROL_SERIAL.readString();
    entry.trim();
  }
}

void setFireTime() {
  CONTROL_SERIAL.println("enter value");
  while (entry == "delay") {
    if (CONTROL_SERIAL.available()>0){
      fireTime = CONTROL_SERIAL.parseFloat()*1000;
      CONTROL_SERIAL.print("value set to ");
      CONTROL_SERIAL.println(fireTime, DEC);
      entry = " ";
    }
  }
}

void setServoSpeed() {
 CONTROL_SERIAL.println("enter value");
  while (entry == "speed"){
    if (CONTROL_SERIAL.available()>0){
      servoSpeed = CONTROL_SERIAL.parseFloat();
      CONTROL_SERIAL.print("value set to ");
      CONTROL_SERIAL.println(servoSpeed, DEC);
      entry = " ";
    }
  }
}

void setOpenAngle() {
  CONTROL_SERIAL.println("enter value");
  while (entry == "angle"){
    if (CONTROL_SERIAL.available()>0){
      openAngle = CONTROL_SERIAL.parseFloat();
      CONTROL_SERIAL.print("value set to ");
      CONTROL_SERIAL.println(openAngle);
      entry = " ";
    }
  }
}

void setArmState() {
  CONTROL_SERIAL.println("arm ignition system? yes/no");
  while (entry == "start"){
    check();
  }
  if (entry == "yes"){
    armState = true;
    CONTROL_SERIAL.println("ignition is armed go for fire");
  } else {
      armState = false;
      CONTROL_SERIAL.println("not armed go for fire without ignition");
  }
}

void fire() {

  if (entry != "s"){
    startTime = millis();
    while (millis() - startTime < Buffer){
      if (entry != "s"){
        sensorRead();
        stopCheck();
      }
    }
  }

  if (entry != "s"){
    if (armState == true){
      digitalWrite(HURTS_PIN, HIGH);
    }
  }

  if (entry != "s"){
    for (pos; pos != openAngle; pos += 1){
      startTime = millis();
      while (millis() - startTime < servoSpeed){
        if (entry != "s"){
          sensorRead();
          stopCheck();
        }
      }
    }
  }

  if (entry != "s"){
    digitalWrite(HURTS_PIN, LOW);
  }

  if (entry != "s"){
    startTime = millis();
    while (millis() - startTime < fireTime){
      if (entry != "s"){
        sensorRead();
        stopCheck();
      }   
    }
  }
    
  if (entry != "s"){
    for (pos; pos != 0; pos -= 1){
      startTime = millis();
      while (millis() - startTime < servoSpeed){
        if (entry != "s"){
          sensorRead();
          stopCheck();
        }
      }
    }
  }

  if (entry != "s"){
    digitalWrite(SOL_PIN, HIGH);
  }

  if (entry != "s"){
    startTime = millis();
    while (millis() - startTime < Buffer){
      if (entry != "s"){
        sensorRead();
        stopCheck();
      }
    }
  }

  if (entry != "s"){
    digitalWrite(SOL_PIN, LOW);
  }

  entry = " ";
}