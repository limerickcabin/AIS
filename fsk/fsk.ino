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

float    LAT=   20.7660;  //Punta Mita anchorage
float    LON= -105.5131;   
uint32_t MMSI= 367499000;
uint8_t  NAV=  1;
char*    CALL= "WDF1111";
char*    NAME= "NO NAME";
char*    DEST= "PUNTA MITA";

void setup()
{
 	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  radioSetup();
  initCRC();

  Serial.printf("%s %s %s\n",__FILE__,__DATE__,__TIME__);
  if (tests()) Serial.println("all ais tests ran successfully");
  else {
    Serial.println("something is broken - stopped");
    while (true) delay(1000);
  }
  Serial.println("enter t to transmit");
}

void loop()
{
  static uint32_t count;
  static char buf1[80],buf2[80];
  static bool transmit=false;

  char c = Serial.read();
  if (c=='t') {
    transmit=true;
    count=0;
    Serial.println("transmit mode");
  }
  if (c=='r') {
    transmit=false;
    count=0;
    Serial.println("receive mode");
  }

  if (transmit) {
    // transmit NRZI FSK packet
    transmitAIS(LAT,LON,MMSI,NAV,CALL,NAME,DEST);  //todo: this is where one would insert real gps information
    
    // update display
    sprintf(buf1,"fsk.ino\nTransmitting AIS\nCount: %d",++count);
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, buf1);
    Heltec.display -> display();
    Serial.println(count);

    delay(10000);
  }
  else {
    if (receiveAIS()==25) count+=1;

    // update display
    sprintf(buf1,"fsk.ino\nReceiving AIS\nCount: %d",count);
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, buf1);
    Heltec.display -> display();
  }
}

