/*
 * Sends AIS messages via Heltec WiFi LoRa module
*/

#include "Arduino.h"
#include "heltec.h"
#include "WiFi.h"
#include "images.h"
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
byte nrziPacket[29];

void setup()
{
	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);

	logo();
	delay(3000);
	Heltec.display -> clear();

  radioLibSetup();
  nrzi(packet,sizeof(packet),nrziPacket);
}

void loop()
{
  static uint32_t count;
  static char buf[80];

  // transmit FSK packet
  int state = radio.transmit(nrziPacket,sizeof(nrziPacket));
  if (state == RADIOLIB_ERR_NONE) Serial.println(F("[SX1262] Packet transmitted successfully!"));
  else Serial.println(state);
  
  sprintf(buf,"count %d",count++);
	Heltec.display -> clear();
  Heltec.display -> drawString(0, 20, "Sending AIS transmissions");
  Heltec.display -> drawString(0, 30, buf);
  Heltec.display -> display();

  delay(10000);
 }

void logo(){
	Heltec.display -> clear();
	Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
	Heltec.display -> display();
}

void radioLibSetup() {
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
  state = radio.setOutputPower(-9.0);
  state = radio.setCurrentLimit(100.0);
  state = radio.setDataShaping(RADIOLIB_SHAPING_1_0);
  state = radio.setCRC(0);
  state = radio.setPreambleLength(1);
  state = radio.setSyncWord((uint8_t)0xcc, 8); //seems not to like this
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print(F("Unable to set configuration, code "));
    Serial.println(state);
    //while (true);
  }
}

void nrzi(byte * hdlc, int len, byte * nrzi) {
  uint8_t bit;
  uint8_t byte;
  uint8_t sample;

  for (int i=0;i<len;i++){ //least significant byte is first 
    byte=0;
    for (int j=0;j<8;j++){ //first bit is least significant in data - needs to end up on the left
      bit=(hdlc[i]>>j)&1;
      sample=bit?sample:sample^0x01;
      byte=byte<<1;         //shift the sample in, first bit ending up on the left (left to right)
      byte=byte|sample;
    }
    nrziPacket[i]=byte&0xff;
  }
}


