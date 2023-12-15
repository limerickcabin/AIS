/*
 * Sends AIS messages via Heltec WiFi LoRa module
*/

#include "Arduino.h"
#include "heltec.h"
#include "images.h"

void setup()
{
	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);

	logo();
	delay(3000);

  radioSetup();
}

void loop()
{
  static uint32_t count;
  static char buf1[80],buf2[80];

  // transmit NRZI FSK packet
  transmitAIS();
  
  // update display
  strcpy(buf1,"Sending AIS\nCount: ");
  itoa(++count,buf2,10);
  strcat(buf1,buf2);
  Heltec.display -> clear();
  Heltec.display -> drawString(0, 20, buf1);
  Heltec.display -> display();

  delay(10000);
 }

void logo(){
	Heltec.display -> clear();
	Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
	Heltec.display -> display();
}

