# AIS
Generate AIS RF packets in Python for Windows and Raspberry Pi
## On Windows
- Edit packet.py with your location and mmsi, run it and you will get two files suitable for transmission by hackrf_transfer (sendiq.bat)
## On Raspberry Pi
- install rpitx https://github.com/F5OEO/rpitx
- put a little wire on GPIO4 (note, the may be illegal as the pin generates a lot of spurious signals - filter it to be sure)
- git cloan https://github.com/limerickcabin/AIS somewhere on your RPi
- edit and run rpiais.py in Thonny - it will build the packets and spawn rpitx to transmit them
## Google doc with more information
- https://docs.google.com/document/d/1x-EnqImrwt8cS4G0BIZkCU3ouJ_4ixGBlbQNpQnAJSA/edit?usp=sharing
