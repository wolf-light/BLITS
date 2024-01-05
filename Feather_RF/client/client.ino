#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <memory>

// /////// NFC AUTHORIZATION TEST IMPLEMENTATION ///////
// // #include <Wire.h>
// #include <PN532_I2C.h>
// #include <PN532.h>
// #include <NfcAdapter.h>
// PN532_I2C pn532_i2c(Wire);
// NfcAdapter nfc = NfcAdapter(pn532_i2c);
// String tagId = "None";
// byte nuidPICC[4];
// //---// NFC AUTHORIZATION TEST IMPLEMENTATION //---//

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 915.0

#define CLIENT_ADDRESS 1  //Football is client
#define SERVER_ADDRESS 2  //Boombox is server

#define PRIME_SW 10
#define FIRE_SW 11

#define RED_LED A3  //A0 pin was not working properly on the board
#define GREEN_LED A1
#define BLUE_LED A2
#define BUTTON_LED 12

//Class objects
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
bool responseSuccessful = false;
bool pckg_send = false;

enum STATE {
  SAFE,
  MARM,
  PRIME,
  FIRE
};

STATE state = SAFE;
bool armState = false;
String recvdata = "";

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

bool serial_setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("serial initialized");
  return true;
}

bool radio_setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  if (!manager.init()) {
    Serial.println("manager init failed");
  }

  while (!driver.setFrequency(RF95_FREQ)) {
    Serial.println("freq set failed");
    delay(5);
  }

  manager.setRetries(3);

  driver.setTxPower(23, false);

  Serial.println("radio initialized");

  delay(1000);
  return true;
}

String print_byte(uint8_t const byte_arr[], const int& arr_len) {
  String recvdata = "Received data: ";
  for (size_t i = 0; i < arr_len; i++) {
    Serial.print(static_cast<char>(byte_arr[i]));
    recvdata += static_cast<char>(byte_arr[i]);
  }
  return recvdata;
}


void print_string_dst_len(const String& string, const int& address, const int& len) {
  Serial.print(string);
  Serial.print(" 0x");
  Serial.print(address, HEX);
  Serial.print(" (");
  Serial.print(len);
  Serial.print("): ");
}

bool send_packet(uint8_t data[], int data_length, int dst_address) {
  Serial.println();
  print_string_dst_len("sending message to", dst_address, data_length);
  print_byte(data, data_length);
  Serial.println();

  if (manager.sendtoWait(data, data_length, dst_address)) {
    Serial.println("transmission successful");
    return true;
  } else {
    Serial.println("transmission failed");
    return false;
  }
}

int recieve_packet(bool& message_recieved, int timeout) {
  recvdata = "";
  message_recieved = false;
  if (manager.waitAvailableTimeout(timeout)) {
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)) {  // note len var declared each call
      print_string_dst_len("recieved message from", from, len);
      recvdata += print_byte(buf, len);
      message_recieved = true;
      return len;
    } else {
      Serial.println("No reply, is rf95_reliable_datagram_server running?");
      return -1;
    }
  } else {
    //Serial.println("no message");
    return -1;
  }
}

uint8_t* string_to_buf(const String& string) {
  uint8_t* data = new uint8_t[string.length() + 1];
  for (size_t i = 0; i < string.length(); i++) {
    data[i] = static_cast<uint8_t>(string[i]);
  }
  return data;
}

void proccess_current_state() {
  switch (state) {
    case STATE::SAFE:
      LED_reset();
      // LED_write("RED", 170);
      LED_write("GREEN", 190);
      armState = false;
      break;
    case STATE::MARM:
      armState = false;
      break;
    case STATE::PRIME:
      LED_reset();
      LED_write("RED", 170);
      LED_write("GREEN", 190);
      armState = true;
      break;
    case STATE::FIRE:
      LED_reset();
      LED_write("RED", 170);
      LED_write("GREEN", 255);
      LED_write("BLUE", 255);
      digitalWrite(BUTTON_LED, HIGH);
      break;
  }
}

// /////// REMOVE IF STUPID ///////

// int nfc_fire_auth = 0;

