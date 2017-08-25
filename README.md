# Arduino JMY6xx

This library is meant to be used with JMY6xx modules (like the JMY622 or JMY612 for reading ISO 14443 or ISO 15693 rfid tags)

it currently support only UART interface:

```C
#include <SoftwareSerial.h>
SoftwareSerial SSerial(12, 14); // RX, TX to the RFID module
//SoftwareSerial SSerial(0x50); // the I2C address (note below)

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

## I2C

JMY622 supports I2C, and the default address in the documentation is 0xA0.

You should use 0x50. the Wire library consider only the 7 address bits, so the number is from 0 to 127. But jinmuyu documentation consider the whole byte (so 0xA0 for writing, and 0xA1 for reading). If you changed the defaults, just remember to divide it by 2 when entering the ic2_address in the constructor.
