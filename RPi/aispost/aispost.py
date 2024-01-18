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
#inputMode="!UDP"
inputPort=10110

#UDP broadcast port
outputPort=10111

#position data from the same MMSI is filtered for a while
#identical other packets are filtered for a while
maxTrafficAge=0
maxOtherAge=0

if inputMode=="UDP":
    sock2= socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock2.bind(('', inputPort))
else:
    ser=serial.Serial(serialP,serialS,timeout=10)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)



def convert(char):
    #convert the character into its 6-bit value
    char-=48
    if char>40:
        char-=8
    return(char)

def getInt(data, bitStart, width, signed=1):
    #get an integer of size width (signed or unsigned) from bitStart bits into data
    offset=bitStart%6       #each character in data contains 6 bits of information
    mask=0b111111>>offset
    numBits=6-offset
    index=int(bitStart/6)
    accum=convert(data[index])&mask
    index+=1

    while (numBits<=width):
        accum=(accum<<6)+convert(data[index])
        index+=1
        numBits+=6

    accum=accum>>(numBits-width)

    #do the two-compliment on the integer if msb is 1 and requested
    if (accum&(1<<(width-1))) and signed:
        accum=accum-(1<<width)
    return(accum)

def inList(searchTerm,searchList,refresh=0):
    now=int(time.time())
    for l in searchList:
        if l[0]==searchTerm:
            #print("dupe traffic")
            if refresh==1:
                #refreshes the time so the list has the last time seen
                searchList.remove(l)
                searchList.append([searchTerm,now])
            return(False)
    #new traffic mmsi
    searchList.append([searchTerm,now])
    return(True)

def refreshList(searchTerm,searchList):
    inList(searchTerm,searchList,1)

then=then2=time.time()
mmsiList=[]
hourList=[]
otherList=[]
numSent=0
numNotSent=0

#pretend we are at the dock - actual ownship will overwrite
VDO=b"!AIVDO,1,1,,,15NNHkPP00o@4n`KA55>4?vf0<0m,0*29\r\n"

print("aispost.py starting up")

while True:
    if inputMode=="UDP":
        data=sock2.recv(200)
    else:
        data=ser.readline()
    #print(data)
    oneLine=data                    #binary array
    lineStr=data.decode("utf-8")    #convert array to string
    now=time.time()
    if (data[1:6]==b"AIVDM" or data[1:6]==b"AIVDO"):
        if data[1:6]==b"AIVDO":
            VDO=data
        comma=0
        index=0
        while (index<len(data)):
            index+=1
            if data[index-1]==ord(','):
                comma+=1
                if comma==5:
                    break;
        data=data[index:] #whatever is after the 5th comma

        if comma==5:
            if len(data)>7:
                msgType=convert(data[0])
                mmsi=getInt(data,8,30,0)
        
                traffic= msgType==1 or msgType==2 or msgType==3 or msgType==18
                if traffic:
                    unique=inList(mmsi,mmsiList)
                    refreshList(mmsi,hourList)
                else:
                    #look for unique other-than-traffic packets
                    unique=inList(lineStr,otherList)
                
                if unique==True:
                    #print(lineStr,end="")
                    numSent+=1
                    try:
                        sock.sendto(oneLine, (udpAddr, udpPort))
                        #sock.sendto(oneLine, ("127.0.0.1", 10111))
                        sock.sendto(oneLine,  ('<broadcast>', outputPort))

                    except:
                        print("socket error 1")
                else:
                    numNotSent+=1

    if (now-then)>1: #every so often
        try:
            #sock.sendto(VDO, (udpAddr, udpPort))
            sock.sendto(VDO,  ('<broadcast>', outputPort))
        except:
            print("socket error 2")
        #print(len(mmsiList))
        #print(numSent,numNotSent,round(numSent/(numSent+numNotSent),2))
        print(len(mmsiList),len(hourList),round(numSent/(numSent+numNotSent),2))
        then=now
        #numSent=numNotSent=0
        #go through the lists
        for l in mmsiList:
            if l[1]<(now-maxTrafficAge): #remove traffic mmsi if too old
                mmsiList.remove(l)
               # print("deleted traffic")
        for l in hourList:
            if l[1]<(now-3600): #remove traffic mmsi if too old
                hourList.remove(l)
               # print("deleted traffic")
        for l in otherList:
            if l[1]<(now-maxOtherAge): #remove other packets if too old
                otherList.remove(l)
                #print("deleted other")

    if (now-then2)>60: #every so often
        then2=now
        try:
            sock.sendto(VDO, (udpAddr, udpPort))
        except:
            print("socket error 3")

