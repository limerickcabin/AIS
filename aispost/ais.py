import socket
import serial
import time

#MarineTraffic assigns the port number when you register your station
udpPort=12296
udpAddr="5.9.207.224"

#Serial port connected to the AIS receiver
serialP='/dev/ttyUSB0'
serialS=38400

#Choose UDP or serial input (RTL dongle or AIS receiver)
inputMode="UDP"
inputPort=10110

#UDP broadcast port
outputPort=10111

#setup ports
if inputMode=="UDP":
    sock2= socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock2.bind(('', inputPort))
else:
    ser=serial.Serial(serialP,serialS,timeout=10)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

then=time.time()

while True:
    if inputMode=="UDP":
        data=sock2.recv(200)
    else:
        data=ser.readline()
    #print(data)
    oneLine=data                    #binary array
    lineStr=data.decode("utf-8")    #convert array to string
    now=int(time.time())
    if (data[1:6]==b"AIVDM"):
        #print(lineStr,end="")
        try:
            sock.sendto(oneLine, (udpAddr, udpPort))
            sock.sendto(oneLine,  ('<broadcast>', outputPort))
	    if time.time()-then>10:
		sock.sendto("!AIVDO,1,1,,,15NNHkPP00o@4n`KA55>4?vf0<0m,0*29\r\n", (udpAddr,udpPort))
		then=time.time()
        except:
            print("UDP error")
