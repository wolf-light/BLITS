// Libraries
#include <Adafruit_TiCoServo.h>
#include <avr/power.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include "Adafruit_MAX31855.h"
#include "HX711.h"

// Variable Definitions
#define calibration_factor -1750.0

// Pin definitions
#define TC_DO_PIN   3
#define TC_CS_PIN   4
#define TC_CLK_PIN  5

#define LC_DAT_PIN  6
#define LC_CLK_PIN  7

#define PS1_PIN  A7
#define PS2_PIN  A6
// #define PS3_PIN  A5

#define HURTS_PIN A3
#define SOL_PIN   A1
#define SRVO_PIN  9

#define DATA_SERIAL Serial
#define DATA_BAUDRATE 115200

#define CONTROL_SERIAL Serial1
#define CONTROL_BAUDRATE 115200

#define DIFFERENT_SERIALS //Use this define if the two above serials are different

const char* safe_message = "system state: SAFE, enter \"start\" to marm system";
const char* marm_message = "system state: MARM, enter \"yes\" to prime system";
const char* prime_message = "system state: PRIME, enter \"fire\" to fire";
const char* fire_message = "reading test data";

// KEEP IN ACENDING ORDER OF DANGER
enum class STATE {
   SAFE,
   MARM,
   PRIME,
   FIRE
};

// Object declarations
SoftwareSerial Serial1(A5,A4);
Adafruit_MAX31855 ThermoCouple(TC_CLK_PIN, TC_CS_PIN, TC_DO_PIN);
HX711 LoadCell;
Adafruit_TiCoServo Servo;

STATE state = STATE::SAFE;
String command;
bool armState = false;
unsigned long readTime;

const unsigned int fireTime = 180000;
const unsigned int dataOffset = 5000;
const unsigned int hurtsTime = 2000;
unsigned int fireStart;
unsigned int relativeTime;

bool serial_setup() {
  DATA_SERIAL.begin(DATA_BAUDRATE);
  #ifdef DIFFERENT_SERIALS
  CONTROL_SERIAL.begin(CONTROL_BAUDRATE);
  #endif
}

void proccess_current_state() {
    switch (state) 
    {
    case STATE::SAFE:
        CONTROL_SERIAL.println(safe_message);
        DATA_SERIAL.println(safe_message);
        armState = false;
        break;
    case STATE::MARM:
        CONTROL_SERIAL.println(marm_message);
        DATA_SERIAL.println(marm_message);
        armState = false;
        break;
    case STATE::PRIME:
        CONTROL_SERIAL.println(prime_message);
        DATA_SERIAL.println(prime_message);
        armState = true;
        break;
    case STATE::FIRE:
        CONTROL_SERIAL.println(fire_message);
        fireStart = millis();
        break;
    }
}

void test_data_reading(int counts) {
int i=0;
while (i<counts){
  String data;
    readTime = millis();
    data = "";
    Serial.print(readTime);
    Serial.print(",");
    data += readTime;
    data += ",";

    double c = ThermoCouple.readCelsius();
    Serial.print(c);
    Serial.print(",");
    data += c;
    data += ",";

    float m = LoadCell.get_units(0);
    Serial.print(m);
    Serial.print(",");
    data += m;
    data += ",";

    float p = analogRead(PS1_PIN);
    Serial.print(p);
    Serial.print(",");
    data += p;
    data += ",";

    p = analogRead(PS2_PIN);
    Serial.print(p);
    Serial.print(",");
    data += p;
    data += ",";

    // p = analogRead(PS3_PIN);
    // Serial.print(p);
    // Serial.print(",");
    // data += p;

    //Serial.print(pos);
    //Servo.write(pos);
    Serial.println();
    CONTROL_SERIAL.println(data);
    i++;
}
}

void sensor_read() {
    readTime = millis();
    Serial.print(readTime);
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

    // p = analogRead(PS3_PIN);
    // Serial.print(p);
    // Serial.print(",");

    //Serial.print(pos);
    //Servo.write(pos);
    Serial.println();
}


void safe_to_marm() {
    if (state == STATE::SAFE) {
        state = STATE::MARM;
    }
    proccess_current_state();
}

void marm_to_prime() {
    if (state == STATE::MARM) {
        state = STATE::PRIME;
    }
    proccess_current_state();
}

void prime_to_fire() {
    if (state == STATE::PRIME) {
        state = STATE::FIRE;
    }
    proccess_current_state();
}

void setup() {
  serial_setup();
  pinMode(SOL_PIN, OUTPUT);
  pinMode(HURTS_PIN, OUTPUT);
  LoadCell.begin(LC_DAT_PIN, LC_CLK_PIN);
  LoadCell.set_scale(calibration_factor);              // found with HX_set_persistent example code
  LoadCell.tare();
  #if (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1);
  #endif
  Servo.attach(SRVO_PIN);
}

void loop() {
    if (CONTROL_SERIAL.available() > 0) {
        command = CONTROL_SERIAL.readString();
        if (command == "read data") {
           test_data_reading(6);
        } else if (command == "info") {
           // print_system_info();
        } else if (command == "safe") {
            state = STATE::SAFE;
            proccess_current_state();
        } else if (command == "start") {
            safe_to_marm();
        } else if (command == "yes") {
            marm_to_prime();
        } else if (command == "fire") {
            prime_to_fire();
        } else {
            Serial1.print("Invalid Command: ");Serial1.print(command);
        }
    }
    
    switch (state)
    {
    case STATE::SAFE:
        break;
    case STATE::MARM:
        break;
    case STATE::PRIME:
        break;
    case STATE::FIRE:
        relativeTime = millis() - fireStart;
        Serial.println("Relative Time Set");
        Serial.println(relativeTime);
        if (armState && relativeTime > dataOffset) {
          Serial.println("In First IF");
            digitalWrite(HURTS_PIN, HIGH);
            if (relativeTime - dataOffset > hurtsTime) {
              Serial.println("In Second HURTS IF");
                armState = false;
                digitalWrite(HURTS_PIN, LOW);
            }
        }
        Serial.println("Outside of First Main If Loop Thing");
        if (relativeTime - dataOffset > fireTime) {
          Serial.println("Inside Final IF Loop");
            state = STATE::SAFE;
        } else {
          Serial.println("Skipped final IF loop, in else");
        // while(CONTROL_SERIAL.available() == 0){
        sensor_read();
        // }
        }
        // break;
    }

}

// Data Offset is how much it collects before ignition
// Relative Time is time relative to start of fire (end of countdown)
// Hurts Time length of ematch (hurts) ignition power
// Fire Time is length of data collection after ignition sequence