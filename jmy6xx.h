#ifndef JMY6xx_h
#define JMY6xx_h

#define JMY6XX_BUF_SIZE 64
#define byte uint8_t
#include <Stream.h>

class JMY6xx {
  Stream *S;
  int addr = 0;
  int i2c_addr = 0x50; // default
  
  byte buf[JMY6XX_BUF_SIZE]; // 2 bytes length, 1 addr, 1 cmd, [data]+, chk
  
  // current tag
  byte uid[8];
  byte dsfid;
  
  public: 
    // TODO this should be private when library is complete
    virtual int _req(const byte cmd, int size);
    byte* data = buf+4;


    JMY6xx(Stream *S);
    JMY6xx(int addr); // use i2c

    void        setAddress(int addr);
    int         info();
    int         idle();

    const byte* iso15693_scan();
    const byte* iso15693_scan(byte afi);
    int         iso15693_info();
    const byte* iso15693_read(int block, int count);
    int         iso15693_quiet();
    int         iso15693_ready(const byte* uid);
    int         iso15693_write(int block, int count, const byte* buf);

    void hexprint(Stream *S, const byte* data, int len);
    void hexdump(Stream *S, const byte* data, int len);
    void hexprint(const byte* data, int len);
    void hexdump(const byte* data, int len);

    int debug = 0;
    int read_timeout = 3000;

  private:
    void _send(const byte cmd, int size);
    int _recv();
    int serial_read(byte* at, int size);
    int i2c_read(byte* at, int size);
};





#endif
