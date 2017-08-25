# Arduino JMY6xx

This library is meant to be used with JMY6xx modules (like the JMY622 or JMY612 for reading ISO 14443 or ISO 15693 rfid tags)

It supports both UART (by passing a reference to a Stream) ad I2C

```C
#include <SoftwareSerial.h>
SoftwareSerial SSerial(12, 14); // RX, TX to the RFID module
//SoftwareSerial SSerial(0x50); // the I2C address (note below)

#include <jmy6xx.h>;
//JMY6xx jmy622(&SSerial); // pointer to a Stream object, can be SoftSerial or Serial1
JMY6xx jmy622(0x50); // I2C address (default is 0xA0, which is 0x50 in Wire.h)

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

## Supported devices

| device | status |
|--------|--------|
| JMY622 | tested |
| JMY612 | should work |
| JMY620 | ? |
| JMY601 | ? |
| JMY6021 | ? |

## UART

tested with SoftwareSerial, should work with any library that extends Stream

## I2C

JMY622 supports I2C, and even tho the default address in the documentation is 0xA0, you should use **0x50**. 

The Wire library consider only the 7 address bits, so the number is from 0 to 127. But jinmuyu documentation consider the whole byte (so 0xA0 for writing, and 0xA1 for reading). If you changed the defaults, just remember to **divide by 2** when entering the ic2_address in the constructor.

## commands

I'm adding commands as long as I need them, so for now they are not too many.

|code|name|status|comments|
|----|----|------|--------|
|0x10|info()|OK|it also request 0x03 (PCD info) and dump to Serial|
||ISO 15693|||
|0x5C|scan(afi?)|OK|both with and w/o AFI|
|0x5D|quiet()|OK||
|0x5F|ready(uid)|OK||
|0x54|read(first,ct)|OK|reads ct blocks, starting from first (usually 4bytes per block)|


### custom command

this feature might be removed soon.

```
#include <jmy6xx.h>
JMY6xx rfid(0x50);

void setup() {
  Serial.begin(9600);

  rfid.data[0] = 0x12; // AFI
  // .data is RW. the response will overwrite the request
  
  int len = rfid._req(0x5C, 1); // 0x5C is ISO15693 Inventory, 1 is for the lenght of **data**
  if (len) {
    rfid.hexdump(rfid.data, len-4); // len include 4 header bytes
  }
}
```
