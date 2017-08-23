#include "jmy6xx.h"
#include <Arduino.h>
//#include <SoftwareSerial.h>
//SoftwareSerial SSerial(12,14); 

//JMY6xx::JMY6xx(int tx, int rx) {
//  SSerial.begin(19200);
//  this->S = &SSerial;
//  Serial.println("JMY6xx init");
//}

#include <Wire.h>

JMY6xx::JMY6xx(Stream *S) {
  this->S = S;
  Serial.println("JMY6xx init UART");
}

JMY6xx::JMY6xx(byte i2c_addr) {
  this->S = NULL;
  addr = i2c_addr;
  Serial.println("JMY6xx init I2C");
}

void JMY6xx::setAddress(int addr) {
  this->addr = addr;
}

void JMY6xx::hexprint(const byte* data, int length) {
  for (int i=0; i<length; i++) {
    if (i==0) Serial.print(data[i] < 0x10 ? "0" : "");
    else Serial.print(data[i] < 0x10 ? ":0" : ":");
    Serial.print(data[i], HEX);
  }
}

void serialhexdump(const byte* data, int length) {
  for (int i=0; i<length; i+=8) {
    if (i<10) Serial.print("  ");
    else if (i<100) Serial.print(" ");
    Serial.print(i);
    Serial.print(":");
    for (int j=i; j<i+8; j++) {
      if (j>=length) {
        Serial.print("   ");
      } else {
        Serial.print(data[j] < 0x10 ? " 0" : " ");
        Serial.print(data[j], HEX);
      }
    }
    Serial.print("  \"");
    for (int j=i; j<length && j<i+8; j++) {
      if (data[j]>=0x20 && data[j]<=0x7E) Serial.write(data[j]);
      else Serial.print(".");
    }
    Serial.println("\"");
  }
}

int JMY6xx::_req(byte cmd, int len) {
  if (!S) Wire.beginTransmission(addr);
  _send(cmd, len);
  int rlen = _recv();
  if (!S) Wire.endTransmission();
  
  if (buf[3] == cmd) return 1;
  if (buf[3] == 0xA3 and cmd == 0x5C) return 0; // nothing scanned, but no reason to spam
  if (debug>=2) {
    Serial.print("Request fail, expected 0x");
    Serial.print(cmd, HEX);
    Serial.print(" but got 0x");
    Serial.println(buf[3], HEX);
  }
  return 0;
}

void JMY6xx::_send(byte cmd, int len) {
  if (debug>=3) Serial.println("SENDING");
  len +=4; // 2 length, 1 addr, 1 cmd
  int i;
  buf[1] = len%256;
  buf[0] = len/256;
  buf[2] = addr;
  buf[3] = cmd;
  if (debug>=3) serialhexdump(buf, len);
  byte chk = 0;
  for (int i=0; i<len; i++) {
    S ? S->write(buf[i]) : Wire.write(buf[i]);
    chk ^= buf[i];
  }
  if (debug>=3) { Serial.print("chk: 0x"); Serial.println(chk, HEX); }
  S->write(chk);
}

int JMY6xx::_read() {
  if (S) {
    for (int i=0; i<read_timeout/5; i++) {
      if (S->available()) {
        return S->read();
      }
      delay(5);
    }
  } else {
    for (int i=0; i<read_timeout/5; i++) {
      if (Wire.available()) {
        return Wire.read();
      }
      delay(5);
    }
  }
  Serial.println(String("")+"ERROR: TIMEOUT while reading. Abort without waiting further");
  return -1;
}

int JMY6xx::_recv() {
  if (debug>=3) Serial.println("RECEIVING");
  int i;
  byte chk = 0;
  chk ^= buf[0] = _read();
  if (buf[0]==255) {
    Serial.println("TIMEOUT");
    return 0;
  }
  chk ^= buf[1] = _read();
  int len = buf[0]*256+buf[1];
  if (len>JMY6XX_BUF_SIZE) {
    Serial.println(String("")+"ERROR: must read "+len+" bytes, but max "+JMY6XX_BUF_SIZE+" available. Abort without reading");
    return 0;
  }
  for (int i=2; i<len; i++) {
    chk ^= buf[i] = _read();
  }
  if (debug>=3) serialhexdump(buf, len);
  byte chk2 = buf[len] = _read();
  if (chk != chk2) {
    Serial.print("ERROR: checksum mismatch, expected 0x");
    Serial.print(chk, HEX);
    Serial.print(" but got 0x");
    Serial.println(chk2, HEX);
    return 0;
  }
  return len;
}

void JMY6xx::info() {
  if (debug) Serial.println("info()");
  _send(0x10, 0);
  if (!_recv()) {
    Serial.println("Can't find JMY6xx device");
    return;
  }
  _send(0x03, 0);
  _recv();
}

const byte* JMY6xx::scan() {
  if (debug) Serial.println("scan()");

  //data[0] = 0x00; // AFI
  if (_req(0x5C, 0)) {
    serialhexdump(data, 9);
    for (int i=0; i<8; i++) {
      uid[i] = data[8-i]; // uid, reverse
    }
    dsfid = data[0]; // dsfid
    return uid;
  }
  return NULL;
}

const byte* JMY6xx::scan(byte afi) {
  if (debug) {
    Serial.print("scan(afi: 0x");
    Serial.print(afi, HEX);
    Serial.println(")");
  }

  data[0] = afi;
  if (_req(0x5C, 1)) {
    for (int i=0; i<8; i++) {
      uid[i] = data[8-i]; // uid, reverse
    }
    dsfid = data[0]; // dsfid
    return data+20;
  }
  return NULL;
}

int JMY6xx::quiet() {
  if (debug) Serial.println("quiet()");
  _send(0x5D, 0);
  _recv();
  return buf[3] == 0x5D;
}

int JMY6xx::ready(const byte* uid) {
  if (debug) {
    Serial.print("ready(uid: ");
    hexprint(uid, 8);
    Serial.println(")");
  }
  for (int i=0; i<8; i++) {
    data[7-i] = uid[i]; // reverse TODO
  }
  return _req(0x5F, 0);
}

const byte* JMY6xx::read(int block, int ct) {
  if (debug) {
    Serial.print("read(from block: ");
    Serial.print(block);
    Serial.print(", nr blocks: ");
    Serial.print(ct);
    Serial.println(")");
  }
  return NULL;
}

//byte inventory[] = { 0x00, 0x04, 0x00, 0x5C };
//byte read_block[] = { 0x00, 0x06, 0x00, 0x54, 0, 9 };
//byte stay_quiet[] = { 0x00, 0x04, 0x00, 0x5d };

