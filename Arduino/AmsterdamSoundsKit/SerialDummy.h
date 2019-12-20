/**

 Serial port dummy for easily removing all serial port communication for sure by

*/

#ifndef SERIAL_DUMMY_H
#define SERIAL_DUMMY_H

class SerialDummy : public Stream {
  public:
  void begin(...) {}
  void println(...) {}
  void println(int i) {}
  void print(...) {}
  void printf(const char format[], ...) {}
  operator bool() { return true; }
  int available() { return 0; }
  int read() { return 0; }
  int peek() { return 0; }
  size_t write(uint8_t s) { return 0; }
};

SerialDummy dummy = SerialDummy();

#endif /* SERIAL_DUMMY_H */
