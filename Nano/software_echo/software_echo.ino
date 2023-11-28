
#include <SoftwareSerial.h>

SoftwareSerial Serial1(5, 6);

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    while (!Serial || !Serial1) {
        ;
    }
    Serial.println("serial ports initialized");
}

void loop() {
    if (Serial1.available() > 0) {
        String message = Serial1.readString();
        Serial.print(message);
        Serial1.print(message);
    }
}