// Libraries
#include <avr/power.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"
#include "HX711.h"

// Pin definitions
#define I2C_ADDRESS_1 (0x67)
#define I2C_ADDRESS_2 (0x60)

#define LC_DAT_PIN 6
#define LC_CLK_PIN 7

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

const auto TC_1_TYPE = MCP9600_TYPE_J;
const auto TC_2_TYPE = MCP9600_TYPE_J;

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
HX711 LoadCell;
Adafruit_MCP9600 tc_1;
Adafruit_MCP9600 tc_2;

// Universal Variables
STATE state = STATE::FIRE;
String command;
bool armState = true;
unsigned long readTime;

// Times used in FIRE state of loop()
unsigned long fireTime = 180000;  // length of data collection after ignition
unsigned long dataOffset = 0;  // length of data collection before ignition
unsigned long hurtsTime = 2000;   // length of fire of hurts (ematch) pin
unsigned long fireStart;          // start of the fire from millis()
unsigned long relativeTime;       // time relative to fireStart
int fireState = 0;

// Calibration factors
const int testReadings = 6;
int LC_calibration_factor = -1750.0;


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


  String input;
  if (DATA_SERIAL.available() > 0) {
    input = DATA_SERIAL.readString();
  }
  while (input == '+' || input == 'a' || input == '-' || input == 'z') {
    LoadCell.set_scale(LC_calibration_factor);  //Adjust to this calibration factor

    DATA_SERIAL.print("Reading: ");
    DATA_SERIAL.print(LoadCell.get_units(), 1);
    DATA_SERIAL.print(" lbs");  //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
    DATA_SERIAL.print(" calibration_factor: ");
    DATA_SERIAL.print(LC_calibration_factor);
    DATA_SERIAL.println();

    if (DATA_SERIAL.available()) {
      char temp = DATA_SERIAL.read();
      if ('+' == temp || 'a' == temp) {
        LC_calibration_factor += 10;
      } else if ('-' == temp|| 'z' == temp) {
        LC_calibration_factor -= 10;
      } else {
        break;
      }
    }
  }
  print_both("Calibration Complete");
}

void print_system_info() {
  print_both("System Info Incoming: ");
  String info = "Calibration Factors: ";

  info += " TIMES: fireTime(";
  info += fireTime;
  info += "), dataOffset(";
  info += dataOffset;
  info += ") hurtsTime(";
  info += hurtsTime;
  info += ") \n";

  print_both(info);
  print_both(thermocouple_info(&tc_1, '1'));      // Is this correct? Or is it just "tc_1"
  print_both(thermocouple_info(&tc_2, '2'));
}

char thermocouple_type(const Adafruit_MCP9600* const thermocouple) {
  switch (thermocouple->getThermocoupleType()) {
    case MCP9600_TYPE_K: return 'K';
    case MCP9600_TYPE_J: return 'J'; 
    case MCP9600_TYPE_T: return 'T';
    case MCP9600_TYPE_N: return 'N';
    case MCP9600_TYPE_S: return 'S'; 
    case MCP9600_TYPE_E: return 'E'; 
    case MCP9600_TYPE_B: return 'B'; 
    case MCP9600_TYPE_R: return 'R';
    default: return 'U';
  }
}

int thermocouple_address_resolution(const Adafruit_MCP9600* const thermocouple) {
  switch (thermocouple->getADCresolution()) {
    case MCP9600_ADCRESOLUTION_18: return 18;
    case MCP9600_ADCRESOLUTION_16: return 16;
    case MCP9600_ADCRESOLUTION_14: return 14;
    case MCP9600_ADCRESOLUTION_12: return 12;
    default: return -1;
  }
}

String thermocouple_info(const Adafruit_MCP9600* const thermocouple, char num) {
  String info = "";
  info += "Thermocouple " + String(num) + " type set to " + String(thermocouple_type(thermocouple)) + " type, ";
  info += "Filter coefficient value set to: " + String(thermocouple->getFilterCoefficient());
  info += ", Alert #1 temperature set to " + String(thermocouple->getAlertTemperature(1));
  info += ", ADC resolution set to " + String(thermocouple_address_resolution(thermocouple)) + " bits";
  return info;
}

/**
 * Completes the logic for if messages are sent to both serial connections
 * Dependent on the if there are two serial connections or just one
*/
void print_both(String message) {
  #ifdef DIFFERENT_SERIALS
  CONTROL_SERIAL.println(message);
  #endif
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
  data += relativeTime;
  data += ",";

  // data += tc_1.readThermocouple();
    data += 0.00;
  data += ",";

  // data += tc_2.readThermocouple();
    data += 0.00;
  data += ",";

  data += LoadCell.get_units();
  data += ",";

  float p = analogRead(PS1_PIN);
  data += p;
  data += ",";

  p = analogRead(PS2_PIN);
  data += p;

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

void setup_thermocouple(Adafruit_MCP9600* thermocouple, int address, _themotype type) {
  /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
  if (!thermocouple->begin(address)) {
    print_both("Sensor TC 1 not found. Check wiring!");
    exit;
  }

  thermocouple->setADCresolution(MCP9600_ADCRESOLUTION_18);

  thermocouple->setThermocoupleType(type);

  thermocouple->setFilterCoefficient(3);

  thermocouple->setAlertTemperature(1, 30);

  thermocouple->configureAlert(1, true, true);  // alert 1 enabled, rising temp

  thermocouple->enable(true);

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
      fireState = 0;
      break;
  }
}

//---------STATE CHANGE FUNCTIONS------------------------//
void safe_to_marm() {
  if (STATE::SAFE == state) {
    state = STATE::MARM;
  }
  proccess_current_state();
}

void marm_to_prime() {
  if (STATE::MARM == state) {
    state = STATE::PRIME;
  }
  proccess_current_state();
}

void prime_to_fire() {
  if (STATE::PRIME == state) {
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
      relativeTime = 0;    //If we do a proper countdown, it messes up the formatting of our data in the cloud... maybe.
      relativeTime = relativeTime + 0.0000000001;
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
  // setup_thermocouple(&tc_1, I2C_ADDRESS_1, TC_1_TYPE);
  // setup_thermocouple(&tc_2, I2C_ADDRESS_2, TC_2_TYPE);
  setup_ematch();
}

// command = "systest";

void loop() {

  // if (command == "systest") {
  //   command = "start";
  //   delay(400);
  // } else if (command == "start") {
  //   command = "yes";
  //   delay(400);
  // } else if (command == "yes") {
  //   command = "fire";
  //   delay(400);
  // }


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
      print_both("Invalid Command: ");
      print_both(command);
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