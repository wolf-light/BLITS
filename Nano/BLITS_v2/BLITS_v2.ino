// Libraries
#include <avr/power.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"
#include "HX711.h"

#define I2C_ADDRESS_1 (0x67)
#define I2C_ADDRESS_2 (0x60)

Adafruit_MCP9600 mcp;
Adafruit_MCP9600 mcp2;

// Pin definitions
#define TC_DO_PIN 3
#define TC_CS_PIN 4
#define TC_CLK_PIN 5

#define LC_DAT_PIN 6
#define LC_CLK_PIN 7

// #define LC_calibration_factor -1750.0
int LC_calibration_factor = -1750.0;

#define PS1_PIN A7
#define PS2_PIN A6
#define PS3_PIN A5

#define HURTS_PIN A3
#define SOL_PIN A1
#define SRVO_PIN 9

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
#define DIFFERENT_SERIALS  //Use this define if the two serials are different (comment out if using 1 serial connection)

const char* safe_message = "system state: (1)SAFE, enter \"start\" to marm system";
const char* marm_message = "system state: (2)MARM, enter \"yes\" to prime system";
const char* prime_message = "system state: (3)PRIME, enter \"fire\" to fire";
const char* fire_message = "system state: (4)FIRE, reading test data";

const char* serial_setup_msg = "serial setup complete";
const char* loadcell_setup_msg = "loadcell setup complete";
const char* thermocouple_setup_msg = "tc setup complete";
const char* ematch_setup_mesg = "ematch setup complete";
const char* zero_sensors_message = "sensors zeroed";

// KEEP IN ACENDING ORDER OF DANGER
enum class STATE {
  SAFE,
  MARM,
  PRIME,
  FIRE
};

// Object declarations
SoftwareSerial Serial1(Software_RX, Software_TX);
// Adafruit_MAX31855 ThermoCouple(TC_CLK_PIN, TC_CS_PIN, TC_DO_PIN);
HX711 LoadCell;

// Universal Variables
STATE state = STATE::SAFE;
String command;
bool armState = false;
unsigned long readTime;

// Times used in FIRE state of loop()
unsigned long fireTime = 180000;  // length of data collection after ignition
unsigned long dataOffset = 5000;  // length of data collection before ignition
unsigned long hurtsTime = 2000;   // length of fire of hurts (ematch) pin
unsigned long fireStart;          // start of the fire from millis()
unsigned long relativeTime;       // time relative to fireStart
int fireState = 0;

// Calibration factors
const int testReadings = 6;

void set_calibration_factors() {
  print_both("Enter Calibration Factors. Load Cell first. Next to skip, end to stop.");

  delay(1000);

  DATA_SERIAL.println("HX711 calibration sketch");
  DATA_SERIAL.println("Remove all weight from scale");
  DATA_SERIAL.println("After readings begin, place known weight on scale");
  DATA_SERIAL.println("Press + or a to increase calibration factor");
  DATA_SERIAL.println("Press - or z to decrease calibration factor");

  delay(1000);
  LoadCell.tare();  //Reset the scale to 0

  long zero_factor = LoadCell.read_average();  //Get a baseline reading
  DATA_SERIAL.print("Zero factor: ");          //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  DATA_SERIAL.println(zero_factor);

LOAD_CELL_START:
  String command2;
  if (DATA_SERIAL.available() > 0) {
    command2 = DATA_SERIAL.readString();
  }
  if (command2 == "next") {
    goto TC_START;
  } else if (command2 == "end") {
    goto END;
  } else if (command2 != '+' && command2 != 'a' && command2 != '-' && command2 != 'z') {
    goto TC_START;
  } else{
    LoadCell.set_scale(LC_calibration_factor);  //Adjust to this calibration factor

    DATA_SERIAL.print("Reading: ");
    DATA_SERIAL.print(LoadCell.get_units(), 1);
    DATA_SERIAL.print(" lbs");  //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    DATA_SERIAL.print(" calibration_factor: ");
    DATA_SERIAL.print(LC_calibration_factor);
    DATA_SERIAL.println();

    if (Serial.available()) {
      char temp = DATA_SERIAL.read();
      if (temp == '+' || temp == 'a') {
        LC_calibration_factor += 10;
      } else if (temp == '-' || temp == 'z') {
        LC_calibration_factor -= 10;
      }
    }
  }
  goto LOAD_CELL_START;
TC_START:
END:
print_both("Calibration Complete");
}

