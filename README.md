# Arduino JMY6xx

This library is meant to be used with JMY6xx modules (like the JMY622 or JMY612) from jinmuyu.com

it currently support UART, but should be easy to extend to I2C

```C
#include <SoftwareSerial.h>
SoftwareSerial SSerial(12, 14); // RX, TX to the RFID module

#include <jmy6xx.h>;
JMY6xx jmy622(&SSerial);

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
