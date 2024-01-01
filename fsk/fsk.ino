/*
 * Sends and receives AIS messages via Heltec WiFi LoRa module
 * Todo:
 * Receive is not working very well. It seems the sensitivity is poor. Even
 * when the transmitter is very close, it misses a lot of packets. 
 * Perhaps turning off preamble is reducing packet decoding percentage
 * Perhaps AIS packets are being split due to raw packet timeout
 * Might try continuously reading the module's buffer directly
 
*/
#include "Arduino.h"
#include "heltec.h"
#include "crc16.h"
#include "radio.h"

float    LAT=   20.7664;  //Punta Mita anchorage
float    LON= -105.5131;   
uint32_t MMSI= 367499000;

void setup()
{
 	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  radioSetup();
  initCRC();

  if (tests()) Serial.println("all ais tests ran successfully");
  else {
    Serial.println("something is broken - stopped");
    while (true) delay(1000);
  }
}

void loop()
{
  static uint32_t count;
  static char buf1[80],buf2[80];
  static bool transmit=false;

  char c = Serial.read();
  if (c=='t') {
    transmit=true;
    Serial.println("transmit mode");
  }
  if (c=='r') {
    transmit=false;
    Serial.println("receive mode");
  }

  if (transmit) {
    // transmit NRZI FSK packet
    transmitAIS(LAT,LON,MMSI);  //todo: this is where one would insert real gps information
    
    // update display
    strcpy(buf1,"fsk.ino\nxmit ");
    itoa(++count,buf2,10);
    strcat(buf1,buf2);
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, buf1);
    Heltec.display -> display();
    Serial.println(count);

    delay(10000);
  }
  else {
    int numbytes=receiveAIS();
    if (numbytes==25) { //position reports are 21 bytes +2 crc +2 flags
      count+=1;
      Serial.printf("%s\n",hexbuf2str(buf,hdlc,numbytes));
    }

    // update display
    strcpy(buf1,"fsk.ino\nReceiving AIS\nCount: ");
    itoa(count,buf2,10);
    strcat(buf1,buf2);
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, buf1);
    Heltec.display -> display();

  }
}

