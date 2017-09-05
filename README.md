# Arduino JMY6xx

This library is meant to be used with JMY6xx modules (like the JMY622 or JMY612 for reading ISO 14443 or ISO 15693 rfid tags)

It supports both UART (by passing a reference to a Stream, can be Serial1, SoftwareSerial, etc) and I2C:

```C
#include <SoftwareSerial.h>
SoftwareSerial SSerial(12, 14); // RX, TX to the RFID module

#include <jmy6xx.h>;
JMY6xx jmy622(&SSerial); // pointer to SoftwareSerial
//JMY6xx jmy622(&Serial1); // pointer to Serial1
//JMY6xx jmy622(0x50); // I2C address (read the I2C paragraph for important notes)

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

Also, the Wire.h library force a max buffer size of **32 bytes**. When the library issue a requestForm() for more than 32 bytes, Wire.h simply downgrade it to 32. This is not supported by JMY6xx modules, so you can't just fetch data in two rounds.

This has caused me quite some headache with .info() and .iso15693_read(). You can modify Wire.h to allow more than 32 bytes to circumvent it.

## commands

I'm adding commands as long as I need them, so for now they are not too many.

### Constructor

#### `JMY6xx(Stream* Stream)`

Construct a JMY6xx object which use the given `Stream` to send and receive data to the module.

The stream must be initiated and setup (`begin(38400)` is the default)

### Generic

#### `setAddress(int addr)`

set the device address, default is 0 (broadcast)

Note: this is the device address, not the I2C address. Keep it to 0 unless you use the same wires to talk with distinct modules.

#### `hexprint(Stream *S, const byte* data, int len)`
#### `hexprint(const byte* data, int len)`
#### `hexdump(Stream *S, const byte* data, int len)`
#### `hexdump(const byte* data, int len)`

hexdump the given data to the give Stream, or Serial if not specified.

`hexprint` write all the values in hex separated by colon. e.g.: `E0:11:22:33:44:55:66:77'

`hexdump` is better for long data, it prints several lines of 8 bytes each. e.g.:
```
  0: 4A 4D 59 36 32 32 48 20  "JMY622H "
  8: 35 2E 37 35 32 30 31 35  "5.752015"
 16: 31 31 33 30 00 01 A0 01  "1130...."
 24: 00 00 0A 00 00 00        "......"
```

#### `info()`

Dumps on Serial a list of information retrieved via 0x10 and 0x03

### ISO 15693

#### 0x5C `const char* scan()` and `const char* scan(const char* afi)`

SCan for new tags in range.

If there is at least one tag present, then the tag is marked as `current` and the `uid` is returned.

the optional `afi` argument can be used to limit to tags which the corresponding afi value

Note: `memcpy` the returned value, or it will be overwritten by the next call

#### 0x5D `quiet()`

set the current tag (if any) to *Stay Quiet*

#### 0x5F `ready(const char* uid)`

reset a tag to *Ready* which was previously set to *Stay Quiet*

#### 0x54 `const char* read(int first, int count)`

if the current tag is still in range, read `count` blocks starting from `first`

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
