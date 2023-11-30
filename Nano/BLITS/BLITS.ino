// Libraries
#include <Adafruit_TiCoServo.h>
#include <avr/power.h>
#include <SPI.h>
#include <SoftwareSerial.h>
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

#define DATA_SERIAL Serial
#define DATA_BAUDRATE 115200

#define CONTROL_SERIAL Serial1
#define CONTROL_BAUDRATE 9600

#define DIFFERENT_SERIALS //Use this define if the two above serials are different

enum class STATE {
   SAFE,
   MARM,
   PRIME,
   FIRE
};

// Object declarations
SoftwareSerial Serial1(10,11);
Adafruit_MAX31855 ThermoCouple(TC_CLK_PIN, TC_CS_PIN, TC_DO_PIN);
HX711 LoadCell;

STATE state = STATE::SAFE;
bool armState = false;
String command;
String data;

bool serial_setup() {
  #ifdef DIFFERENT_SERIALS
  CONTROL_SERIAL.begin(CONTROL_BAUDRATE);
  #endif
  DATA_SERIAL.begin(DATA_BAUDRATE);
}

void sensorRead() {
    data = "";
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

void fire() {
    switch (armState) {
        case true:
            digitalWrite(HURTS_PIN, HIGH);
            break;
    
        case false:
            digitalWrite(HURTS_PIN, LOW);
            break;
    }
    sensorRead();
}

bool read_serial() {
    if (CONTROL_SERIAL.available() > 0) {
        message = CONTROL_SERIAL.readString();
        return true;
    }
    return false;
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
    switch (state)
    {
    case STATE::SAFE:
        armState = false;
        if (read_serial()) {
            if (command == "start") {
                CONTROL_SERIAL.println("system MARMed. would you like to prime the system? yes/no");
                state = STATE::MARM;
            } else if (command == "read data") {
                test_data_reading();
            } else {
                CONTROL_SERIAL.println("system remaining in safe. enter \"start\" to marm the system");
            }
        }
        break;
    }
    case STATE::MARM:
        if (read_serial()) {
            if (command == "yes") {
                CONTROL_SERIAL.println("system primed. enter \"fire\" to fire");
                state = SATE::PRIME
            } else if (command == "read data") {
                test_data_reading();
            } else if (command == "safe") {

            } else {
                CONTROL_SERIAL.println("system not primed. enter \"yes\" to prime");
            }
        }
        break;
    case STATE::PRIME:
        if (read_serial()) {
            if (command == "fire") {
                CONTROL_SERIAL.println("firing");
            } else if (command)
        }
        break;
    case STATE::FIRE:
        break;
}



