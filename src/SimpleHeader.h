#ifndef __SIMPLEHEADER_H
#define __SIMPLEHEADER_H

#include <cstdint>
// maximum size for the payload
#define DATA_SZ 1024
// size of header
#define HEADER_SZ 4

struct simplepacket {
  uint8_t header[HEADER_SZ]; // Each position in the array is 8 bits
  char data[DATA_SZ];  // payload
};

// exports routines to get and set the header parameters val (integer between 0 and 3) and payload_length (16 bit unsigned integer)
//  0 1   2    3   7 8     15 16      31
// +---+-----+-----+---------+---------+
// |Type| TR  | Win |  Seq   | Length
// |    |     | dow |  Num   |
// +---+-----+-----+---------+---------+

class SimpleHeader {
private:
  struct simplepacket packet;

  // start index of the payload length field (private)
  const int PL=2;

  // start index of val header field
  const int VL=0;

  // start index of TR field
  const int indTr = 0;

  // start index of Window field
  const int Win = 0;

  // start index of seqNum
  const int indSeqNum = 1;

  // start index of payload
  const int indPL = 0;

public:
  // default constructor initializes the header to zero.
  SimpleHeader();

  // sets the value of the payload length
  // val = length; if val > DATA_SZ, the value set is DATA_SZ
  void setPayloadLength(unsigned int val);

  // returns the length of the payload
  unsigned int getPayloadLength() const;

  // returns the val field of the header. Must be between 0..3 since
  // it is a 2 bit value
  unsigned int getVal() const;

  // sets the value field of the header.
  // If the val is not between 0..3, the value set is 0
  void setVal(unsigned int val);

   // returns the val field of the header. Must be between 0..3 since
  //  it is a 2 bit value
  unsigned int getType() const;

  // sets the value field of the header.
  // If the type is not between 0..3, the value set is 0
  void setType(unsigned int type);

  // Retrieves the value of TR (1 bit value)
  unsigned int getTR() const;

  // Sets the value of TR to a specified value
  void setTR(unsigned int tr);

  // Retrieves the value of Window (8 bit value)
  unsigned int getWindow() const;

  // Sets the value of Window to a specified value
  void setWindow(unsigned int tr);

  // Retrieves the value of SeqNum (1 bit value)
  unsigned int getSeqNum() const;

  // Sets the value of SeqNum to a specified value
  void setSeqNum(unsigned int seqNum);

  // Retrieves the value of payload (8 bit char value)
  char getPayload(int index);

  char* getEntirePayload();

  // Sets the value of payload to a specified char value at a certain index
  void setPayload(char payloadVal, int index);

  // Sets the value of payload to a specified char value at a certain index
  void setEntirePayload(char* payloadVal, unsigned int size);

  // returns the size of the packet, including headers and data
  // to be used with recvfrom() or sendto()
  unsigned int totalPacketSize() const {
    return getPayloadLength() + HEADER_SZ;
  }

  // returns pointer to the structure holding the thePacket, including the headers
  // To be used with recvfrom or sendto
  void * thePacket() {
    return &packet;
  }

  void * thePayload() {
    return packet.data;
  }
};

#endif