void print_system_info() {
  print_both("System Info Incoming: ");
  String data = "Calibration Factors: ";
  // Put calibration factors here
  data += " TIMES: fireTime(";
  data += fireTime;
  data += "), dataOffset(";
  data += dataOffset;
  data += ") hurtsTime(";
  data += hurtsTime;
  data += ") ";
  delay(500);
  print_both(data);
  data += "Thermocouple 1 type set to ";
  switch (mcp.getThermocoupleType()) {
    case MCP9600_TYPE_K: data += "K"; break;
    case MCP9600_TYPE_J: data += "J"; break;
    case MCP9600_TYPE_T: data += "T"; break;
    case MCP9600_TYPE_N: data += "N"; break;
    case MCP9600_TYPE_S: data += "S"; break;
    case MCP9600_TYPE_E: data += "E"; break;
    case MCP9600_TYPE_B: data += "B"; break;
    case MCP9600_TYPE_R: data += "R"; break;
  }
  data += " type, ";
  data += "Filter coefficient value set to: ";
  data += mcp.getFilterCoefficient();
  data += ", Alert #1 temperature set to ";
  data += mcp.getAlertTemperature(1);
  data += ", ";
  data += "ADC resolution set to ";
  switch (mcp.getADCresolution()) {
    case MCP9600_ADCRESOLUTION_18: data += "18"; break;
    case MCP9600_ADCRESOLUTION_16: data += "16"; break;
    case MCP9600_ADCRESOLUTION_14: data += "14"; break;
    case MCP9600_ADCRESOLUTION_12: data += "12"; break;
  }
  data += " bits";
  delay(500);
  print_both(data);
  data += "Thermocouple 2 type set to ";
  switch (mcp2.getThermocoupleType()) {
    case MCP9600_TYPE_K: data += "K"; break;
    case MCP9600_TYPE_J: data += "J"; break;
    case MCP9600_TYPE_T: data += "T"; break;
    case MCP9600_TYPE_N: data += "N"; break;
    case MCP9600_TYPE_S: data += "S"; break;
    case MCP9600_TYPE_E: data += "E"; break;
    case MCP9600_TYPE_B: data += "B"; break;
    case MCP9600_TYPE_R: data += "R"; break;
  }
  data += " type, ";
  data += "Filter coefficient value set to: ";
  data += mcp2.getFilterCoefficient();
  data += ", Alert #1 temperature set to ";
  data += mcp2.getAlertTemperature(1);
  data += ", ";
  data += "ADC resolution set to ";
  switch (mcp2.getADCresolution()) {
    case MCP9600_ADCRESOLUTION_18: data += "18"; break;
    case MCP9600_ADCRESOLUTION_16: data += "16"; break;
    case MCP9600_ADCRESOLUTION_14: data += "14"; break;
    case MCP9600_ADCRESOLUTION_12: data += "12"; break;
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

void print_both_int(unsigned long message) {
  // #if DIFFERENT_SERIALS
  CONTROL_SERIAL.println(message);
  // #endif
  DATA_SERIAL.println(message);
}
void test_data_reading() {
  int i = 0;
  while (i < testReadings) {
    CONTROL_SERIAL.println(sensor_read());
    i++;
  }
  proccess_current_state();
}

String sensor_read() {
  String data("");
  // data += millis();
  data += relativeTime;
  data += ",";

  // data += ThermoCouple.readCelsius();
  data += mcp.readThermocouple();
  data += ",";

  // data += ThermoCouple.readCelsius();
  data += mcp2.readThermocouple();
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
  while (!DATA_SERIAL || !CONTROL_SERIAL) {
    ;  // wait for serial connections to finish
  }
  print_both(serial_setup_msg);
}

void setup_ematch() {
  pinMode(HURTS_PIN, OUTPUT);
  delay(1000);
  digitalWrite(HURTS_PIN, LOW);
  print_both(ematch_setup_mesg);
}

void setup_loadcell() {
  LoadCell.begin(LC_DAT_PIN, LC_CLK_PIN);
  LoadCell.set_scale(LC_calibration_factor);  // found with HX_set_persistent example code
  delay(1000);
  LoadCell.tare();
  print_both(loadcell_setup_msg);
}

void zero_loadcell() {
  LoadCell.tare();
}

void setup_thermocouple() {
  /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
  if (!mcp.begin(I2C_ADDRESS_1)) {
    print_both("Sensor TC 1 not found. Check wiring!");
    while (1)
      ;
  }

  mcp.setADCresolution(MCP9600_ADCRESOLUTION_18);

  mcp.setThermocoupleType(MCP9600_TYPE_J);

  mcp.setFilterCoefficient(3);

  mcp.setAlertTemperature(1, 30);

  mcp.configureAlert(1, true, true);  // alert 1 enabled, rising temp

  mcp.enable(true);

  print_both(thermocouple_setup_msg);

  if (!mcp2.begin(I2C_ADDRESS_2)) {
    print_both("Sensor TC 2 not found. Check wiring!");
    while (1)
      ;
  }

  mcp2.setADCresolution(MCP9600_ADCRESOLUTION_18);

  mcp2.setThermocoupleType(MCP9600_TYPE_J);

  mcp2.setFilterCoefficient(3);

  mcp2.setAlertTemperature(1, 30);

  mcp2.configureAlert(1, true, true);  // alert 1 enabled, rising temp

  mcp2.enable(true);

  print_both(thermocouple_setup_msg);
}

void zero_sensors() {
  zero_loadcell();
  print_both(zero_sensors_message);
}

//---------ON STATE TRANSITION------------------------//
void proccess_current_state() {
  switch (state) {
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
      // print_both("Fire Start Time Set");
      // print_both_int(fireStart);
      fireState = 0;
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
    print_both("10 SECOND ABORT");
    delay(100);
    unsigned long now = millis();
    delay(100);
    while (millis() - now < 10000) {
      if (CONTROL_SERIAL.available() > 0) {
        print_both("TEST ABORTED");
        state = STATE::SAFE;
        proccess_current_state();
        return;
      }
      // delay(1000);
      // print_both("FIRE SEQUENCE");
      // print_both_int((millis() - now));
      relativeTime = 10000 - (millis() - now);
      sensor_read();
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
  setup_ematch();
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
      zero_sensors();   //Do we want this here? Or elsewhere?
      marm_to_prime();
    } else if (command == "fire") {
      prime_to_fire();
    } else if (command == "calibrate") {
      set_calibration_factors();
    } else if (command == "zero sensors") {
      zero_sensors();
    } else {
      Serial1.print("Invalid Command: ");
      Serial1.println(command);
    }
  }

  switch (state) {
    case STATE::SAFE:
      break;
    case STATE::MARM:
      break;
    case STATE::PRIME:
      break;
    case STATE::FIRE:
      // print_both("FIRE INITIATED");
      relativeTime = millis() - fireStart;
      // print_both("Relative Time Set");
      // print_both_int(relativeTime);
      // delay(1000);
      if (armState && relativeTime > dataOffset) {
        // print_both("HURTS HIGH");
        digitalWrite(HURTS_PIN, HIGH);
        fireState = 1;
        if (relativeTime - dataOffset > hurtsTime) {
          // print_both("ARM STATE SET FALSE");
          digitalWrite(HURTS_PIN, LOW);
          armState = false;
        }
      }
      if (fireState == 1 && (relativeTime - dataOffset > fireTime)) {
        state = STATE::SAFE;
        print_both("END TEST");
        proccess_current_state();
      }
      sensor_read();
      break;
  }
}