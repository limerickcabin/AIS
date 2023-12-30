/*
 * Sends AIS messages via Heltec WiFi LoRa module
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
  bool transmit=false;

  if (transmit) {
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
  else {
    if (Serial.read()>0) {
      transmitAIS(LAT,LON,MMSI);  
      Serial.println("packet transmitted");
    }

    int numbytes=receiveAIS();
    if (numbytes==25) {
      if (crc16_ibm_sdlc(&hdlc[1],23)==0x0f47) {
        count+=1;
        Serial.printf("%s\n",hexbuf2str(buf,hdlc,numbytes));
      }
    }

    // update display
    strcpy(buf1,"fsk.ino\nReceiving AIS\nCount: ");
    itoa(count,buf2,10);
    strcat(buf1,buf2);
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 10, buf1);
    Heltec.display -> display();

  }
}

