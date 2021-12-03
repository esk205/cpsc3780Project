#include "SimpleHeader.h"
#include <iostream>

SimpleHeader::SimpleHeader() {
  // silly code, you can do better
  packet.header[0] = packet.header[1] = packet.header[2] = packet.header[3] = 0;
}

void SimpleHeader::setPayloadLength(unsigned int val) {
  packet.header[PL] = (val>>8); // shift the integer to right by 8 bits to get the msb
  packet.header[PL+1] = (val&255); // bitwise AND with 8 LSB bits set to 1.
}

unsigned int SimpleHeader::getPayloadLength() const {
  return packet.header[PL+1] | (packet.header[PL]<<8);
}

unsigned int SimpleHeader::getVal() const {
  return (packet.header[VL] >> 6);
}

void SimpleHeader::setVal(unsigned int val) {
  // clear the val first
  packet.header[VL] &= 0x3f;

  // set the lowest 2 bits of val to the header field, but do not disturb the other bits
  packet.header[VL] |= (val << 6);
}

unsigned int SimpleHeader::getType() const {
  // packet.header[0] >> 6
  return (packet.header[VL] >> 6);
}

void SimpleHeader::setType(unsigned int type) {
  // clear the val first
  packet.header[VL] &= 0x3f; // six 1's

  // set the lowest 2 bits of val to the header field, but do not disturb the other bits
  packet.header[VL] |= (type << 6);
}

unsigned int SimpleHeader::getTR() const {
  // return packet.header[0] >> 5
  return (packet.header[indTr] >> 5);
}

void SimpleHeader::setTR(unsigned int tr) {
  // clear the val first
  packet.header[indTr] &= 0x2B67; // five 1's

  // set the lowest 2 bits of val to the header field, but do not disturb the other bits
  packet.header[indTr] |= (tr << 5);
}

unsigned int SimpleHeader::getWindow() const {
  return (packet.header[indTr] >> 4);
}

void SimpleHeader::setWindow(unsigned int window) {
  // clear the val first
  packet.header[Win] &= 0x457; // four 1's

  // set the lowest 2 bits of val to the header field, but do not disturb the other bits
  packet.header[Win] |= (window << 4);
  // OBS: this implementation contains an error. It does not clear the header field if the value is > 3. Add a test for this case, see it fail, then fix the error
}
unsigned int SimpleHeader::getSeqNum() const {
  return (packet.header[indSeqNum] >> 3);
}

void SimpleHeader::setSeqNum(unsigned int seqNum) {
  // clear the val first
  packet.header[indSeqNum] &= 0x7; // three 1's

  // set the lowest 2 bits of val to the header field, but do not disturb the other bits
  packet.header[indSeqNum] |= (seqNum << 3);
  // OBS: this implementation contains an error. It does not clear the header field if the value is > 3. Add a test for this case, see it fail, then fix the error
}

char SimpleHeader::getPayload(int index){
 return packet.data[index];
}
char* SimpleHeader::getEntirePayload(){
  return packet.data;
}

void SimpleHeader::setPayload(char payloadVal, int index) {
// set character 8 bits 5 header positions
// do not need to clear payload
   packet.data[index] = payloadVal;
}
void SimpleHeader::setEntirePayload(char* payloadVal, unsigned int size) {
   char* payloadValue = reinterpret_cast<char*>(payloadVal);
   for(int i = 0; i < size; i++){
     packet.data[i] = *payloadValue;
   }
}
