//For Load Cell
#include "HX711.h"
#include <SPI.h>
#include <SD.h>

#define calibration_factor -1770.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 5;
float force;
unsigned long period;  
HX711 scale;

//For SD card
String datastring;

const int chipSelect = 10;

//For Servo
File myFile;


//void receiveEvent(int bytes) {
//  x = Wire.read();    // read one character from the I2C
//}

void setup() {
  Serial.begin(9600);
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

//End of Card Initiailization
//Setting up the load cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor); //This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
//Finish Load Cell setup

  myFile = SD.open("datalog1.txt", FILE_WRITE);
  myFile.println("Time, Force (lb)");
  myFile.close();

}

void loop() {
  force = scale.get_units(); //scale.get_units() returns a float

  period = (millis());

  //Creating and Writing to SD card
  String dataString = "";  
  dataString = String(period) + "," + String(force);
  myFile = SD.open("datalog1.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (myFile) {
    myFile.println(dataString);//data
    myFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
    
  }

  Serial.println(dataString);
  }