// void readNFC() {
//   if (nfc.tagPresent()) {
//     NfcTag tag = nfc.read();
//     tagId = tag.getUidString();

//     if (tagId == "73 A7 E0 F4") {
//       nfc_fire_auth = 1;
//     } else {Serial.print("INVALID SYS AUTH");}
//   } else {Serial.print("NEGATIVE NFC KEYCARD");}
// }

// //---// REMOVE IF STUPID //---//

void err_flash() {
  Serial.println();
  Serial.println("FOOTBALL: NEGATIVE MARM or FIRE SWITCH");
  LED_reset();
  LED_write("RED", 170);
  LED_write("GREEN", 190);
  digitalWrite(BUTTON_LED, LOW);
  delay(200);
  LED_reset();
  digitalWrite(BUTTON_LED, HIGH);
  delay(200);
  LED_write("RED", 170);
  LED_write("GREEN", 190);
  digitalWrite(BUTTON_LED, LOW);
  delay(200);
  LED_reset();
  digitalWrite(BUTTON_LED, HIGH);
  proccess_current_state();
}

void setup() {
  serial_setup();
  radio_setup();
  setup_pins();

  LED_reset();
  // LED_write("RED", 170);
  LED_write("GREEN", 190);

  // nfc.begin();  /////// REMOVE IF STUPID //---//
}

void safe_to_marm(uint8_t* data, String message) {
  int buttonstate = digitalRead(PRIME_SW);
  if (buttonstate == HIGH) {
    state = MARM;
    pckg_send = true;
  } else {
    err_flash();
  }
}

void marm_to_prime(uint8_t* data, String message) {
  int buttonstate = digitalRead(PRIME_SW);
  if (buttonstate == HIGH) {
    // readNFC();
    // if (nfc_fire_auth == 1) {
    state = PRIME;
    proccess_current_state();
    pckg_send = true;
    // }
  } else {
    err_flash();
  }
}

void prime_to_fire(uint8_t* data, String message) {
  if (armState) {
    int buttonstate = digitalRead(PRIME_SW);
    if (buttonstate == HIGH) {
      int buttonstate = digitalRead(FIRE_SW);
      if (buttonstate == HIGH) {
        state = FIRE;
        proccess_current_state();
        pckg_send = true;
      } else {
        err_flash();
      }
    } else {
      err_flash();
    }
  }
}

void loop() {
  responseSuccessful = false;
  uint8_t* data;
  pckg_send = false;

  if (Serial.available() > 0) {
    String message = Serial.readString();

    if (message == "start") {
      safe_to_marm(data, message);
      if (pckg_send) {
        data = string_to_buf(message);
      }
    } else if (message == "yes") {
      marm_to_prime(data, message);
      if (pckg_send) {
        data = string_to_buf(message);
      }
    } else if (message == "fire") {
      prime_to_fire(data, message);
      if (pckg_send) {
        data = string_to_buf(message);
      }
    } else if (message == "safe") {
      pckg_send = true;
      data = string_to_buf(message);
    } else {
      pckg_send = true;
      data = string_to_buf(message);
    }





    if (pckg_send && send_packet(data, message.length(), SERVER_ADDRESS)) {
      len = recieve_packet(responseSuccessful, 10000);
    }
    delete[] data;
  }

  recieve_packet(responseSuccessful, 1);

  // Serial.println("");
  // Serial.println(recvdata);

  // if (recvdata == "(1)SAFE, enter \"start\" to arm system") {
  //   state = SAFE;
  //   proccess_current_state();
  // } else if (recvdata == "(2)ARM, enter \"yes\" to prime system") {
  //   state = MARM;
  //   proccess_current_state();
  // } else if (recvdata == "(3)PRIME, enter \"fire\" to fire") {
  //   state = PRIME;
  //   proccess_current_state();
  // } else if (recvdata == "10 SECOND ABORT") {
  //   state = FIRE;
  //   proccess_current_state();
  // } else if (recvdata == "NEGATIVE MARM: recieved message from 0x2 (20): no response recieved") {
  //   err_flash();
  // }
}
