# AIS
AIS is used by boats to broadcast their location for situational awareness. Most boats over 20m are mandated to broadcast AIS but many, and increasingly so, smaller vessels broadcast voluntarily. Two marine VHF channels were set aside years ago to allow AIS to work worldwide. 
# Overview
There are many very good SDR dongle receive projects out there. This is different. 
This is a collection of various AIS transmit projects that I have created. 
- Generates AIS RF packets in Python for Windows and Raspberry Pi and Arduino for LoRa Arudino module.
- Transmits over the air with HackRF, directly with Raspberry Pi and LoRa Arduino modules!
## On Windows
- Edit packet.py with your location and mmsi, run it and you will get two files suitable for transmission with your HackRF
- Send them over the air with sendiq.bat
## On Raspberry Pi
- install rpitx https://github.com/F5OEO/rpitx (note: it works fine on my RPi4 but not on my RPi5)
- git clone https://github.com/limerickcabin/AIS somewhere on your RPi
- put a little wire on GPIO4 (note, this may be illegal as the pin generates a lot of spurious signals - filter it to be sure)
- edit and run rpiais.py in Thonny - it will build the packets and spawn rpitx to transmit them
## On Arduino LoRa Module
- Arduino sketch is in the LoRa folder
- get yourself a LoRa module. I used https://www.amazon.com/gp/product/B07FYWFH4C
- install RadioLib in your Arduino library folder https://github.com/jgromes/RadioLib
- install the Heltec libraries in your Arduino library folder https://github.com/HelTecAutomation/Heltec_ESP32.git
- be sure you are set up to compile ESP32
- edit the AISinfo structure with your specifics
- compile and upload the sketch
- from the serial console, press t to begin transmitting
## Google doc with more information
- https://docs.google.com/document/d/1x-EnqImrwt8cS4G0BIZkCU3ouJ_4ixGBlbQNpQnAJSA/edit?usp=sharing
