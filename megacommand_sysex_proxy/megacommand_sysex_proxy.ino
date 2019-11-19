// A proxy that converts between midi/arduino usb-com data.

uint8_t serial_read;

void setup() {
  Serial1.begin(31250);
  Serial.begin(250000);
}

void loop() {

  if (Serial.available() > 0) {
    serial_read = Serial.read();
    Serial1.write(serial_read);
  }
  
  if (Serial1.available() > 0) {
    serial_read = Serial1.read();
    Serial.write(serial_read);
  }
}
