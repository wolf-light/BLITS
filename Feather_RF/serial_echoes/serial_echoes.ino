

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
    while (!Serial || !Serial1) {
        ;
    }
    Serial.println("serial ports initialized");
    Serial1.println("serial ports initialized");
}

void loop() {
    if (Serial.available() > 0) {
        String message = Serial.readString();
        Serial.println(message);
        Serial1.println(message);
    }
}