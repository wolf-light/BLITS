#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <memory>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 915.0

#define CLIENT_ADDRESS 1  //Football is client
#define SERVER_ADDRESS 2  //Boombox is server

#define CONTROL_SERIAL Serial1
#define CONTROL_BAUDRATE 115200
#define CONTROL_RESPONSE_DELAY 1000

#define DEBUG_SERIAL Serial
#define DEBUG_BAUDRATE 115200

#define MARM_SW 13
int armState = 0;

//Class objects
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHReliableDatagram manager(driver, SERVER_ADDRESS);

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
bool responseSuccessful = false;

bool setup_debug_serial() {
  DEBUG_SERIAL.begin(DEBUG_BAUDRATE);
  while (!DEBUG_SERIAL) {
    ;
  }
  DEBUG_SERIAL.println("debug serial initialized");
  return true;
}

bool setup_control_serial() {
  CONTROL_SERIAL.begin(CONTROL_BAUDRATE);
  while (!CONTROL_SERIAL) {
    ;
  }
  DEBUG_SERIAL.println("control serial initialized");
  return true;
}

bool radio_setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  if (!manager.init()) {
    DEBUG_SERIAL.println("manager init failed");
  }

  while (!driver.setFrequency(RF95_FREQ)) {
    DEBUG_SERIAL.println("freq set failed");
    delay(5);
  }

  manager.setRetries(3);

  driver.setTxPower(23, false);

  DEBUG_SERIAL.println("radio initialized");

  delay(1000);
  return true;
}

void print_byte(uint8_t const byte_arr[], const int& arr_len) {
  for (size_t i = 0; i < arr_len; i++) {
    Serial.print(static_cast<char>(byte_arr[i]));
  }
}

String byte_to_string(uint8_t const byte_arr[], const int& arr_len) {
  String str = "";
  for (size_t i = 0; i < arr_len; i++) {
    str += static_cast<char>(byte_arr[i]);
  }
  return str;
}

void print_string_dst_len(const String& string, const int& address, const int& len) {
  DEBUG_SERIAL.print(string);
  DEBUG_SERIAL.print(" 0x");
  DEBUG_SERIAL.print(address, HEX);
  DEBUG_SERIAL.print(" (");
  DEBUG_SERIAL.print(len);
  DEBUG_SERIAL.print("): ");
}

bool send_packet(uint8_t data[], int data_length, int dst_address) {
  DEBUG_SERIAL.println();
  print_string_dst_len("sending message to", dst_address, data_length);
  print_byte(data, data_length);
  DEBUG_SERIAL.println();

  if (manager.sendtoWait(data, data_length, dst_address)) {
    DEBUG_SERIAL.println("transmission successful");
    return true;
  } else {
    DEBUG_SERIAL.println("transmission failed");
    return false;
  }
}

int recieve_packet(bool& message_recieved, int timeout) {
  message_recieved = false;
  if (manager.waitAvailableTimeout(timeout)) {
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)) {  // note len var declared each call
      print_string_dst_len("recieved message from", from, len);
      print_byte(buf, len);
      message_recieved = true;
      return len;
    } else {
      DEBUG_SERIAL.println("No reply, is rf95_reliable_datagram_server running?");
      return -1;
    }
  } else {
    //DEBUG_SERIAL.println("no rf message");
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

bool recieve_response(String& message, const unsigned long& waitMillis) {
  unsigned long startTime = millis();
  while (CONTROL_SERIAL.available() == 0 && millis() - startTime < waitMillis) {
    ;  // wait for serial to come available
  }

  if (CONTROL_SERIAL.available() > 0) {
    message = Serial.readString();
    return true;
  } else {
    message = "no response recieved";
    return false;
  }
}

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
        // print_both("Fire Start Time Set");
        // print_both_int(fireStart);
        fireState = 0;
        break;
    }
}

void setup() {
  setup_debug_serial();
  setup_control_serial();
  radio_setup();
  pinMode(MARM_SW, INPUT);
}

void checkMARM(){
int buttonstate = digitalRead(MARM_SW);
  if (buttonstate == HIGH) {
    // Serial.println("MARM_SW ARMED");
    if (armState == 0) {
    uint8_t* response = string_to_buf("MARM_SW ARMED: ");
    uint8_t length = 13;
    send_packet(response, length, CLIENT_ADDRESS);
    }
    armState = 1;
  } else {
    // Serial.println("MARM_SW NOT ARMED");
    if (armState == 1) {
      CONTROL_SERIAL.print("safe");
    }
    armState = 0;
  }
}

void loop() {
  responseSuccessful = false;

  if (CONTROL_SERIAL.available() > 0) {
    String serial_message = CONTROL_SERIAL.readString();
    uint8_t* serial_bytes = string_to_buf(serial_message);
    send_packet(serial_bytes, serial_message.length(), CLIENT_ADDRESS);
    delete[] serial_bytes;
  }

  len = recieve_packet(responseSuccessful, 10000);

  if (responseSuccessful) {
    String message = byte_to_string(buf, len);
    checkMARM();
    if ((message == "fire" || message == "yes" || message == "start") && armState == 1) {
      DEBUG_SERIAL.print("Message forwarded: ");
      DEBUG_SERIAL.println(message);
      CONTROL_SERIAL.print(message);  // may have to change to println if that's what control system expects
    } else if ((message == "fire" || message == "yes" || message == "start") && armState == 0) {
      uint8_t* response = string_to_buf("NEGATIVE MARM: ");
      uint8_t length = 15;
      send_packet(response, length, CLIENT_ADDRESS);
    } else {
      CONTROL_SERIAL.print(message);
    }
    // if (message == "read data") {
    //   String sensorData;
    //   sensorData = recieve_response(message, 20000);
    //   Serial.print(sensorData);
    // } else {
      // if (message != "fire"){
        recieve_response(message, 10000);
      // }
    // }

    uint8_t* response = string_to_buf(message);
    uint8_t length = message.length();
    send_packet(response, length, CLIENT_ADDRESS);

    delete[] response;
  }
}
