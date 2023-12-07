'''
builds an ownship NMEA AIS packet
send the NMEA to marinetraffic and it will think you are anywhere you want
also generates ft files for rpitx and sends them (position and static/voyage)

some lessons learned
fastcrc did not import so I wrote crc16
has to run in Thonny or Python3. Python (I assume v2) did not work
be sure to turn off Run->Pygame Zero mode if you use Thonny IDE
'''
import crc16,struct,os,time

'''
LAT=   48.49963 #Anacortes, on the hard (degrees)
LON= -122.60392
MMSI= 367499470
'''
LAT=   20.7500  #Punta Mita anchorage
LON= -105.5000   
MMSI= 367499000

def buildFT(hdlc,numbits,fn):
    #builds a frequency/time file for rpitx running in RF mode
    w=open(fn,"wb")
    ta=[]                           #test array
    r=0
    sample=6000                     #deviation Hz
    k=0.8                           #filter 
    f1=f2=f3=f4=0
    h=hdlc                          #make copies
    n=numbits
    oversample=0
    fs=48000                        #didn't work right at 9600 so oversample
    fb=9600
    sp=int(round(1e9/fs,0))         #sample period in nanoseconds

    i=10000
    while i:                        #porch
        #each sample in .ft file is 16 bytes
        #first 8 is a double float for frequency offset
        #next is an uint_32 with sample period in nanoseconds
        #next is 4-byte pad to round it out to 16-bytes
        i-=1
        s=struct.pack('<d',0)       #double frequency
        w.write(s)
        s=struct.pack('<q',sp)      #4 byte integer time and 4 byte pad
        w.write(s)
        r+=1
       
    while n:
        n-=1
        b=h & 1                     #lowest bit of first byte is first
        h=h>>1
        #todo: think about bit stuffing here
        if b==0:
            #nrzi: flip on 0 bits
            sample=-sample
        while oversample<fs:
            oversample+=fb
            #filter sample
            f1+=k*(sample-f1)
            f2+=k*(f1-f2)
            f3+=k*(f2-f3)
            f4+=k*(f3-f4)
            s=struct.pack('<d',f4)
            w.write(s)
            s=struct.pack('<q',sp)
            w.write(s)
            r+=1
            ta+=bytes([int(f4) & 0xff]) #test array
        oversample-=fs

    for i in range(10000):           #tail
        s=struct.pack('<d',0)
        w.write(s)
        s=struct.pack('<q',sp)
        w.write(s)
        r+=1
    w.close()
    print("wrote {} records".format(r))
    
    #return crc of generated frequencies
    cs=crc16.ibm_sdlc(ta)
    return(cs)

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
    c=crc16.ibm_sdlc(ba)
    #print(hex(crc))
    ba+=bytes([ c     & 0xff])  #lsb first
    ba+=bytes([(c>>8) & 0xff])  #then msb

    #test the completed packet
    if (crc16.ibm_sdlc(ba)!=0x0f47):
        print("something is broken in crc()")
        return(b'')
    else:
        return(ba)

def buildHDLC(block,numbits):
    #todo: think about moving bit stuffing to the transmitter
    
    #get byte array with crc glued to the end
    ba=crc(block,numbits)

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

def str2sixbit(str,len):
    #convert str to six bit ascii packed into an int
    #len is the max number of characters
    sixbit=0
    for c in str:
        sixbit=sixbit<<6
        v=ord(c)
        if v<64:
            sixbit+=ord(c)
        else:
            sixbit+=ord(c)-64
        len-=1
    #pad out with spaces
    i=len
    while i:
        i-=1
        sixbit=(sixbit<<6) + 32 
    return(sixbit)
        
def buildBlock(latitude,longitude,mmsi):
    #builds the 168 bit AIS location block with user data
    #all packed into an Int
    
    #convert to deci-milli-minutes (1/10000)
    latdmm=int(latitude *600000) & (2**27 - 1)
    londmm=int(longitude*600000) & (2**28 - 1)

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
    b=(b<<42) +str2sixbit("WDF1234",7) #call sign
    b=(b<<120)+str2sixbit("SEA STAR 7",20)
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
    b=(b<<120)+str2sixbit("PUNTA MITA NAYARIT",20)
    b=(b<<1)+1      #DTE
    b=(b<<1)        #spare
    
    return(b,424)

def test():
    #generate good/bad nmea, hdlc and FT
    b,n=buildBlock(47,-122,367499470)
    n1=buildNMEA(b)
    n2=buildNMEA(b-1)
    h2,nbits1=buildHDLC(b-1,n)
    h1,nbits1=buildHDLC(b,  n)
    cs=buildFT(h1,nbits1,"/tmp/test.ft")
    #known good outputs:
    nmea='!AIVDO,1,1,,,15NNHkUP00GAQl0Jq<@>401p0<0m,0*21'
    hdlc=26645051762786174409183938347213669988067871452032379540152515209898
    ftcs=14227
    #do tests
    testOK1=(n1==nmea) and (n2!=nmea)   #test nmea
    testOK2=(h1==hdlc) and (h2!=hdlc)   #test hdlc
    testOK3=cs==ftcs
    testOK=testOK1 and testOK2 and testOK3

    if testOK==False:
        print("calculated nmea's:",n1,n2,testOK1)
        print("calculated hdlc's:",h1,h2,testOK2)
        print("calculated ftcs:", cs,testOK3)

    return(testOK)

def main(lat,lon,mmsi):
    if test():
        #position
        b,n=buildBlock(lat,lon,mmsi)    #build a block
        nmea=buildNMEA(b)               #build NMEA packet
        h,n=buildHDLC(b,n)              #build HDLC
        buildFT(h,n,"/tmp/pos.ft")      #build ft file for rpitx
        os.system('sudo rpitx -m RF -i /tmp/pos.ft -f 162025')
        os.system('sudo rpitx -m RF -i /tmp/pos.ft -f 161925')
        #static voyage:
        b,n=buildSVblock(mmsi)
        h,n=buildHDLC(b,n)
        buildFT(h,n,"/tmp/sv.ft") 
        os.system('sudo rpitx -m RF -i  /tmp/sv.ft -f 162025')
        print("NMEA AIS string for",lat,lon,mmsi)
        print(nmea)
    else:   
        print("test failed - something is broken")

def repeat():
    while True:
        os.system('sudo rpitx -m RF -i /tmp/pos.ft -f 162025')
        os.system('sudo rpitx -m RF -i /tmp/pos.ft -f 161975')
        time.sleep(3*60)            

main(LAT,LON,MMSI)

