//----------------------------
//    Variables and Constants 
//----------------------------

//DO NOT CHANGE- xxxxxxxxxxxxxxxxxxxx
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 915.0

#define CLIENT_ADDRESS 1 //Football is client
#define SERVER_ADDRESS 2 //Boombox is server

#define PRIME_SW 10
#define FIRE_SW 11

#define RED_LED A3 //A0 pin was not working properly on the board
#define GREEN_LED A1
#define BLUE_LED A2
#define BUTTON_LED 12

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
int data_length;
bool responseSuccessful;

//Class objects
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

//State machine definition
enum STATE
{
  CONN_WAIT,
  SAFE,
  MARM,
  PRIME,
  ARM,
  FIRE,
  COLLECT,
  SAFING,
  ERR
};

STATE state = ERR;

//-DO NOT CHANGE xxxxxxxxxxxxxxxxxxxx




//----------------------------
//    Functions 
//----------------------------

//Setup functions
void setup_serial(int baud_rate) {
  Serial.begin(baud_rate);
  delay(2000);
  Serial.println("Serial initialized");
}

void setup_towerlight() {
  Serial.println("Tower light initialized");
  delay(500);
}

void setup_radio() {
  //Initializes the RHReliableDatagram manager and Radio Driver
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  
  if (!manager.init()) {
    Serial.println("Manager init failed");
  }

  while (!driver.setFrequency(RF95_FREQ)) {
    Serial.println("Freq set failed");
    delay(5);
  }

  manager.setRetries(3);

  driver.setTxPower(23, false);

  Serial.println("Radio initialized");

  state = CONN_WAIT;
  delay(500);
  
}

void setup_pins() {
  pinMode(PRIME_SW, INPUT);
  pinMode(FIRE_SW, INPUT);

  pinMode(BUTTON_LED, OUTPUT);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  analogWrite(RED_LED, 170);
  analogWrite(GREEN_LED, 230);
  analogWrite(BLUE_LED, 255);
  delay(500);
  analogWrite(RED_LED, 0);
  analogWrite(GREEN_LED, 0);
  analogWrite(BLUE_LED, 0);
  delay(300);
  analogWrite(RED_LED, 170);
  analogWrite(GREEN_LED, 230);
  
  
  delay(100);
}

//LED Functions
void LED_write(const char* led, int value) {
  if (led == "RED") {
    analogWrite(RED_LED, value);
  }

  if (led == "GREEN") {
    analogWrite(GREEN_LED, value);
  }

  if (led == "BLUE") {
    analogWrite(BLUE_LED, value);
  }
  
}

void LED_reset() {
  analogWrite(RED_LED, 0);
  analogWrite(GREEN_LED, 0);
  analogWrite(BLUE_LED, 0);
}


//Radio Functions
void send_packet(uint8_t data[], int data_length, int boombox_address)
{
  Serial.print("Sending message to boombox: ");
  print_byte(data, data_length);

  if (manager.sendtoWait(data, data_length, boombox_address))
  {
    // Now wait for a reply from the server
    uint8_t len = sizeof(buf);
    uint8_t from;   
    if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
    {
      Serial.print("got reply from : 0x");
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);
      responseSuccessful = true;
    }
    else
    {
      Serial.println("No reply, is rf95_reliable_datagram_server running?");
      responseSuccessful = false;
    }
  }
  else
  {
    Serial.println("sendtoWait failed");
    responseSuccessful = false;
  }
  delay(500);
}

void print_byte(uint8_t input[], int data_length) {
  //RH_RF95::printBuffer("Print: ", input, data_length);
  Serial.println((char*)input);
}

bool received_message(const char* message) {
  if (strcmp((char*)buf, message) == 0)
  {
    return true; 
  }
  else
  {
    return false;
  }
}

void wait_for_marm(const char* signal, uint8_t response[], int response_length) {
      if (manager.available())
      {
        uint8_t len = sizeof(buf);
        uint8_t from;
        if (manager.recvfromAck(buf, &len, &from))
        {
          Serial.print("got request from : 0x");
          Serial.print(from, HEX);
          Serial.print(": ");
          Serial.println((char*)buf);
          
    
          // Send a reply back to the originator client
          if (!manager.sendtoWait(response, response_length, from))
            {
              Serial.println("sendtoWait failed");
              responseSuccessful = false;
            }
            else
            {
              responseSuccessful = true;
            }
          }
        }
}


