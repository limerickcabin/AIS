/*
 * Sends AIS messages via Heltec WiFi LoRa module
*/
#include "Arduino.h"
#include "heltec.h"

float    LAT=   20.7500;  //Punta Mita anchorage
float    LON= -105.5000;   
uint32_t MMSI= 367499000;

void setup()
{
 	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  radioSetup();
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

  // transmit NRZI FSK packet
  transmitAIS(LAT,LON,MMSI);  //todo: this is where one would insert real gps information
  
  // update display
  strcpy(buf1,"fsk.ino\nSending AIS\nCount: ");
  itoa(++count,buf2,10);
  strcat(buf1,buf2);
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 10, buf1);
  Heltec.display -> display();

  delay(10000);
 }

