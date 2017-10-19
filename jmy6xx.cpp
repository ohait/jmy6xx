#include "jmy6xx.h"
#include <Arduino.h>

#include <Wire.h>

JMY6xx::JMY6xx(Stream *S) {
  this->S = S;
  if (debug>=1) Serial.println("JMY6xx init UART");
}

JMY6xx::JMY6xx(int i2c_addr) {
  this->S = NULL;
	this->i2c_addr = addr;
  if (debug>=1) Serial.println("JMY6xx init I2C");
  Wire.begin();
}

void JMY6xx::setAddress(int addr) {
  this->addr = addr;
}

void JMY6xx::hexprint(Stream *S, const byte* data, int length) {
  if (!data) {
    S->println("NULL");
    return;
  }
  for (int i=0; i<length; i++) {
    if (i==0) S->print(data[i] < 0x10 ? "0" : "");
    else S->print(data[i] < 0x10 ? ":0" : ":");
    S->print(data[i], HEX);
  }
}

void JMY6xx::hexprint(const byte* data, int length) {
//  hexprint(&Serial, data, length);
  if (!data) {
    Serial.println("NULL");
    return;
  }
  for (int i=0; i<length; i++) {
    if (i==0) Serial.print(data[i] < 0x10 ? "0" : "");
    else Serial.print(data[i] < 0x10 ? ":0" : ":");
    Serial.print(data[i], HEX);
  }
}

void JMY6xx::hexdump(Stream *S, const byte* data, int length) {
  if (!data) {
    S->println("  0: NULL POINTER");
    return;
  }
  for (int i=0; i<length; i+=16) {
    delay(1);
    if (i<16) S->print("  ");
    else if (i<256) S->print(" ");
    S->print(i, HEX);
    S->print(":");
    for (int j=i; j<i+16; j++) {
      if (j%16==8) S->print(" ");
      if (j>=length) {
        S->print("   ");
      } else {
        S->print(data[j] < 0x10 ? " 0" : " ");
        S->print(data[j], HEX);
      }
    }
    S->print("  \"");
    for (int j=i; j<length && j<i+16; j++) {
      if (data[j]>=0x20 && data[j]<=0x7E) S->write(data[j]);
      else S->print(".");
    }
    S->println("\"");
  }
}

void JMY6xx::hexdump(const byte* data, int length) {
  hexdump(&Serial, data, length);
}


int JMY6xx::_req(byte cmd, int len) {
  _send(cmd, len);
  int rlen = _recv();
  if (!rlen) return 0;
  if (buf[3] == cmd) return rlen;

  if (buf[3] == 0xA3 and cmd == 0x5C) return 0; // nothing scanned, but no reason to spam
  if (debug>=1) {
    Serial.print("Request fail, expected 0x");
    Serial.print(cmd, HEX);
    Serial.print(" but got 0x");
    Serial.println(buf[3], HEX);
    delay(100);
  }
  return 0;
}

void JMY6xx::_send(byte cmd, int len) {
  if (S) {
    while (S->available()) S->read();
  } else {
    Wire.beginTransmission(i2c_addr);
  }

  if (debug>=3) Serial.println("SENDING");
  len +=4; // 2 length, 1 addr, 1 cmd
  int i;
  buf[1] = len%256;
  buf[0] = len/256;
  buf[2] = addr;
  buf[3] = cmd;
  if (debug>=3) hexdump(buf, len);
  byte chk = 0;
  for (int i=0; i<len; i++) {
    S ? S->write(buf[i]) : Wire.write(buf[i]);
    chk ^= buf[i];
  }
  if (debug>=3) { Serial.print("chk: 0x"); Serial.println(chk, HEX); }
  S ? S->write(chk) : Wire.write(chk);

  if (!S) Wire.endTransmission();
}

int JMY6xx::serial_read(byte* buf, int size) {
  if (debug>=3) {
    Serial.print("_read(buf, ");
    Serial.print(size);
    Serial.println(")");
  }
  long expire = millis() + read_timeout;
  for (int at = 0;;) {
    int r = S->readBytes(buf+at, size-at);
    at += r;
    if (debug>=3) hexdump(buf, at);
    if (at==size) break;
    if (millis() > expire) {
      Serial.println("_read(at, size) TIMEOUT");
      return 0;
    }
    delay(5);
  }
  return 1;
}