//----------------------------
//    Main 
//----------------------------

void setup() {
  //Starts serial communication at specified rate
  setup_serial(9600);
  
  //Initializes the RHReliableDatagram manager and Radio Driver
  setup_radio();

  //Initializes tower light
  setup_towerlight();

  //Initialize Pins
  setup_pins();
  
}

void loop() {
  responseSuccessful = false;
  
  //Conn_wait--------------------------------
  if (state == CONN_WAIT) {
    Serial.println("Awaiting connection");
    
    uint8_t data[] = "BDR CONN";
    data_length = sizeof(data);

    //Send out a packet, if it doesn't work then try again
    Serial.println("Awaiting connection");
    send_packet(data, data_length, SERVER_ADDRESS);

    //Check if the message received is equivalent to argument passed
    if (received_message("BDR CONN CONFIRM") && responseSuccessful)
    {
      Serial.println("Connection confirmed\n");
      state = SAFE;
      LED_reset();
      LED_write("GREEN", 255);
    }
    else
    {
      Serial.println("Failed to get connection signal");
      state = CONN_WAIT;
    }
  }

  //Safe--------------------------------
  if (state == SAFE) {
    Serial.println("*STATE: SAFE*");
    
    uint8_t response[] = "BDR MARM CONFIRM";
    data_length = sizeof(response);
    
    wait_for_marm("BDR MARM", response, data_length);
    
    if (received_message("BDR MARM") && responseSuccessful)
    {
      Serial.println("Received MARM signal\n");
      state = MARM;
      LED_reset();
      LED_write("RED", 170);
      LED_write("GREEN", 190);
    }
    else
    {
      Serial.println("Waiting for MARM signal");
      state = SAFE;
    }
  }

  //MARM--------------------------------
  if (state == MARM) {
    Serial.println("*STATE: MARM*");
    Serial.println("Waiting for PRIME Switch");
    
    int buttonstate = digitalRead(PRIME_SW);
    
    if (buttonstate == HIGH) {
      Serial.println("PRIME_SW ARMED");
      
      uint8_t data[] = "BDR PRIME";
      data_length = sizeof(data);
      
      Serial.println("Sending PRIME signal");
      send_packet(data, data_length, SERVER_ADDRESS);
      

      if (received_message("BDR PRIME CONFIRM")  && responseSuccessful)
      {
        Serial.println("PRIME confirmed\n");
        state = PRIME;
        LED_reset();
        LED_write("RED", 255);
        digitalWrite(BUTTON_LED, HIGH);
      }
      else
      {
        Serial.println("Failed to get PRIME signal");
        state = MARM;
      }
    }
  }

  //PRIME--------------------------------
  if (state == PRIME) {
    Serial.println("*STATE: PRIME*");
    Serial.println("Waiting for FIRE switch");
    
    int buttonstate = digitalRead(FIRE_SW);
    
    if (buttonstate == HIGH) {
      Serial.println("FIRE_SW ARMED");
      
      uint8_t data[] = "BDR FIRE";
      data_length = sizeof(data);
      
      Serial.println("Sending FIRE signal");
      send_packet(data, data_length, SERVER_ADDRESS);
      

      if (received_message("BDR FIRE CONFIRM")  && responseSuccessful)
      {
        Serial.println("FIRE confirmed\n");
        state = FIRE;
        LED_reset();
        LED_write("RED", 170);
        LED_write("GREEN", 255);
        LED_write("BLUE", 255);
        digitalWrite(BUTTON_LED, LOW);
        while(true) {
          digitalWrite(BUTTON_LED, HIGH);
          delay(100);
          digitalWrite(BUTTON_LED, LOW);
          delay(100);
        }
      }
      else
      {
        Serial.println("Failed to get FIRE signal");
        state = PRIME;
      }
    }
  }
  delay(100);
}
