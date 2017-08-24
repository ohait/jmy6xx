# Arduino JMY6xx

This library is meant to be used with JMY6xx modules (like the JMY622 or JMY612 for reading ISO 14443 or ISO 15693 rfid tags)

it currently support only UART interface:

```C
#include <SoftwareSerial.h>
SoftwareSerial SSerial(12, 14); // RX, TX to the RFID module

#include <jmy6xx.h>;
JMY6xx jmy622(&SSerial); // pointer to a Stream object, can be SoftSerial or Serial1

void setup() {
  SSerial.begin(19200);
}

void loop() {
  for(;;) {
    const byte* uid = jmy622.scan();
    if (!uid) break;
    jmy622.quiet(); // set the current tag to quiet, so we can scan others in range
  }
  delay(1000);
}
```

