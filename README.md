# AIS
Various AIS receive and transmit projects. 
- Generates AIS RF packets in Python for Windows, Raspberry Pi and LoRa Arudino module.
- Transmits over the air with HackRF, directly with Raspberry Pi and LoRa Arduino modules!
## On Windows
- Edit packet.py with your location and mmsi, run it and you will get two files suitable for transmission with your HackRF
- Send them over the air with sendiq.bat
## On Raspberry Pi
- install rpitx https://github.com/F5OEO/rpitx
- put a little wire on GPIO4 (note, this may be illegal as the pin generates a lot of spurious signals - filter it to be sure)
- git clone https://github.com/limerickcabin/AIS somewhere on your RPi
- edit and run rpiais.py in Thonny - it will build the packets and spawn rpitx to transmit them
## On Arduino LoRa Module
- Arduino sketch is in the LoRa folder
- get yourself a LoRa module https://www.amazon.com/gp/product/B07FYWFH4C
- install RadioLib in your Arduino library folder https://github.com/jgromes/RadioLib
- install the Heltec libraries in your Arduino library folder
- be sure you are set up to compile ESP32
- from the serial console, press t to begin transmitting
## Google doc with more information
- https://docs.google.com/document/d/1x-EnqImrwt8cS4G0BIZkCU3ouJ_4ixGBlbQNpQnAJSA/edit?usp=sharing
