#include <SoftwareSerial.h>
SoftwareSerial SSerial(12, 14); // RX, TX to the RFID module

#include <jmy6xx.h>;
JMY6xx jmy622(&SSerial);

void setup() {
  Serial.begin(9600);
  SSerial.begin(19200);
}

void loop() {
  Serial.println("scanning...");
  for(;;) {
    const byte* uid = jmy622.scan();
    if (!uid) break;
    jmy622.read(0,4); // read blocks 0..3
    Serial.print("found tag: ");
    hexprint(uid, 8);
    Serial.println();
    jmy622.quiet(); // set the current tag to quiet, so we can scan others in range
  }
  
  delay(1000);
}

void hexprint(const byte* data, int length) {
  for (int i=0; i<length; i++) {
    if (i==0) Serial.print(data[i] < 0x10 ? "0" : "");
    else Serial.print(data[i] < 0x10 ? ":0" : ":");
    Serial.print(data[i], HEX);
  }
}