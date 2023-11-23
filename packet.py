#builds an ownship NMEA AIS packet
#send the NMEA to marinetraffic and it will think you are anywhere you want
#also generates a HackRF file for RF transmission

from fastcrc import crc16
import aisiq

'''
lat=   48.49963 #Anacortes, on the hard (degrees)
lon= -122.60392
mmsi= 367499470
'''
lat=   20.7500  #Punta Mita anchorage
lon= -105.5000   
mmsi= 367490000 #fake

def crc(b,numbits):
    #parse numbits-bit integer block b, return byte array with crc appended
    
    #build a byte array ba 8 bits at a time
    block=b
    ba=b''
    while numbits:
        numbits-=8
        v= (block>>numbits) & 0xff
        ba+=bytes([v])

    #glue the crc onto the end
    crc=crc16.ibm_sdlc(ba)
    #print(hex(crc))
    ba+=bytes([crc & 0xff])       #lsb first
    ba+=bytes([(crc>>8) & 0xff])  #then msb

    #test the completed packet
    if (crc16.ibm_sdlc(ba)!=0x0f47):
        print("something is broken in crc()")
        return(b'')
    else:
        return(ba)

def buildHDLC(b,numbits):
    #todo: think about moving bit stuffing to the transmitter
    ba=crc(b,numbits)

    #start the packet
    hdlc=0x7eaaaaaa     #preamble and flag
    bitCount=32
    
    #first byte in byte array is the least significant
    oneCount=0
    for b in ba:
        for count in range(8):
            if b & (1<<count):
                #shift bits into hdlc and perform bit stuffing
                oneCount+=1
                hdlc |= 1<<bitCount
            else:
                oneCount=0
            bitCount+=1
            if oneCount==5:     #need to stuff a 0?
                oneCount=0
                bitCount+=1     #advancing the bitCount shifts in a 0
    hdlc |= 0x7e<<bitCount
    bitCount+=8
    #print(hex(hdlc))
    return(hdlc,bitCount)

def buildNMEA(b):    
    #build NMEA sentence
    #armor the integer block into a string, 6 bits at a time
    block=b
    s=""
    numbits=168
    while numbits:
        numbits-=6
        v= (block>>numbits) & 0b111111
        #grab six bits starting on the left (msb)
        v+=48
        if v>87: #88 through 95 are not used
            v+=8
        s=s+chr(v)

    #glue the amored string into empty VDO packet
    p1="!AIVDO,1,1,,," + s + ",0*"

    #checksum - xor characters between ! and *
    cs=0
    for c in p1[1:-1]:
        cs^=ord(c)
    hexcs="{:02X}".format(cs)

    #finish packet
    p2=p1+hexcs

    #test results of packet and crc
    if (len(p2)!=46):
        print("nmea packet is the wrong length")
        return("")
    else:
        return(p2)

def str2sixbit(str):
    #convert string to six bit ascii
    sixbit=0
    for c in str:
        sixbit=sixbit<<6
        v=ord(c)
        if v<64:
            sixbit+=ord(c)
        else:
            sixbit+=ord(c)-64
    return(sixbit)
        
def buildBlock(lat,lon,mmsi):
    #builds the 168 bit AIS location block with user data
    
    #convert to deci-milli-minutes (1/10000)
    latdmm=int(lat*600000) & (2**27 - 1)
    londmm=int(lon*600000) & (2**28 - 1)

    #build block (amazing one can store 168 bits in an integer)
    #block is a left to right representation of the bit fields
    #for example, the first six bits on the left is the message type
    #while the bytes are left to right, the bits are not
    #it is expected that one shifts the bits out of the bytes lsb first
    b=1             #message type
    b=(b<<2)+0      #repeat
    b=(b<<30)+mmsi  #MMSI
    b=(b<<4)+5      #status - moored
    b=(b<<8)+128    #ROT
    b=(b<<10)+0     #SOG
    b=(b<<1)+0      #integrity bit
    b=(b<<28)+londmm#28 bits of longitude
    b=(b<<27)+latdmm#27 bits of latitude
    b=(b<<12)+3600  #COG
    b=(b<<9)+0      #HDG
    b=(b<<6)+60     #Time
    b=(b<<2)+0      #Maneuver
    b=(b<<3)+0      #Spare
    b=(b<<1)+0      #RAIM
    b=(b<<19)+49205 #Radio Status

    return(b,168)

def buildSVblock(mmsi):
    #builds static and voyage related block
    b=5             #message type
    b=(b<<2)+0      #repeat
    b=(b<<30)+mmsi  #mmsi
    b=(b<<2)+0      #version
    b=(b<<30)+0     #imo
    b=(b<<42) +str2sixbit("WDF1234") #call sign
    b=(b<<120)+str2sixbit("SEA STAR 7          ")
    b=(b<<8)+37     #ship type: pleasure
    b=(b<<9)+10     #to bow
    b=(b<<9)+10     #to stern
    b=(b<<6)+3      #to port
    b=(b<<6)+3      #to starboard
    b=(b<<4)+1      #epfd
    b=(b<<4)+11     #month
    b=(b<<5)+22     #day
    b=(b<<5)+23     #hour
    b=(b<<6)+3      #minute
    b=(b<<8)+17     #draft
    b=(b<<120)+str2sixbit("PUNTA MITA NAYARIT  ")
    b=(b<<1)+1      #DTE
    b=(b<<1)        #spare
    
    return(b,424)

def test():
    #generate good/bad nmea and hdlc
    b,n=buildBlock(47,-122,367499470)
    n1=buildNMEA(b)
    n2=buildNMEA(b-1)
    h1,nbits=buildHDLC(b,  n)
    h2,nbits=buildHDLC(b-1,n)
    #known good outputs:
    nmea='!AIVDO,1,1,,,15NNHkUP00GAQl0Jq<@>401p0<0m,0*21'
    hdlc=26645051762786174409183938347213669988067871452032379540152515209898
    #do tests
    testOK1=(n1==nmea) and (n2!=nmea)   #test nmea
    testOK2=(h1==hdlc) and (h2!=hdlc)   #test hdlc
    testOK=testOK1 and testOK2

    if testOK==False:
        print("calculated nmea's:",n1,n2)
        print("calculated hdlc's:",h1,h2)

    return(testOK)

def main(lat,lon,mmsi):
    if test():
        #position
        b,n=buildBlock(lat,lon,mmsi)    #build a block
        print(buildNMEA(b))             #build NMEA packet
        h,n=buildHDLC(b,n)              #build HDLC
        aisiq.main(h,n,"pos.s8")        #build iq file for hackrf
        #static voyage:
        b,n=buildSVblock(mmsi)
        h,n=buildHDLC(b,n)
        aisiq.main(h,n,"sv.s8")
    else:   
        print("test failed - something is broken")

main(lat,lon,mmsi)


