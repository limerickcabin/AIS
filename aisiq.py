'''
builds an iq file for a given hdlc ais packet
nrzi gaussian-ish fsk

to send the file, type this at the windows command line:
hackrf_transfer -t ais2e6.s8 -s 2e6 -f 162e6 -x 47
transmit ais2e6.s8, 2Msps, AIS center freq, high power

'''
import math
import crc16

fs=2000000  #hackrf_transfer sample rate
fb=9600     #baudrate
dev=6000    #deviation
tailSeconds=1

def fsk(hdlc,numbits):
    iq=[]
    wt=0                            #omega-t
    winc=dev/fs*2*math.pi           #how much to change omega-t each sample
    oversample=0
    sample=1
    fdev1=fdev2=fdev3=fdev4=0
    k=0.1
    wincOffset=25000/fs*2*math.pi   #25kHz offset to avoid sideband issues
    porch=0
    hdlc=hdlc<<porch
    numbits+=porch
    while numbits:
        numbits-=1
        b=hdlc & 1                  #lowest bit of first byte is first
        hdlc=hdlc>>1
        #todo: think about bit stuffing here
        if b==0:
            #nrzi: flip on 0 bits
            sample=-sample
        while oversample<fs:    #go fill a baud amount of samples
            oversample+=fb      #on average will sample a bit every fs/fb
            #calculate i,q
            i=int(round(math.cos(wt)*127,0))
            q=int(round(math.sin(wt)*127,0))
            #filter sample - todo: do proper gaussian
            fdev1+=k*(sample-fdev1)
            fdev2+=k*(fdev1 -fdev2)
            fdev3+=k*(fdev2 -fdev3)
            fdev4+=k*(fdev3 -fdev4)
            #next time
            wt+=winc*fdev4          #fm modulate
            wt+=wincOffset          #offset to avoid sideband issues
            if wt>7:
                wt-=math.pi*2
            if wt<-7:
                wt+=math.pi*2
            #change i,q back to byte and store
            if i<0:
                i+=256
            if q<0:
                q+=256
            iq.append(i)  #i
            iq.append(q)  #q
        #end filling baud period of time
        oversample-=fs          #next bit
    #end filling bits
    #add a tail of no carrier
    for j in range(int(fs*tailSeconds)):
        iq.append(0)  #i
        iq.append(0)  #q
    return(iq)

def test():
    #test vector and result
    hdlc=2246661663122323673581324273823821738011688848303975804895708217726
    nbits=221
    c=20802
    #try it
    crc=crc16.ibm_sdlc(bytes(fsk(hdlc,nbits)))
    testOK = crc == c
    if testOK==False:
        print("iq crc does not match, computed crc:",crc)
    return(testOK)

def main(hdlc, numbits, filename):
    #hdlc is a big integer with the bit pattern to be sent
    if test():
        #generate i and q bytes
        iq=fsk(hdlc, numbits)
        #print(crc16.ibm_sdlc(bytes(iq)))
        #publisize
        w=open(filename,"wb")
        w.write(bytes(iq))
        w.close()
        print("wrote {} bytes".format(len(iq)))
    else:
        print("something is wrong with fsk() in aisiq.py")

