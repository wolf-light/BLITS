//----------------------------
//    Variables and Constants 
//----------------------------
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include "HX711.h"

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 915.0

#define CLK A5
#define DOUT A4
#define calibration_factor -1750.0

#define CLIENT_ADDRESS 1 //Football is client
#define SERVER_ADDRESS 2 //Boombox is server

#define MARM_SW 9
#define BUZZER 10
#define RED_LED 11
#define YELLOW_LED 12
#define GREEN_LED 13
#define EMATCH 5

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
int data_length;
bool responseSuccessful = false;
float start_time = -1;
float weight = -1;

//Class objects
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHReliableDatagram manager(driver, SERVER_ADDRESS);

HX711 scale;

//State machine definition
enum STATE
{
  CONN_WAIT,
  SAFE,
  MARM,
  PRIME,
  FIRE,
  COLLECT,
  SAFING,
  ERR
};

STATE state = ERR;

//Setup Functions
void setup_serial(int baud_rate) {
  Serial.begin(baud_rate);
  Serial.println("Serial initialized");
  delay(2000);
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

void setup_loadcell() {
  Serial.println("Initializing loadcell...");
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();
  Serial.println("Loadcell initialized!");
}

void setup_pins() {
  
  pinMode(MARM_SW, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(EMATCH, OUTPUT);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  
  delay(100);
}

//Sounds
//Buzzer sounds
void bootup_sound() {
  tone(BUZZER, 1000);
  delay(100);
  tone(BUZZER, 800);
  delay(100);
  tone(BUZZER, 900);
  delay(100);
  tone(BUZZER, 1000);
  delay(100);
  noTone(BUZZER);
}

void beep_once() {
  tone(BUZZER, 2000);
  delay(500);
  noTone(BUZZER);
}

void SAFE_sound() {
  tone(BUZZER, 2000);
  delay(250);
  tone(BUZZER, 2300);
  delay(250);
  noTone(BUZZER);
}

void MARM_sound() {
  tone(BUZZER, 4000);
  delay(500);
  tone(BUZZER, 5000);
  delay(500);
  tone(BUZZER, 6000);
  delay(500);
  noTone(BUZZER);
}

void PRIME_sound() {
  tone(BUZZER, 4000);
  delay(250);
  tone(BUZZER, 3500);
  delay(250);
  tone(BUZZER, 4000);
  delay(500);
  noTone(BUZZER);
}

void FIRE_sound() {
  tone(BUZZER, 4000);
  delay(2000);
  noTone(BUZZER);
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

void wait_for_signal(const char* signal, uint8_t response[], int response_length) {
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

void send_packet(uint8_t data[], int data_length, int football_address)
{
  Serial.print("Sending message to football: ");
  print_byte(data, data_length);

  if (manager.sendtoWait(data, data_length, football_address))
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
    delay(500);
  }
}

void print_byte(uint8_t input[], int data_length) {
  //RH_RF95::printBuffer("Print: ", input, data_length);
  Serial.println((char*)input);
}

void read_data(float start_time) {
  weight = scale.get_units();
  Serial.print((millis() - start_time)/1000);
  Serial.print(", ");
  Serial.println(weight, 1);

  if (((millis() - start_time)/1000) > 180) {
    state = SAFING;
  }
  
}


void setup() {
  setup_radio();

  setup_serial(9600);

  setup_pins();

  setup_loadcell();

  bootup_sound();
}

void loop() {
  responseSuccessful = false;
  
  //CONN_WAIT--------------------------------
  if (state == CONN_WAIT) {
    uint8_t response[] = "BDR CONN CONFIRM";
    int data_length = sizeof(response);
    wait_for_signal("BDR CONN", response, data_length);

    if (received_message("BDR CONN") && responseSuccessful)
    {
      Serial.println("Received connection message!\n");
      state = SAFE;
      digitalWrite(GREEN_LED, HIGH);
    }
    else
    {
      Serial.println("Waiting for connection message...");
      state = CONN_WAIT;
      digitalWrite(GREEN_LED, HIGH);
      beep_once();
      digitalWrite(GREEN_LED, LOW);
    }
  }

  //SAFE--------------------------------
  if (state == SAFE) {
    Serial.println("*STATE: SAFE*");
    Serial.println("Waiting for MARM Switch");
    
    int buttonstate = digitalRead(MARM_SW);
    
    if (buttonstate == HIGH) {
      Serial.println("MARM_SW ARMED");
      
      uint8_t data[] = "BDR MARM";
      data_length = sizeof(data);
      
      Serial.println("Sending MARM signal");
      send_packet(data, data_length, CLIENT_ADDRESS);
      

      if (received_message("BDR MARM CONFIRM") && responseSuccessful)
      {
        Serial.println("MARM confirmed\n");
        state = MARM;
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(YELLOW_LED, HIGH);
        MARM_sound();
      }
      else
      {
        Serial.println("Failed to get MARM signal");
        state = SAFE;
      }
    }
  }

  //MARM--------------------------------
  if (state == MARM) {
    Serial.println("*STATE: MARM*");
    uint8_t response[] = "BDR PRIME CONFIRM";
    data_length = sizeof(response);
    wait_for_signal("BDR PRIME", response, data_length);

    if (received_message("BDR PRIME") && responseSuccessful)
    {
      Serial.println("Received PRIME signal\n");
      state = PRIME;
      digitalWrite(RED_LED, HIGH);
      PRIME_sound();
      digitalWrite(RED_LED, LOW);
    }
    else
    {
      Serial.println("Waiting for PRIME signal");
      state = MARM;
    }
  }

  //PRIME--------------------------------
  if (state == PRIME) {
    Serial.println("*STATE: PRIME*");
    uint8_t response[] = "BDR FIRE CONFIRM";
    data_length = sizeof(response);
    wait_for_signal("BDR FIRE", response, data_length);

    if (received_message("BDR FIRE") && responseSuccessful)
    {
      Serial.println("Received FIRE signal\n");
      state = FIRE;
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      delay(100);
      digitalWrite(RED_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      delay(100);
      digitalWrite(RED_LED, HIGH);
      FIRE_sound();
    }
    else
    {
      Serial.println("Waiting for FIRE signal");
      digitalWrite(YELLOW_LED, LOW);
      delay(100);
      state = PRIME;
      digitalWrite(YELLOW_LED, HIGH);
    }
  }

  //FIRE--------------------------------
  if (state == FIRE) {
    Serial.println("*STATE: FIRE");
    
    digitalWrite(EMATCH, HIGH);
    delay(2000);
    digitalWrite(EMATCH, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    start_time = millis();
    state = COLLECT; 
    Serial.println("COLLECT");
  }

  //COLLECT--------------------------------
  if (state == COLLECT) {
    read_data(start_time);
  }

  //SAFING--------------------------------
  if (state == SAFING) {
    Serial.println("SAFING");
    delay(500);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  }
  
  delay(100);
}
