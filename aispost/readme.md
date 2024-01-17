# AISPOST
The boat Pi executes ais2.sh at boot. It runs aispostsimple.py in the background and rtl_ais. The intent is to send AIS information over the internet to MarineTraffic and AISHUB and broadcast traffic locally.
## AISPOSTSIMPLE
aispostsimple.py looks at data either from USB serial (connected to the AIS transponder) or local UDP (from rtl_ais). When aispostsimple sees a packet, it transmits it to MarineTraffic and 
AISHUB along with a local broadcast for OpenCPN and other apps that can display AIS traffic. When the boat is idle, I move the antenna from the transponder to the dongle on the boat Pi (disconnecting the power to the transponder) and configure aispostsimple to look for UDP data. 
## OWNSHIP
aispostsimple.py generates ownship packets if it does not see any coming from the transponder (ie when listening to rtl_ais). You manually set the lat/lon to where the boat is located. If it does see an ownship location packet, then it will use it and stop faking. Long term, the real solution is to add a GPS to the Pi. 
## RTL_AIS
A very nice dual channel ais receiver using a dongle SDR. https://github.com/dgiardini/rtl-ais It exits sometimes so I put it in a loop. It has been running for months now. 

