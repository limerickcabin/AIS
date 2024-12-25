/*
 * Sends and receives AIS messages via Heltec WiFi LoRa module
 * Transmit appears to work very well
 * Todo:
 * Receive is not working very well. It seems the sensitivity is poor. Even
 * when the transmitter is very close, it misses a lot of packets. 
 * Might try continuously reading the module's buffer directly
 *
 * Libraries from Library Manager
 * Heltec ESP32 Dev-Boards v1.1.5 - later versions crash on Heltec.begin()
 * RadioLib v6.5.0 and 7.1.1 work
 * 
*/

//Not sure why I need to define this - shouldn't the selection of the board do it?
#define WIFI_LORA_32_V3

#include "Arduino.h"
#include "heltec.h"
#include "radio.h"

//EDIT this with your actual position and static information
AISinfo ais = {  20.7660,    //todo: this is where one would insert real gps information
                -105.5131,   
               367499001,    //MMSI
               1,            //0 underway, 1 anchored, 5 moored
               "WDF1111",    //call sign 7 char max
               "NO NAME",    //boat name 20 char max
               "PUNTA MITA", //destination 20 char max
               3600,         //cog unknown
               0,            //sog
               0             //hdg
};

void setup()
{
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);
  radioSetup();
  Serial.begin(115200);
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
  static int loopCount=0;

  char c = Serial.read();
  if (c=='t') {
    transmit=true;
    count=0;
    loopCount=0;
    Serial.println("transmit mode");
  }
  if (c=='r') {
    transmit=false;
    count=0;
    Serial.println("receive mode");
  }

  if (transmit) {
    if (--loopCount<0) {
      loopCount=120;
      // transmit NRZI FSK packet
      transmitAIS(ais);  
      
      // update display
      sprintf(buf1,"LoRa.ino\nTransmitting AIS\nCount: %d",++count);
      Heltec.display -> clear();
      Heltec.display -> drawString(0, 0, buf1);
      Heltec.display -> display();
      Serial.println(count);
    }
    delay(1000);
  }
  else {
    if (receiveAIS()==25) count+=1;

    // update display
    sprintf(buf1,"LoRa.ino\nReceiving AIS\nCount: %d",count);
    Heltec.display -> clear();
    Heltec.display -> drawString(0, 0, buf1);
    Heltec.display -> display();
  }
}

