/**
 * This is our new Arduino library to use the 2.4GHz MRF24J40 wireless radio from Microchip.
 * It supports communication using the 802.15.4 wireless protocol at the MAC level, and enables
 * use of the MRF24J40MA/B/C/D/E wireless modules from Microchip.
 *
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
*/
#include <Nanoshield_MRF.h>
#include <SPI.h>

#define MRF_RXMCR    0x00
#define MRF_PANIDL   0x01
#define MRF_PANIDH   0x02
#define MRF_SADRL    0x03
#define MRF_SADRH    0x04
#define MRF_TXNCON   0x1B
#define MRF_PACON2   0x18
#define MRF_TXSTBL   0x2E
#define MRF_GPIO     0x33
#define MRF_TRISGPIO 0x34
#define MRF_RFCTL    0x36
#define MRF_BBREG2   0x3A
#define MRF_BBREG6   0x3E
#define MRF_CCAEDTH  0x3F
#define MRF_RFCON0   0x200
#define MRF_RFCON1   0x201
#define MRF_RFCON2   0x202
#define MRF_RFCON6   0x206
#define MRF_RFCON7   0x207
#define MRF_RFCON8   0x208
#define MRF_SLPCON1  0x220
#define MRF_TESTMODE 0x22F

Nanoshield_MRF::Nanoshield_MRF(Mrf24j40Type type, int cs) {
  if (type == MRF24J40MA) {
    this->paLna = false;
  } else {
    this->paLna = true;
  }

  this->cs = cs;
  this->panId = 0;
  this->srcAddr = 1;
  this->seqNumber = 0;
  this->txCount = 0;
  this->rxSize = 0;
  this->rxCount = 0;
}

void Nanoshield_MRF::begin() {
  pinMode(cs, OUTPUT);
  SPI.setBitOrder(MSBFIRST) ;
  SPI.setDataMode(SPI_MODE0);
  SPI.begin();

  // MRF24J40 module configuration
  writeShort(MRF_PACON2, 0x98);
  writeShort(MRF_TXSTBL, 0x95);
  writeLong(MRF_RFCON0, 0x03);
  writeLong(MRF_RFCON1, 0x01);
  writeLong(MRF_RFCON2, 0x80);
  writeLong(MRF_RFCON6, 0x90);
  writeLong(MRF_RFCON7, 0x80);
  writeLong(MRF_RFCON8, 0x10);
  writeLong(MRF_SLPCON1, 0x21);
  writeShort(MRF_TRISGPIO, 0x08);
  writeShort(MRF_GPIO, 0x08);
  writeShort(MRF_BBREG2, 0x80);
  writeShort(MRF_CCAEDTH, 0x60);
  writeShort(MRF_BBREG6, 0x40);
  setPanId(panId);
  setCoordinator(false);
  setPaLna(paLna);
  setChannel(11);
}

void Nanoshield_MRF::setPanId(uint16_t panId) {
  this->panId = panId;
  writeShort(MRF_PANIDH, panId >> 8);
  writeShort(MRF_PANIDL, panId);
}

void Nanoshield_MRF::setAddress(uint16_t addr) {
  this->srcAddr = addr;
  writeShort(MRF_SADRH, addr >> 8);
  writeShort(MRF_SADRL, addr);
}

void Nanoshield_MRF::setPaLna(bool paLna) {
  this->paLna = paLna;
  uint8_t reg = readLong(MRF_TESTMODE);
  writeLong(MRF_TESTMODE, paLna ? reg | 0b00000111 : reg & ~0b00000111);
}

void Nanoshield_MRF::setCoordinator(bool coord) {
  uint8_t reg = readShort(MRF_RXMCR);
  writeShort(MRF_RXMCR, coord ? reg | 0b00001100 : reg & ~0b00001100);
}

void Nanoshield_MRF::setChannel(int channel) {
  if (channel < 11 || channel > 26) {
    return;
  }

  // Change channel
  writeLong(MRF_RFCON0, (channel - 11) << 4 | 0x03);
  
  // Perform RF state machine reset and wait for RF circuitry to calibrate
  writeShort(MRF_RFCTL, 0x04);
  writeShort(MRF_RFCTL, 0x00);
  delay(1);
}

bool Nanoshield_MRF::write(uint8_t b) {
  return writeToBuffer(&b, 1);
}

uint8_t Nanoshield_MRF::read() {
  uint8_t result;
  return readFromBuffer(&result, 1) ? result : 0;
}

bool Nanoshield_MRF::writeInt(int16_t i) {
  return writeToBuffer(&i, 2);
}

int16_t Nanoshield_MRF::readInt() {
  int16_t result;
  return readFromBuffer(&result, 2) ? result : 0;
}

bool Nanoshield_MRF::writeUnsignedInt(uint16_t u) {
  return writeToBuffer(&u, 2);
}

uint16_t Nanoshield_MRF::readUnsignedInt() {
  uint16_t result;
  return readFromBuffer(&result, 2) ? result : 0;
}

bool Nanoshield_MRF::writeLong(int32_t l) {
  return writeToBuffer(&l, 4);
}

int32_t Nanoshield_MRF::readLong() {
  int32_t result;
  return readFromBuffer(&result, 4) ? result : 0;
}

bool Nanoshield_MRF::writeUnsignedLong(uint32_t u) {
  return writeToBuffer(&u, 4);
}

uint32_t Nanoshield_MRF::readUnsignedLong() {
  uint32_t result;
  return readFromBuffer(&result, 4) ? result : 0;
}

bool Nanoshield_MRF::writeFloat(float f) {
  return writeToBuffer(&f, sizeof(float));
}

