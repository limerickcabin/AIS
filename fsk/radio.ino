#include <RadioLib.h>
//#include "crc16.h"

// SX1262 has the following connections on Heltec V3:
// NSS pin:   8
// DIO1 pin:  14
// NRST pin:  12
// BUSY pin:  13
SX1262 radio = new Module(8,14,12,13);

//initialize SX1262 for AIS operation
void radioSetup() {
  // initialize SX1262 FSK modem with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  int state = radio.beginFSK();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // the following settings can also
  // be modified at run-time
  state = radio.setFrequency(162.025);
  state = radio.setBitRate(9.6);
  state = radio.setFrequencyDeviation(2.4);
  state = radio.setRxBandwidth(58.6);
  state = radio.setRxBoostedGainMode(true,false);
  state = radio.setOutputPower(10.0);       //-9 to 22 dBm
  state = radio.setCurrentLimit(100.0);     //about 10 dBm - I think it needs to be 150 for 22 dBm
  state = radio.setDataShaping(RADIOLIB_SHAPING_1_0); //I think GMSK is supposed to be 0_3 but this seems to work
  state = radio.setCRC(0);
  state = radio.setPreambleLength(0);
  state = radio.fixedPacketLengthMode();
  uint8_t sync[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  state = radio.setSyncWord(sync, 0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Unable to set configuration, code: %d\n",state);
  }
  
}

/*************************** AIS STUFF *************************/
//todo: add packet 18 support, more dynamics in position report (make a struct)

/*
decodes nrzi[] of length len into raw[]
*/
void unNRZI(byte* raw, byte* nrzi, uint16_t len){
  bool bit,lastbit;

  for (int i=0;i<ARRAYLEN;raw[i++]=0);

  for (int i=0;i<len;i++){
    for (int j=7;j>=0;j--){
      bit=(nrzi[i]>>j) & 1;
      raw[i]<<=1;
      if (bit==lastbit) raw[i]+=1;
      lastbit=bit;
    }
  }
}

/* find and store the first flag in raw of length len, destuff and reverse rest of bits 
   storing in hdlc until next flag. Returns number of bytes of packet including flags*/
int16_t frame(byte* hdlc, byte* raw, uint16_t len){
  uint16_t b=0,ones=0,bit,state=0,index=0;
  uint32_t bits=0;
  
  for (int i=0;i<len;hdlc[i++]=0);
  for (int i=0;i<len;i++){    //byte by byte
    for (int j=7;j>=0;j--){   //bit by bit, msb first
      bit=(raw[i]>>j)&1;
      b>>=1;
      if (bit) b|=0x80;       //shift in bit towards the right making a reversal
      bits+=1;

      //state 1: destuffing and storing bits
      if (state==1) {
        if (bit==1) {
          ones+=1;
        }
        else {
          //debit stuff if necessary
          if (ones==5) {
            //Serial.println("destuff");
            b=b<<1;   //shift out the stuffed zero
            bits-=1;
          }
          ones=0;
        }
        //if b is full, place it
        if (bits==8){
          hdlc[index++]=b;
          bits=0;
          if (b==0x7e) {
            if (crc16(&hdlc[1],index-2)==0x0f47) { //crc stuff between the flags but not the flags
              //todo: this returns only one hdlc packet per raw packet - there might be more
              return(index); //todo: assumes flag is on byte boundary
            }
            else {
              //keep looking if the packet did not crc
              index=0;          //start over
              hdlc[index++]=b;  //store flag (even though there is one already there)
            }
          }
        }
      }
      
      //state 0: looking for flag
      if (state==0) if (b==0x7e) {
        //Serial.println("flag");
        hdlc[index++]=b;
        state=1;
        b=0; 
        bits=0;
      }        
    }//byte is done
  }//array is done
  //if we got here, never saw the flag
  return(0);
}

/*
Builds a transmittable packet output[] from lat, lon and mmsi
Returns size of output[] numBytes
Note: the packet is in left to right sequence as expected by RadioLib with the lsb on the far left
*/
uint16_t buildPacket(byte *output, float lat, float lon, uint32_t mmsi){
  uint16_t numBytes;
  numBytes=buildBlock(block,lat,lon,mmsi);
  numBytes=buildHDLC(block,numBytes);  
  buildNRZI(output,hdlc,numBytes);
  return(numBytes);
}

//reverses bits in bytes of hdlc[] of length len bytes, peforms NRZI storing in nrzi[]
//nrzi flips the output data on 0 and leaves it the same on 1
void buildNRZI(byte * nrzi, byte * hdlc, int len) {
  uint8_t bit,byte,sample=0;

  for (int i=0;i<len;i++){      //least significant byte is first 
    byte=0;
    for (int j=0;j<8;j++){      //first bit is least significant in data - needs to end up on the left
      bit=(hdlc[i]>>j)&1;
      if (bit==0)sample^=0x01;  //flip the bit on 0's
      byte<<=1;                 //shift the sample in, first bit ending up on the left (left to right)
      byte|=sample;
    }
    nrzi[i]=byte;
  }
}

/*
hdlc packet with preamble, flags and crc from block[] of length in bytes
note that the bits in the bytes are reversed from convention - one needs to transmit the lsb first
however, most implementations and RadioLib expects the first bit sent to be on the far left (msb)
so a reversal will be needed somewhere downstream
*/
uint16_t buildHDLC(uint8_t *block, uint8_t length){
  uint32_t bitCount,oneCount,index,bitsinbits,bits;

  //start building hdlc
  for (int i=0;i<ARRAYLEN;i++) hdlc[i]=0;
  storeBits(hdlc,0xaaaaaaaaaaaa,0,24); //preample
  storeBits(hdlc,0x7e,24,8);     //flag
  bitCount=32;
  index=bitCount/8;

  bits=oneCount=bitsinbits=0;
  for (int i=0;i<length;i++){
    uint8_t b=block[i];         //take a bite
    for (int j=0;j<8;j++) {     //spit it out bit by bit, lsb first (the direction of transmit)
      if (b&(1<<j)) {
        oneCount+=1;
        bits|=(1<<bitsinbits);
        bitsinbits+=1;
      }
      else {
        oneCount=0;
        bitsinbits+=1;
      }

      if (oneCount==5) {        //stuff a bit if necessary
        //Serial.printf("stuffed a bit at %d\n",bitCount+bitsinbits);
        oneCount=0;
        bitsinbits+=1;
      }
    } //end of incoming byte
    //store the stuffed byte
    hdlc[index++]=bits;
    bitCount+=8;
    bits>>=8;
    bitsinbits-=8;
    
    //if there are enough bits left over to fill a byte, store the byte
    if (bitsinbits>=8) {
      hdlc[index++]=bits;
      bitCount+=8;
      bits>>=8;
      bitsinbits-=8;
    }
  } //end of incoming array
  //add crc
  int crc=crc16(block,length);
  bits|=crc<<bitsinbits;
  hdlc[index++]=bits;
  bits>>=8;
  hdlc[index++]=bits;
  bits>>=8;
  bitCount+=16;
  
  //add flag
  bits|=0x7e<<bitsinbits;
  hdlc[index++]=bits;
  bits>>=8;
  bitCount+=8;

  //there could be left over bits due to stuffing
  if (bitsinbits>0) {
    hdlc[index++]=bits;
    bitCount+=8;
  }
  
  return(bitCount/8);
}

void buildNMEA(uint8_t *block, uint16_t numBytes) {  
//build NMEA sentence
//armor the block into a string, 6 bits at a time
  char s[88];
  char c;
  uint16_t charPos=0;
  uint32_t threeByte;

  for (int i=0;i<numBytes;i+=3) { //do 3 bytes (24 bits) at a time creating 4 6-bit characters
    threeByte =block[i  ]<<16;//first is msByte
    threeByte+=block[i+1]<<8;
    threeByte+=block[i+2];
    //Serial.printf("%02x",threeByte);
    for (int j=0;j<4;j++) {
      c=(threeByte>>18) & 0b111111;        //top 6 bits of the 24 bits in threeByte
      c+=48;
      if (c>87) c+=8;
      s[charPos++]=c;
      threeByte<<=6;
    }
  }
  s[charPos]=0;

  //glue it into NMEA wrapper
  strcpy(nmea,"!AIVDO,1,1,,,");
  strcat(nmea,s);
  strcat(nmea,",0*");

  //checksum everything between ! and *
  charPos=1;
  uint16_t cs=0;
  while (true) {
    c=nmea[charPos++];
    if (c=='*') break;
    cs^=c;
  }
  sprintf(s,"%02X",cs);
  strcat(nmea,s);
}

/*
place bits amount of source into dest[] offset bits into dest[]
*/
void storeBits(uint8_t *dest, uint32_t source, uint16_t offset, uint8_t bits) {
  uint8_t  bitOffset=offset%8;
  uint8_t  byteOffset=offset/8;
  uint8_t  shift=64-bits-bitOffset;
  uint64_t mask=(1<<bits)-1;
  uint64_t shiftedSource=((uint64_t)(source) & mask)<<shift; //msbyte of source is now aligned in msbyte of shiftedSource
  //Serial.printf("shifted source %llx\n",shiftedSource);
  for (int i=0;i<8;i++){
    dest[i+byteOffset]|=shiftedSource>>(64-8);
    shiftedSource<<=8;
    if (shiftedSource==0) break;
  }
}

uint16_t buildBlock(uint8_t *block, float lat, float lon, uint32_t mmsi){
  uint32_t latdmm=int(lat*600000);
  uint32_t londmm=int(lon*600000);
  
  for (int i=0;i<ARRAYLEN;i++) block[i]=0;

  storeBits(block,1,      0,   6); //todo - make a version that just needs len
  storeBits(block,0,      6,   2);
  storeBits(block,mmsi,   8,  30);
  storeBits(block,5,      38,  4);
  storeBits(block,128,    42,  8);
  storeBits(block,0,      50, 10);
  storeBits(block,0,      60,  1);
  storeBits(block,londmm, 61, 28);
  storeBits(block,latdmm, 89, 27);
  storeBits(block,3600,   116,12);
  storeBits(block,0,      128, 9);
  storeBits(block,60,     137, 6);
  storeBits(block,0,      143, 2);
  storeBits(block,0,      145, 3);
  storeBits(block,0,      148, 1);
  storeBits(block,49205,  149,19);
  return(21);
}

/*
builds a hexstring str from buf of size len
*/
char* hexbuf2str(char *str, byte *buf, uint16_t len){
  for (int i=0;i<(len);i++){
    sprintf(&str[i*2],"%02x",buf[i]);
    str[i*2+2]=0;
  }
  return(str);
}

bool tests(void) {
  //test vector
  float lat=47;
  float lon=-122;
  uint32_t mmsi=367499470;
  //known good results
  char goodNMEA[]="!AIVDO,1,1,,,15NNHkUP00GAQl0Jq<@>401p0<0m,0*21";
  char goodNRZI[]="ccccccfe95e6fbd1bd515595a7ea549d484b8552aaa0aaabceb4d580aa";

  bool ok=true;
  uint16_t numBytes;

  //test NMEA
  numBytes=buildBlock(block,lat,lon,mmsi);
  buildNMEA(block,numBytes);
  if (strcmp(nmea,goodNMEA)){
	  Serial.println("bad NMEA");
	  ok=false;
  }
  //Serial.println(nmea);

  //test transmit chain
  numBytes=buildPacket(nrzi,lat,lon,mmsi);
  hexbuf2str(buf,nrzi,numBytes);
  if (strcmp(buf,goodNRZI)) {
	  Serial.println("bad NRZI");
	  ok=false;
  }
  //Serial.println(buf);
  
  //test decode chain
  unNRZI(buffer,nrzi,numBytes);                     //decode previously built NRZI
  numBytes=frame(hdlc,buffer,numBytes);             //find, de-bitstuff hdlc packet and reverse bits in bytes
  if ((numBytes!=25) |                              //position reports are 21 bytes info, 2 flags, 2 bytes crc
      (crc16(&hdlc[1],23)!=0x0f47)) {      //crc of a good packet including crc at end is always 0x0f47
      hexbuf2str(buf, hdlc, numBytes);              //printable hdlc
      Serial.printf("bad unNRZI or frame - destuffed, reversed hdlc:\n%s\n",buf);
      ok=false;
  }

  return(ok);
}

int transmitAIS(float lat, float lon, uint32_t mmsi){
  //build packet to send
  uint16_t numBytes=buildPacket(nrzi,lat,lon,mmsi);

  //transmit packet
  int state = radio.transmit(nrzi,numBytes);
  
  return(state);
}

//receive a packet, nrzi it, and try to frame an hdlc frame
//returns number of bytes including the flags or -1 if incomplete frame
int receiveAIS(void) {
  for (int i=0;i<ARRAYLEN;nrzi[i++]=0);
  
  if (radio.receive(nrzi,255)==0) {
    unNRZI(buffer,nrzi,ARRAYLEN);
    int numbytes=frame(hdlc,buffer,ARRAYLEN);
    //Serial.println(hexbuf2str(buf,hdlc,ARRAYLEN));
    return(numbytes);
  }

  return(-1);
}