int JMY6xx::i2c_read(byte* buf, int size) {
  int len = 0;
  for (int i=0; i<30;i++) {
    int len = Wire.requestFrom(0x50, size);
    if (len) {
      int r = 0;
      for (r=0; r<len; r++)
        buf[r] = Wire.read();
      hexdump(buf, size);
      return r;
    }
    delay(10);
  }
  return 0;
}

int JMY6xx::_recv() {
  if (debug>=3) Serial.println("RECEIVING");

  if (S) {
    if (!serial_read(buf, 2)) {
      Serial.println("frame length read failed");
      return 0;
    }
    int len = buf[0]*256 + buf[1];
    if (debug>=3) Serial.println(String("expecting ")+len+" bytes");
    if (len > JMY6XX_BUF_SIZE) {
      Serial.println("BUFFER OVERFLOW");
      Serial.println(len);
      return 0;
    }
    if (!serial_read(buf+2, len-2+1)) return 0;

    if (debug>=2) hexdump(buf, len+1);

    byte chk = 0;
    byte chk2 = buf[len];
    for (int i=0; i<len; i++) chk ^= buf[i];

    if (chk != chk2) {
      Serial.print("ERROR: checksum mismatch, expected 0x");
      Serial.print(chk, HEX);
      Serial.print(" but got 0x");
      Serial.println(chk2, HEX);
      return 0;
    }
    return len;
  }
  else {
    int pos = 0;
    pos += i2c_read(buf+pos, 64);
    int len = buf[0]*256+buf[1];
    if (pos<len) {
        Serial.println("BUFFER OVERFLOW, INCREASE Wire.h BUFFER_SIZE");
        return 0;
    }

    byte chk = 0;
    for (int i=0; i<len; i++) chk ^= buf[i];

    if (chk != buf[len]) {
        Serial.print("checksum error, expected: 0x");
        Serial.print(chk, HEX);
        Serial.print(" but got: 0x");
        Serial.print(buf[len], HEX);
        Serial.print(", length: ");
        Serial.println(len);

        hexdump(buf, len+1);
        return 0;
    }
    if (debug>=3) hexdump(buf, len+1);
    return len;
  }
}

int JMY6xx::info() {
  if (debug) Serial.println("info()");
  if (!_req(0x10, 0)) {
    Serial.println("Can't find JMY6xx device");
    return 0;
  }
/*
 * 30bytes; 
 * 8bytes product name, 
 * 4bytes firmware version,
 * 8bytes firmware date,
 * 1byte UART baud rate code, 
 * 1byte UART Multi-device communication address,
 * 1byte I²C address, 
 * 1byte multi-card operation enable status,
 * 1byte ISO15693 automatic detecting card AFI,
 * 1byte ISO15693 automatic detecting card AFI enable status, 
 * 1byte automatic detecting card interval,
 * 1byte default automatically detecting card status when power on, 
 * 1byte default automatically output SNR set when power on, 
 * 1byt eRF output power
 */
  Serial.print("                    Product name: ");
  for (int i=0; i<8; i++) Serial.write(data[i]);
  Serial.println();

  Serial.print("                    Firmware Ver: ");
  for (int i=8; i<12; i++) Serial.write(data[i]);
  Serial.println();

  Serial.print("                   Firmware Date: ");
  for (int i=12; i<20; i++) Serial.write(data[i]);
  Serial.println();
  
  Serial.print("             UART baud rate code: 0x");
  hexprint(data+20, 1);
  Serial.println();

  Serial.print("                       UART addr: 0x");
  hexprint(data+21, 1);
  Serial.println();

  Serial.print("                        I²C addr: 0x");
  hexprint(data+22, 1);
  Serial.println();

  Serial.print("               multicard enabled: 0x");
  hexprint(data+23, 1);
  Serial.println();

  Serial.print("        ISO15693 auto detect AFI: 0x");
  hexprint(data+24, 1);
  Serial.println();

  Serial.print("ISO15693 auto detect AFI enabled: 0x");
  hexprint(data+25, 1);
  Serial.println();

  Serial.print("            auto detect interval: 0x");
  hexprint(data+26, 1);
  Serial.println();

  Serial.print("       auto detect when power on: 0x");
  hexprint(data+27, 1);
  Serial.println();

  Serial.print("   auto output SNR when power on: 0x");
  hexprint(data+28, 1);
  Serial.println();

  Serial.print("                 RF output power: 0x");
  hexprint(data+29, 1);
  Serial.println();

  delay(10);

  if (!_req(0x03, 0)) {
    Serial.println("Can't obtain PCD info");
    return 0;
  }

  /*
   * 16 bytes, they are 
   * 5 bytes product identification code, 
   * 3 bytes RFU, 
   * 4 bytes UID, 
   * 3 bytes RFU
   */

  Serial.print("         PCD identification code: ");
  hexprint(data+0, 5);
  Serial.println();

  Serial.print("                         PCD RFU: ");
  hexprint(data+5, 3);
  Serial.println();

  Serial.print("                         PCD UID: ");
  hexprint(data+8, 4);
  Serial.println();

  Serial.print("                        PCD RFU2: ");
  hexprint(data+12, 4);
  Serial.println();

  return 1;
}

