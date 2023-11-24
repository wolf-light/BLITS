#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <memory>

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 915.0

#define CLIENT_ADDRESS 1 //Football is client
#define SERVER_ADDRESS 2 //Boombox is server

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
bool responseSuccessful = false;

//Class objects
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHReliableDatagram manager(driver, SERVER_ADDRESS);

bool serial_setup() {
    Serial.begin(9600);
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

void print_byte(uint8_t const byte_arr[], const int& arr_len) {
  for (size_t i = 0; i < arr_len; i++) {
    Serial.print(static_cast<char>(byte_arr[i]));
  }
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
  message_recieved = false;
  if(manager.waitAvailableTimeout(timeout)) {
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from)) { // note len var declared each call
      print_string_dst_len("recieved message from", from, len);
      print_byte(buf, len);
      message_recieved = true;
      return len;
    } else {
      Serial.println("No reply, is rf95_reliable_datagram_server running?");
      return -1;
    }
  } else {
    Serial.println("no message");
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

void setup() {
  serial_setup();
  radio_setup();
}


void loop() {
  responseSuccessful = false;
  len = recieve_packet(responseSuccessful, 60000);

  if (responseSuccessful) {
    send_packet(buf, len, CLIENT_ADDRESS);
  }
}

