// Libraries
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
#define SOL_PIN   A1
#define SRVO_PIN  9

#define DATA_SERIAL Serial
#define DATA_BAUDRATE 115200

#define CONTROL_SERIAL Serial1
#define CONTROL_BAUDRATE 115200

// If using softwareSerial use Serial1 for control serial
#define Software_RX 10
#define Software_TX 11

#define DIFFERENT_SERIALS //Use this define if the two serials are different

const char* safe_message = "system state: (1)SAFE, enter \"start\" to marm system";
const char* marm_message = "system state: (2)MARM, enter \"yes\" to prime system";
const char* prime_message= "system state: (3)PRIME, enter \"fire\" to fire";
const char* fire_message = "system state: (4)FIRE, reading test data";

const char* serial_setup_msg = "";
const char* loadcell_setup_msg = "";
const char* thermocouple_setup_msg = "";
const char* ematch_setup_mesg = "";

// KEEP IN ACENDING ORDER OF DANGER
enum class STATE {
   SAFE,
   MARM,
   PRIME,
   FIRE
};

// Object declarations
SoftwareSerial Serial1(Software_RX,Software_TX);
Adafruit_MAX31855 ThermoCouple(TC_CLK_PIN, TC_CS_PIN, TC_DO_PIN);
HX711 LoadCell;

// Universal Variables
STATE state = STATE::SAFE;
String command;
bool armState = false;
unsigned long readTime;

// Times used in FIRE state of loop()
const unsigned long fireTime = 15000;
const unsigned long dataOffset = 5000;
const unsigned long hurtsTime = 2000;
unsigned long fireStart;
unsigned long relativeTime;


// Calibration factors

void set_calibration_factors() {
    print_both("enter calibration factors");
}

void print_system_info() {
    data = "Calibration Factors: ";
    // Put calibration factors here
    data += " TIMES: fireTime(";
    data += fireTime;
    data += "), dataOffset(";
    data += dataOffset;
    data += ") hurtsTime(";
    data += hurtsTime;
    data += ")";
    print_both(data);
}

void print_both(String message) {
    #if DIFFERENT_SERIALS
    CONTROL_SERIAL.println(message);
    #endif
    DATA_SERIAL.println(message);
}

void test_data_reading() {
    int i=0;
    while (i<6){
        CONTROL_SERIAL.println(sensor_read());
        i++;
    }
}

String sensor_read() {
    String data("");
    data += millis();
    data += ",";

    data += ThermoCouple.readCelsius();
    data += ",";

    data += LoadCell.get_units(0);
    data += ",";

    data += analogRead(PS1_PIN);
    data += ",";

    data = analogRead(PS2_PIN);
    data += ",";

    data += analogRead(PS3_PIN);

    DATA_SERIAL.println(data);
    return data;
}

//---------SETUP FUNCTIONS------------------------//
bool serial_setup() {
  DATA_SERIAL.begin(DATA_BAUDRATE);
  #ifdef DIFFERENT_SERIALS
  CONTROL_SERIAL.begin(CONTROL_BAUDRATE);
  #endif
  while(!DATA_SERIAL || !CONTROL_SERIAL) {
    ; // wait for serial connections to finish
  }
  print_both()
}

void setup_ematch() {
    pinMode(HURTS_PIN, OUTPUT);
}

void setup_loadcell() {
    LoadCell.begin(LC_DAT_PIN, LC_CLK_PIN);
    LoadCell.set_scale(4883); // found with HX_set_persistent example code
    LoadCell.tare();
}

//---------ON STATE TRANSITION------------------------//
void proccess_current_state() {
    switch (state) 
    {
    case STATE::SAFE:
        print_both(safe_message);
        armState = false;
        break;
    case STATE::MARM
        print_both(marm_message)
        armState = false;
        break;
    case STATE::PRIME:
        print_both(prime_message);
        armState = true;
        break;
    case STATE::FIRE:
        print_both(fire_message);
        fireStart = millis();
        break;
    }
}

//---------STATE CHANGE FUNCTIONS------------------------//
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

//---------SETUP AND LOOP FUNCTIONS------------------------//
void setup() {
  serial_setup();
}

void loop() {
    if (CONTROL_SERIAL.available() > 0) {
        command = CONTROL_SERIAL.readString();
        if (command == "read data") {
           test_data_reading();
        } else if (command == "info") {
           print_system_info();
        } else if (command == "safe") {
            state = STATE::SAFE;
            proccess_current_state();
        } else if (command == "start") {
            safe_to_marm();
        } else if (command == "yes") {
            marm_to_prime();
        } else if (command == "fire") {
            prime_to_fire();
        } else if (command == "calibrate") {
            set_calibration_factors();
        } else {
            Serial1.print("Invalid Command: ");
            Serial1.println(command);
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
        if (armState && relativeTime > dataOffset) {
            digitalWrite(HURTS_PIN, HIGH);
            if (relativeTime - dataOffset > hurtsTime) {
                armState = false;
                digitalWrite(HURTS_PIN, LOW);
            }
        }
        if (relativeTime - dataOffset > fireTime) {
            state = STATE::SAFE;
            proccess_current_state();
        }
        sensor_read();
        break;
    }
}