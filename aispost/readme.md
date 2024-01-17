# AISPOST
The boat executes ais2.sh at boot. It runs aispostsimple.py in the background and rtl_ais. 
## AISPOSTSIMPLE
aispostsimple.py looks at data either from USB serial (connected to the AIS transponder) or local UDP (from rtl_ais). When the boat is idle, I move the antenna from the transponder to the dongle on the boat Pi (disconnecting the power to the transponder) and configure aispostsimple to look for UDP data.
## RTL_AIS
A very nice dual channel ais receiver using a dongle SDR. https://github.com/dgiardini/rtl-ais It exits sometimes so I put it in a loop. It has been running for months now.

