# AIS
Generate AIS RF packets in Python for Windows and Raspberry Pi
-
On Windows
- You need to get fastcrc (pip install fastcrc)
- Edit packet.py with your location and mmsi, run it and you will get two files suitable for transmission by hackrf_transfer (sendiq.bat)
- 
On Raspberry Pi
- install rpitx putting a little wire on GPIO4
- put rpiais and crc16 python files somewhere
- edit and run rpiais.py in Thonny

Google doc with more information
- https://docs.google.com/document/d/1x-EnqImrwt8cS4G0BIZkCU3ouJ_4ixGBlbQNpQnAJSA/edit?usp=sharing
