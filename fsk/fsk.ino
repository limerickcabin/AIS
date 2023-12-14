/*
 * HelTec Automation(TM) WIFI_LoRa_32 factory test code, witch includ
 * follow functions:
 * - Basic OLED function test;
 * - Basic serial port test(in baud rate 115200);
 * - LED blink test;
 * - WIFI connect and scan test;
 * - LoRa Ping-Pong test (DIO0 -- GPIO26 interrup check the new incoming messages);
 * - Timer test and some other Arduino basic functions.
 * by Aaron.Lee from HelTec AutoMation, ChengDu, China
 * 鎴愰兘鎯犲埄鐗硅嚜鍔ㄥ寲绉戞妧鏈夐檺鍏徃
 * https://heltec.org
 *
 * this project also releases on GitHub:
 * https://github.com/HelTecAutomation/Heltec_ESP32
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

unsigned int counter = 0;

// not the clearest example. First bit is the msB, lsb
//byte packet[]={0xfd,0xe1,0x1c,0x6b,0x80,0x00,0xf0,0x00,0x20,0x1d,0x1b,0x9f,0x0b,0x30,
//             0x0f,0x1d,0x06,0x00,0x60,0xe1,0x5b,0x9e,0x57,0x04,0x7e,0xaa,0xaa,0xaa};
//swapped around so that the msb of lsB is sent first (left to right)
byte packet[]={0xaa,0xaa,0xaa,0x7e,0x4,0x57,0x9e,0x5b,0xe1,0x60,0x00,0x06,0x1d,0x0f,
               0x30,0x0b,0x9f,0x1b,0x1d,0x20,0x00,0xf0,0x00,0x80,0x6b,0x1c,0xe1,0xfd,0x00};
byte nrziPacket[29];

void logo(){
	Heltec.display -> clear();
	Heltec.display -> drawXbm(0,5,logo_width,logo_height,(const unsigned char *)logo_bits);
	Heltec.display -> display();
}

void WIFISetUp(void)
{
	// Set WiFi to station mode and disconnect from an AP if it was previously connected
	WiFi.disconnect(true);
	delay(100);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoConnect(true);
	WiFi.begin("TP-Link_1340","76011153");
	delay(100);

	byte count = 0;
	while(WiFi.status() != WL_CONNECTED && count < 10)
	{
		count ++;
		delay(1000);
		Heltec.display -> drawString(0, 0, "Connecting...");
		Heltec.display -> display();
	}

	Heltec.display -> clear();
	if(WiFi.status() == WL_CONNECTED)
	{
		Heltec.display -> drawString(0, 0, "Connected.");
		Heltec.display -> display();
//		delay(500);
	}
	else
	{
		Heltec.display -> clear();
		Heltec.display -> drawString(0, 0, "Failed to connect.");
		Heltec.display -> display();
		//while(1);
	}
	Heltec.display -> drawString(0, 10, "WiFi setup done");
	Heltec.display -> display();
	delay(500);
}

void WIFIScan(unsigned int value)
{
	unsigned int i;
    WiFi.mode(WIFI_STA);

	for(i=0;i<value;i++)
	{
		Heltec.display -> drawString(0, 20, "Starting scan.");
		Heltec.display -> display();

		int n = WiFi.scanNetworks();
		Heltec.display -> drawString(0, 30, "Scan done");
		Heltec.display -> display();
		delay(500);
		Heltec.display -> clear();

		if (n == 0)
		{
			Heltec.display -> clear();
			Heltec.display -> drawString(0, 0, "No network found");
			Heltec.display -> display();
			//while(1);
		}
		else
		{
			Heltec.display -> drawString(0, 0, (String)n);
			Heltec.display -> drawString(14, 0, "Networks found:");
			Heltec.display -> display();
			delay(500);

			for (int i = 0; i < n; ++i) {
			// Print SSID and RSSI for each network found
				Heltec.display -> drawString(0, (i+1)*9,(String)(i + 1));
				Heltec.display -> drawString(6, (i+1)*9, ":");
				Heltec.display -> drawString(12,(i+1)*9, (String)(WiFi.SSID(i)));
				Heltec.display -> drawString(90,(i+1)*9, " (");
				Heltec.display -> drawString(98,(i+1)*9, (String)(WiFi.RSSI(i)));
				Heltec.display -> drawString(114,(i+1)*9, ")");
				//display.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
				delay(10);
			}
		}

		Heltec.display -> display();
		delay(800);
		Heltec.display -> clear();
	}
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
  state = radio.setRxBandwidth(250.0);
  state = radio.setOutputPower(20.0);
  state = radio.setCurrentLimit(100.0);
  state = radio.setDataShaping(RADIOLIB_SHAPING_1_0);
  state = radio.setCRC(0);
  state = radio.setPreambleLength(1);
  state = radio.setSyncWord((uint8_t)0xcc, 8);
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
    for (int j=0;j<8;j++){ //first bit is least significant
      bit=(hdlc[i]>>j)&1;
      sample=bit?sample:sample^0x01;
      byte=byte<<1;         //shift the sample in, first bit ending up on the left (left to right)
      byte=byte|sample;
    }
    nrziPacket[i]=byte&0xff;
  }
}

#define BAND 433e6
void setup()
{
	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/, true /*LoRa use PABOOST*/, BAND /*LoRa RF working band*/);

	logo();
	delay(300);
	Heltec.display -> clear();

	WIFISetUp();
	WiFi.disconnect(); //Reinitialize WiFi
	WiFi.mode(WIFI_STA);
	delay(100);

	WIFIScan(1);

  radioLibSetup();
  nrzi(packet,sizeof(packet),nrziPacket);
}

void loop()
{
  // transmit FSK packet
  //debug
  //byte testPacket[]={0x01,0xff,0x00,0x80};
  //int state = radio.transmit(testPacket,4);
  int len=sizeof(nrziPacket);
  int state = radio.transmit(nrziPacket,len);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Packet transmitted successfully!"));
  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    Serial.println(F("[SX1262] Packet too long!"));
  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println(F("[SX1262] Timed out while transmitting!"));
  } else {
    Serial.println(F("[SX1262] Failed to transmit packet, code "));
    Serial.println(state);
  }
  delay(10000);
 }