float Nanoshield_MRF::readFloat() {
  float result;
  return readFromBuffer(&result, sizeof(float)) ? result : 0;
}

int Nanoshield_MRF::writeBytes(uint8_t* buf, int size) {
  if (size > bytesLeftToWrite()) {
    size = bytesLeftToWrite();
  }

  writeToBuffer(buf, size);
  return size;
}

int Nanoshield_MRF::readBytes(uint8_t* buf, int size) {
  if (size > bytesLeftToRead()) {
    size = bytesLeftToRead();
  }

  readFromBuffer(buf, size);
  return size;
}

int Nanoshield_MRF::writeString(char* str) {
  if (bytesLeftToWrite() == 0) {
    return 0;
  }

  int size = strlen(str);
  
  if (size > bytesLeftToWrite() - 1) {
    size = bytesLeftToWrite() - 1;
  }

  writeToBuffer(str, size);
  write('\0');
  return size + 1;
}

const char* Nanoshield_MRF::readString(char* str, int size) {
  char* result = (char*)&rxBuf[rxCount];

  if (str == NULL) {
    return result;
  }

  if (size > bytesLeftToRead()) {
    size = bytesLeftToRead();
  }
  
  if (size == 0) {
    return 0;
  }

  str[size - 1] = '\0';
  strncpy(str, (char*)&rxBuf[rxCount], size - 1);
  rxCount += strlen(str) + 1;
  return result;
}

void Nanoshield_MRF::startPacket() {
  txCount = 0;
}

inline int Nanoshield_MRF::bytesLeftToWrite() {
  return MRF_MAX_PAYLOAD_SIZE - txCount;
}

bool Nanoshield_MRF::sendPacket(uint16_t addr) {
  if (txCount == 0) {
    return false;
  }

  // Send header size, frame size and header
  int i = 0;
  writeLong(i++, MRF_MHR_SIZE);           // Header size
  writeLong(i++, MRF_MHR_SIZE + txCount); // Frame size
  writeLong(i++, 0b01000001);             // Frame control bits 0-7 (data frame, no security, no pending data, ack disabled, intra-PAN)
  writeLong(i++, 0b10001000);             // Frame control bits 8-15 (16-bit addresses with PAN)
  writeLong(i++, seqNumber++);            // Sequence number
  writeLong(i++, panId);                  // PAN ID
  writeLong(i++, panId >> 8);
  writeLong(i++, addr);                   // Destination address
  writeLong(i++, addr >> 8);
  writeLong(i++, srcAddr);                // Source address
  writeLong(i++, srcAddr >> 8);

  // Send data payload
  for (int j = 0; j < txCount; j++) {
    writeLong(i++, txBuf[j]);
  }

  // Start transmission
  writeShort(MRF_TXNCON, 0b00000001);
  
  return true;
}

bool Nanoshield_MRF::receivePacket() {
  int frameSize;

  // Check RXIF in INTSTAT
  if (readShort(0x31) & 0x08) {
    // Disable interrupts and receiver
    noInterrupts();
    writeShort(0x39, 0x04);
    
    // Packet received, get the number of bytes
    frameSize = readLong(0x300);
    
    // Copy the message bytes into the user buffer
    rxSize = 0;
    rxCount = 0;
    while (rxSize < (frameSize - MRF_MHR_SIZE - MRF_MFR_SIZE)) {
      rxBuf[rxSize] = readLong(0x301 + MRF_MHR_SIZE + rxSize);
      rxSize++;
    }
  
    // Flush the reception buffer, re-enable interrupts and receiver
    writeShort(0x0D, 0x01);
    writeShort(0x39, 0x00);
    interrupts();
    
    // Wait until RXIF is cleared (takes a while)
    while(readShort(0x31) & 0x08);
    
    return true;
  }
  
  return false;
}

int Nanoshield_MRF::bytesLeftToRead() {
  return rxSize - rxCount;
}

uint8_t Nanoshield_MRF::readShort(uint8_t addr) {
  digitalWrite(cs, LOW);
  SPI.transfer(addr << 1 & 0b01111110);
  uint8_t result = SPI.transfer(0);
  digitalWrite(cs, HIGH);
  return result;
}

void Nanoshield_MRF::writeShort(uint8_t addr, uint8_t data) {
  digitalWrite(cs, LOW);
  SPI.transfer((addr << 1 & 0b01111110) | 0b00000001);
  SPI.transfer(data);
  digitalWrite(cs, HIGH);
}

uint8_t Nanoshield_MRF::readLong(uint16_t addr) {
  digitalWrite(cs, LOW);
  SPI.transfer(addr >> 3 | 0b10000000);
  SPI.transfer(addr << 5);
  uint8_t result = SPI.transfer(0);
  digitalWrite(cs, HIGH);
  return result;
}

void Nanoshield_MRF::writeLong(uint16_t addr, uint8_t data) {
  digitalWrite(cs, LOW);
  SPI.transfer(addr >> 3 | 0b10000000);
  SPI.transfer(addr << 5 | 0b00010000);
  SPI.transfer(data);
  digitalWrite(cs, HIGH);
}

bool Nanoshield_MRF::writeToBuffer(void* data, int size) {
  if (txCount > bytesLeftToWrite()) {
    return false;
  }

  memcpy(&txBuf[txCount], data, size);
  txCount += size;
  return true;
}

bool Nanoshield_MRF::readFromBuffer(void* data, int size) {
  if (size > bytesLeftToRead()) {
    return false;
  }

  memcpy(data, &rxBuf[rxCount], size);
  rxCount += size;
  return true;
}
