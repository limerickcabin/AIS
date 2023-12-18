/*
 * Sends AIS messages via Heltec WiFi LoRa module
*/

#include "Arduino.h"
#include "heltec.h"

void setup()
{
 	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
 radioSetup();
}

void loop()
{
  static uint32_t count;
  static char buf1[80],buf2[80];

  // transmit NRZI FSK packet
  transmitAIS();
  
  // update display
  strcpy(buf1,"fsk.ino\nSending AIS\nCount: ");
  itoa(++count,buf2,10);
  strcat(buf1,buf2);
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 10, buf1);
  Heltec.display -> display();

  delay(10000);
 }

