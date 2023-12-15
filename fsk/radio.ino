#include <RadioLib.h>

// SX1262 has the following connections on Heltec V3:
// NSS pin:   8
// DIO1 pin:  14
// NRST pin:  12
// BUSY pin:  13
SX1262 radio = new Module(8,14,12,13);

//bit-stuffed hdlc packet in AIS packet bit order (bit 0 is on the left)
//bits in each byte are swapped in nrzi() because RadioLib sends left to right (msb of byte 0 first)
byte packet[]={0xaa,0xaa,0xaa,0x7e,0x04,0x57,0x9e,0x5b,0xe1,0x60,0x00,0x06,0x1d,0x0f,
               0x30,0x0b,0x9f,0x1b,0x1d,0x20,0x00,0xf0,0x00,0x80,0x6b,0x1c,0xe1,0xfd,0xfe}; //don't forget the last 0
byte nrziPacket[sizeof(packet)];

void radioSetup() {
  // initialize SX1262 FSK modem with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.beginFSK();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // the following settings can also
  // be modified at run-time
  state = radio.setFrequency(162.025);
  state = radio.setBitRate(9.6);
  state = radio.setFrequencyDeviation(2.4);
  state = radio.setRxBandwidth(58.6);
  state = radio.setOutputPower(-9.0);       //-9 to 22 dBm
  state = radio.setCurrentLimit(100.0);     //about 10 dBm - I think it needs to be 150 for 22 dBm
  state = radio.setDataShaping(RADIOLIB_SHAPING_1_0); //I think GMSK is supposed to be 0_3 but this seems to work
  state = radio.setCRC(0);
  state = radio.setPreambleLength(1);
  uint8_t sync[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  state = radio.setSyncWord(sync, 1);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Unable to set configuration, code: %d\n",state);
    //Serial.print(F("Unable to set configuration, code "));
    //Serial.println(state);
    //while (true);
  }
}

// hdlc
void nrzi(byte * hdlc, int len, byte * nrzi) {
  uint8_t bit;
  uint8_t byte;
  uint8_t sample;

  for (int i=0;i<len;i++){      //least significant byte is first 
    byte=0;
    for (int j=0;j<8;j++){      //first bit is least significant in data - needs to end up on the left
      bit=(hdlc[i]>>j)&1;
      if (bit==0)sample^=0x01;  //flip the bit on 0's
      byte<<=1;                 //shift the sample in, first bit ending up on the left (left to right)
      byte|=sample;
    }
    nrziPacket[i]=byte;
  }
}

void transmitAIS(){
  nrzi(packet,sizeof(packet),nrziPacket);
  int state = radio.transmit(nrziPacket,sizeof(nrziPacket));
  if (state == RADIOLIB_ERR_NONE) Serial.println(F("[SX1262] Packet transmitted successfully!"));
  else Serial.println(state);
}

