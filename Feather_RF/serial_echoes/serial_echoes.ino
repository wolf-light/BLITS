

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
}

void loop() {
    if (Serial.available() > 0) {
        String message = Serial.readString();
        Serial.println(message);
        Serial1.println(message);
    }
}