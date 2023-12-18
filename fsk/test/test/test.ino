/*
incrementally porting over the AIS packet algorithms developed in Python
 */
#include "Arduino.h"
#include "heltec.h"

void setup() {
  // put your setup code here, to run once:
	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);

  Heltec.display -> clear();
  Heltec.display -> drawString(0, 10, "test.ino\nstuffing bits into array");
  Heltec.display -> display();

  tests();
}

void loop() {
  // put your main code here, to run repeatedly:

}
