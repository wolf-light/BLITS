// Libraries
#include <avr/power.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"
#include "HX711.h"

#define I2C_ADDRESS (0x67)

Adafruit_MCP9600 mcp;

// Pin definitions
#define TC_DO_PIN   3
#define TC_CS_PIN   4
#define TC_CLK_PIN  5

#define LC_DAT_PIN  6
#define LC_CLK_PIN  7

#define LC_calibration_factor -1750.0

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

/**
 * If using softwareSerial use Serial1 for control serial
 * Make sure these are the right pins that you're using 
*/
#define Software_RX 10
#define Software_TX 11


/*--------------READ THE BELOW COMMENTS----------------------*/
/**
 * This is defined when using both the built in serial of the nano, either with RX TX or USB, and two digital pins as a softwareSerial connection.
 * Comment the below out when using just one Serial connection.
*/
#define DIFFERENT_SERIALS //Use this define if the two serials are different (comment out if using 1 serial connection)

const char* safe_message = "system state: (1)SAFE, enter \"start\" to marm system";
const char* marm_message = "system state: (2)MARM, enter \"yes\" to prime system";
const char* prime_message= "system state: (3)PRIME, enter \"fire\" to fire";
const char* fire_message = "system state: (4)FIRE, reading test data";

const char* serial_setup_msg = "serial setup complete";
const char* loadcell_setup_msg = "loadcell setup complete";
const char* thermocouple_setup_msg = "tc setup complete";
const char* ematch_setup_mesg = "ematch setup complete";

// KEEP IN ACENDING ORDER OF DANGER
enum class STATE {
   SAFE,
   MARM,
   PRIME,
   FIRE
};

// Object declarations
SoftwareSerial Serial1(Software_RX,Software_TX);
// Adafruit_MAX31855 ThermoCouple(TC_CLK_PIN, TC_CS_PIN, TC_DO_PIN);
HX711 LoadCell;

// Universal Variables
STATE state = STATE::SAFE;
String command;
bool armState = false;
unsigned long readTime;

// Times used in FIRE state of loop()
unsigned long fireTime = 180000; // length of data collection after ignition
unsigned long dataOffset = 5000; // length of data collection before ignition
unsigned long hurtsTime = 2000; // length of fire of hurts (ematch) pin
unsigned long fireStart; // start of the fire from millis()
unsigned long relativeTime; // time relative to fireStart

// Calibration factors
const int testReadings = 1000;

void set_calibration_factors() {
    print_both("enter calibration factors");
}

void print_system_info() {
    String data = "Calibration Factors: ";
    // Put calibration factors here
    data += " TIMES: fireTime(";
    data += fireTime;
    data += "), dataOffset(";
    data += dataOffset;
    data += ") hurtsTime(";
    data += hurtsTime;
    data += ") ";
    data += "Thermocouple type set to ";
    switch (mcp.getThermocoupleType()) {
      case MCP9600_TYPE_K:  data += "K"; break;
      case MCP9600_TYPE_J:  data += "J"; break;
      case MCP9600_TYPE_T:  data += "T"; break;
      case MCP9600_TYPE_N:  data += "N"; break;
      case MCP9600_TYPE_S:  data += "S"; break;
      case MCP9600_TYPE_E:  data += "E"; break;
      case MCP9600_TYPE_B:  data += "B"; break;
      case MCP9600_TYPE_R:  data += "R"; break;
    }
    data += " type, ";
    data += "Filter coefficient value set to: ";
    data += mcp.getFilterCoefficient();
    data += ", Alert #1 temperature set to ";
    data += mcp.getAlertTemperature(1);
    data += ", ";
    data += "ADC resolution set to ";
    switch (mcp.getADCresolution()) {
      case MCP9600_ADCRESOLUTION_18:   data += "18"; break;
      case MCP9600_ADCRESOLUTION_16:   data += "16"; break;
      case MCP9600_ADCRESOLUTION_14:   data += "14"; break;
      case MCP9600_ADCRESOLUTION_12:   data += "12"; break;
    }
    data += " bits";
    delay(1000);
    print_both(data);
}

/**
 * Completes the logic for if messages are sent to both serial connections
 * Dependent on the if there are two serial connections or just one
*/
void print_both(String message) {
    // #if DIFFERENT_SERIALS
    CONTROL_SERIAL.println(message);
    // #endif
    DATA_SERIAL.println(message);
}

void test_data_reading() {
    int i=0;
    while (i<testReadings){
        CONTROL_SERIAL.println(sensor_read());
        i++;
    }
    proccess_current_state();
}

String sensor_read() {
    String data("");
    data += millis();
    data += ",";

    // data += ThermoCouple.readCelsius();
    data += mcp.readThermocouple();
    data += ",";

    data += LoadCell.get_units();
    // data += 0.00;
    data += ",";

    // data += analogRead(PS1_PIN);
    data += 0.00;
    data += ",";

    // data = analogRead(PS2_PIN);
    data += 0.00;
    // data += ",";

    // data += analogRead(PS3_PIN);
    // data += 0.00;

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
  print_both(serial_setup_msg);
}

void setup_ematch() {
    pinMode(HURTS_PIN, OUTPUT);
    print_both(ematch_setup_mesg);
}

void setup_loadcell() {
    LoadCell.begin(LC_DAT_PIN, LC_CLK_PIN);
    LoadCell.set_scale(LC_calibration_factor); // found with HX_set_persistent example code
    delay(1000);
    LoadCell.tare();
    print_both(loadcell_setup_msg);
}

void setup_thermocouple() {
    /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
    if (! mcp.begin(I2C_ADDRESS)) {
        print_both("Sensor not found. Check wiring!");
        while (1);
    }

  mcp.setADCresolution(MCP9600_ADCRESOLUTION_18);
  
  mcp.setThermocoupleType(MCP9600_TYPE_K);

  mcp.setFilterCoefficient(3);

  mcp.setAlertTemperature(1, 30);

  mcp.configureAlert(1, true, true);  // alert 1 enabled, rising temp

  mcp.enable(true);

  print_both(thermocouple_setup_msg);
}

//---------ON STATE TRANSITION------------------------//
void proccess_current_state() {
    switch (state) 
    {
    case STATE::SAFE:
        digitalWrite(HURTS_PIN, LOW);
        print_both(safe_message);
        armState = false;
        break;
    case STATE::MARM:
        print_both(marm_message);
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
        int now = millis();
        while(millis() - now > 10000) {
            if (CONTROL_SERIAL.available() > 0) {
                state = STATE::SAFE;
                proccess_current_state();
                return;
            }
        }
        state = STATE::FIRE;
    }
    proccess_current_state();
}

//---------SETUP AND LOOP FUNCTIONS------------------------//
void setup() {
  serial_setup();
  setup_loadcell();
  setup_thermocouple();
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