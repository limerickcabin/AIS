import socket
import serial
import time

#MarineTraffic assigns the port number when you register your station
udpPort=12296
udpAddr="5.9.207.224"
mt=True

#AIShub
udpPort2=3823
udpAddr2="144.76.105.244"
ah=True

#Serial port connected to the AIS receiver
serialP='/dev/ttyUSB0'
serialS=38400

#Choose UDP or serial input (RTL dongle or AIS receiver)
inputMode="UDP"
inputPort=10110

#UDP broadcast port
outputPort=10111

if inputMode=="UDP":
    sock2= socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock2.bind(('', inputPort))
else:
    ser=serial.Serial(serialP,serialS,timeout=10)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

then=then2=time.time()

#pretend we are at the dock - actual ownship will overwrite
#fakeVDO=b"!AIVDO,1,1,,,15NNHkPP00o@4n`KA55>4?vf0<0m,0*29\r\n"
#fakeVDO=b"!AIVDO,1,1,,,15NNHkPP00o>iG`Kh79f4?vf0<0m,0*8a\r\n"
#fakeVDO=b"!AIVDO,1,1,,,15NNHkUP00G>iEhKh6e>401p0<0m,0*20\r\n"
fakeVDO=b"!AIVDO,1,1,,,15NNFv5P00HLwfH;pdn>401p0<0m,0*02\r\n"
sawRealVDO=False
sendFake=False


print("aispostsimple.py starting up")

while True:
    if inputMode=="UDP":
        data=sock2.recv(200)
    else:
        try:
            data=ser.readline()
        except:
            print("serial error")

    oneLine=data                    #binary array
    lineStr=data.decode("utf-8")    #convert array to string
    now=time.time()
    
    if data[1:14]==b"AIVDO,1,1,,,1":#ownship position report
        sawRealVDO=True
        realVDO=data

    if data[1:5]==b"AIVD":
        try:
            if mt:
                sock.sendto(oneLine, (udpAddr,  udpPort))   #MarineTraffic
            if ah:
                sock.sendto(oneLine, (udpAddr2, udpPort2))  #AIShub
            sock.sendto(oneLine,  ('<broadcast>', outputPort)) #local
            print(lineStr,end="")
        except:
            print("socket error 1")

    if (now-then)>1: #output ownship for OpenCPN
        try:
            if sawRealVDO:
                sock.sendto(realVDO,  ('<broadcast>', outputPort))
            else:
                if sendFake:
                    #print(fakeVDO)
                    sock.sendto(fakeVDO,  ('<broadcast>', outputPort))
        except:
            print("socket error 2")

    if (now-then2)>120: #every so often
        then2=now
        
        #log times that we are up
        fo=open("log.txt","a")
        fo.write(str(now)+"\n")
        fo.close()
        
        if sawRealVDO==False and sendFake==True:
            try:
                sock.sendto(fakeVDO, (udpAddr , udpPort))
                sock.sendto(fakeVDO, (udpAddr2, udpPort2))
            except:
                print("socket error 3")