int JMY6xx::idle() {
  data[0] = 0x55;
  return _req(0x12,1);
}

const byte* JMY6xx::iso15693_scan() {
  if (debug>=2) Serial.println("scan()");

  //data[0] = 0x00; // AFI
  if (_req(0x5C, 0)>5) {
    for (int i=0; i<8; i++) {
      uid[i] = data[8-i]; // uid, reverse
    }
    dsfid = data[0]; // dsfid
    return uid;
  }
  return NULL;
}

const byte* JMY6xx::iso15693_scan(byte afi) {
  if (debug) {
    Serial.print("scan(afi: 0x");
    Serial.print(afi, HEX);
    Serial.println(")");
  }

  data[0] = afi;
  if (_req(0x5C, 1)>5) {
    for (int i=0; i<8; i++) {
      uid[i] = data[8-i]; // uid, reverse
    }
    dsfid = data[0]; // dsfid
    return data+20;
  }
  return NULL;
}

int JMY6xx::iso15693_info() {
  if (debug) Serial.println("info()");
  int rlen = _req(0x5E, 0);
  if (rlen) {
    hexdump(data, rlen-4);
    return rlen-4;
  } else {
    return 0;
  }
}

int JMY6xx::iso15693_quiet() {
  if (debug) Serial.println("quiet()");
  return _req(0x5D, 0);
}

int JMY6xx::iso15693_ready(const byte* uid) {
  if (debug) {
    Serial.print("ready(uid: ");
    hexprint(uid, 8);
    Serial.println(")");
  }
  for (int i=0; i<8; i++) {
    data[7-i] = uid[i]; // reverse TODO
  }
  if (debug>=3) hexdump(data, 8);
  return _req(0x5F, 0);
}

const byte* JMY6xx::iso15693_read(int block, int ct) {
  if (debug) {
    Serial.print("read(from block: ");
    Serial.print(block);
    Serial.print(", nr blocks: ");
    Serial.print(ct);
    Serial.println(")");
  }
  data[0] = block;
  data[1] = ct;
  if (_req(0x54, 2)) {
    return data;
  } else {
    return NULL;
  }
}

int JMY6xx::iso15693_write(int block, int ct, const byte* buf) {
  if (debug) {
    Serial.print("write(from block: ");
    Serial.print(block);
    Serial.print(", nr blocks: ");
    Serial.print(ct);
    Serial.println(")");
  }
  data[0] = block;
  data[1] = ct;
  memcpy(data+2, buf, ct*4);
  if (_req(0x55, 2+ct*4)) {
    return ct*4;
  } else {
    return 0;
  }
}

//byte inventory[] = { 0x00, 0x04, 0x00, 0x5C };
//byte read_block[] = { 0x00, 0x06, 0x00, 0x54, 0, 9 };
//byte stay_quiet[] = { 0x00, 0x04, 0x00, 0x5d };

