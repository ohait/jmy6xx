#ifndef JMY6xx_h
#define JMY6xx_h

#define JMY6XX_BUF_SIZE 512
#define byte uint8_t
#include <Stream.h>

class JMY6xx {
  Stream *S;
  int addr = 0;
  
  byte buf[JMY6XX_BUF_SIZE]; // 2 bytes length, 1 addr, 1 cmd, [data]+, chk
  byte* data = buf+4; // helper
  
  // current tag
  byte uid[8];
  byte dsfid;
  
  public:
    JMY6xx(byte i2c_addr); // use i2c
    JMY6xx(Stream *S);
    void setAddress(int addr);
    void info();
    const byte* scan();
    const byte* scan(byte afi);
    const byte* read(int block, int size);
    int quiet();
    int ready(const byte* uid);
    int write(int block, int size, const byte* data);

    int debug = 0;
    int read_timeout = 3000;

  private:
    int _req(const byte cmd, int size);
    void _send(const byte cmd, int size);
    int _recv();
    int _read(); // wait for a single byte
    void hexprint(const byte* data, int len);
    void serialhexdump(const byte* data, int len);
};

#endif
