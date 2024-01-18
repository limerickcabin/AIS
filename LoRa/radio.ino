#include <RadioLib.h>
#include "radio.h"
#include "crc16.h"

#define ARRAYLEN 256
uint8_t   nrzi[ARRAYLEN];
uint8_t   hdlc[ARRAYLEN];
uint8_t  block[ARRAYLEN];
uint8_t buffer[ARRAYLEN];
char      nmea[ARRAYLEN];
char       buf[ARRAYLEN*2+1];

// SX1262 has the following connections on Heltec V3:
// NSS pin:   8
// DIO1 pin:  14
// NRST pin:  12
// BUSY pin:  13
SX1262 radio = new Module(8,14,12,13);

//initialize SX1262 for AIS operation
void radioSetup() {
  initCRC();
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
  state = radio.setPreambleLength(16);
  state = radio.fixedPacketLengthMode();
  uint8_t sync[]={0x01,0x23,0x45,0x67,0x89,0x0ab,0xcd,0xef};
  state = radio.setSyncWord(sync, 0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("Unable to set configuration, code: %d\n",state);
  }
  
}

/*************************** AIS STUFF *************************/
//todo: more dynamics in position report (make a struct)

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
   storing in hdlc until next flag. If the packet CRCs, return number of bytes of packet including flags
   otherwise start looking again using the last flag as the first
 */
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
uint16_t buildPacket(byte *output, AISinfo a){
  uint16_t numBytes;
  numBytes=buildBlock(block,a);
  numBytes=buildHDLCnew(block,numBytes);  
  buildNRZInew(output,hdlc,numBytes);
  return(numBytes);
}

/*
    performs NRZI on hdlc of length len and storing in nrzi
*/
void buildNRZInew(byte * nrzi, byte * hdlc, int len) {
  uint8_t bit,byte,sample=0;

  for (int i=0;i<len;i++){      //least significant byte is first 
    byte=0;
    for (int j=7;j>=0;j--){     //first bit is most significant in data - needs to end up on the left
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
note that the bits are reversed per convention: first transmitted bit is on the left
*/
uint16_t buildHDLCnew(uint8_t *block, uint8_t length){
  uint32_t bitCount,oneCount;

  //start building hdlc
  storeBits(hdlc);                    //reset
  storeBits(hdlc,0x555555555555,24);  //preamble
  storeBits(hdlc,0x7e,8);             //flag
  
  oneCount=0;
  for (int i=0;i<length;i++){             //bite by bite
    for (int j=0;j<8;j++) {               //spit it out bit by bit, lsb first (the direction of transmit)
      storeOneBit(hdlc,block[i]&(1<<j));  //stash one bit and stuff if necessary
    }
  }
  //add crc
  uint16_t crc=crc16(block,length);
  for (int j=0;j<16;j++) {                //spit it out bit by bit
    storeOneBit(hdlc,crc&(1<<j));
  }
  //add flag
  bitCount=storeBits(hdlc,0x7e,8);

  //return number of bytes in hdlc
  if (bitCount%8==0) return(bitCount/8);
  else return(bitCount/8+1);   //round up one byte if a few bits are left over
}

//build NMEA sentence
//armor the block into a string, 6 bits at a time
void buildNMEA(uint8_t *block, uint16_t numBytes) {  
  char s[88];
  char c;
  uint16_t charPos=0;
  uint32_t threeByte;

  for (int i=0;i<numBytes;i+=3) { //do 3 bytes (24 bits) at a time creating 4 6-bit characters
    threeByte =block[i  ]<<16;    //first is msByte
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

uint16_t storeBitsOffset;
uint16_t storeOneBitOnes;
/*
place bits amount of source storeBitsOffset number of bits into dest[]
calling with just block resets offset to 0 incrementing by width bits each subsequent call
returns offset that will be used by next call (length in bits)
*/
uint16_t storeBits(uint8_t *dest, uint32_t source, uint16_t bits) {
  uint8_t  bitOffset=storeBitsOffset%8;
  uint8_t  byteOffset=storeBitsOffset/8;
  uint8_t  shift=64-bits-bitOffset;
  uint64_t mask=(1<<bits)-1;
  uint64_t shiftedSource=((uint64_t)(source) & mask)<<shift; //msbyte of source is now aligned in msbyte of shiftedSource
  //Serial.printf("shifted source %llx\n",shiftedSource);
  for (int i=0;i<8;i++){
    dest[i+byteOffset]|=shiftedSource>>(64-8);
    shiftedSource<<=8;
    if (shiftedSource==0) break;
  }
  storeBitsOffset+=bits;
  return(storeBitsOffset);
}

// clears dest and resets storing at offset 0
uint16_t storeBits(uint8_t *dest) {
  storeBitsOffset=0;   //starting over
  storeOneBitOnes=0;
  for (int i=0;i<ARRAYLEN;i++) dest[i]=0;  //todo: length of dest should be passed not assumed
  return(0);
}

// store one bit, bit stuffing as necessary, returns number of bits stored so far
uint16_t storeOneBit(uint8_t *dest, bool bit){
  uint16_t numBits;
  //store the bit
  numBits=storeBits(dest,bit,1);
  //count the ones
  if (bit) storeOneBitOnes+=1;
  else     storeOneBitOnes=0;
  //stuff if necessary
  if (storeOneBitOnes==5) {
    storeOneBitOnes=0;
    numBits=storeBits(dest,0,1);
  }
  return(numBits);
}

//builds static and voyage related block
//file:///C:/Users/limer/Documents/GitHub/AIS/Docs/ais.html#_type_5_static_and_voyage_related_data
uint16_t buildSVblock(uint8_t* block, AISinfo a) {
  storeBits (block);
  storeBits (block,5,      6);     //store 5 in a 6 bit wide bit field
  storeBits (block,0,      2);     //repeat, 3=do not repeat
  storeBits (block,a.mmsi,30);     //mmsi 
  storeBits (block,0,      2);     //version
  storeBits (block,0,     30);     //imo
  str2sixbit(block,a.call, 7);     //call sign (note len is number of chars, not bits)
  str2sixbit(block,a.name,20);     //name
  storeBits (block,37,     8);     //ship type: pleasure
  storeBits (block,10,     9);     //to bow
  storeBits (block,10,     9);     //to stern
  storeBits (block,3,      6);     //to port
  storeBits (block,3,      6);     //to starboard
  storeBits (block,1,      4);     //epfd
  storeBits (block,11,     4);     //month
  storeBits (block,22,     5);     //day
  storeBits (block,23,     5);     //hour
  storeBits (block,3,      6);     //minute
  storeBits (block,17,     8);     //draft
  str2sixbit(block,a.dest,20);
  storeBits (block,1,      1);     //DTE
  uint16_t offset = 
  storeBits (block,1,      1);     //spare
  if (offset!=424) Serial.printf("storeBits returned %d, should have been 424\n",offset);
  return(offset/8);
}

/*
    block is an array containing arbitrary-width bit fields forming an AIS position report
    the fields are packed left to right (msb of first byte on the left -> lsb of last byte on right)
    file:///C:/Users/limer/Documents/GitHub/AIS/Docs/ais.html#_types_1_2_and_3_position_report_class_a
 */
uint16_t buildBlock(uint8_t *block, AISinfo a){
  uint32_t latdmm=int(a.lat*600000); //degrees -> deci-milli minutes
  uint32_t londmm=int(a.lon*600000);

  storeBits(block);               //reset block  

  storeBits(block, 1,      6);    //message type
  storeBits(block, 0,      2);    //repeat
  storeBits(block, a.mmsi,30);    //MMSI
  storeBits(block, a.nav,  4);    //status
  storeBits(block, 128,    8);    //ROT
  storeBits(block, a.sog, 10);    //SOG
  storeBits(block, 0,      1);    //integrity
  storeBits(block, londmm,28);    //longitude, minutes*10000
  storeBits(block, latdmm,27);    //latitude
  storeBits(block, a.cog, 12);    //COG, degrees*10, 3600=unknown
  storeBits(block, a.hdg,  9);    //HDG
  storeBits(block, 60,     6);    //time, seconds
  storeBits(block, 0,      2);    //maneuver
  storeBits(block, 0,      3);    //spare
  storeBits(block, 0,      1);    //RAIM
  uint16_t offset = 
  storeBits(block, 49205, 19);    //Radio Status
  if (offset!=21*8) Serial.printf("block has a problem - storeBits returned %d\n",offset);
  return(offset/8);
}

//convert str to six bit ascii that is storeBits into block
//len is the max number of characters padding out with spaces if necessary
void str2sixbit(uint8_t* block, char* str, uint16_t len) {
  char c;
  int  i;
  for (i=0;i<len;i++) {
    c=str[i];
    if (c==0) break;      //end of incoming string
    c&=(64-1);
    storeBits(block,c,6); //stash the 6-bit
  }
  //pad out with spaces
  while (i<len) {
    storeBits(block,' ',6);
    i+=1;
  }
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
  AISinfo aisTest={47,-122,367499470,5,"CQ","ME","HERE",3600,0,0};
  //known good results
  char goodNMEA[]    PROGMEM = "!AIVDO,1,1,,,15NNHkUP00GAQl0Jq<@>401p0<0m,0*21";
  char goodNRZI[]    PROGMEM = "ccccccfe95e6fbd1bd515595a7ea549d484b8552aaa0aaabceb4d580aa";
  char goodSVblock[] PROGMEM = "14579e6338000000003460820820345820820820820820820820820820250502830c6ed70c44815216082082082082082082082083";
  
  bool ok=true;
  uint16_t numBytes;

  //test static/voyage packet
  numBytes=buildSVblock(block,aisTest);  
  hexbuf2str(buf,block,numBytes);
  if (strcmp(buf,goodSVblock)) {
    Serial.println("bad SV block");
    Serial.println(buf);
    ok=false;
  }

  //test NMEA
  numBytes=buildBlock(block,aisTest);
  buildNMEA(block,numBytes);
  if (strcmp(nmea,goodNMEA)){
	  Serial.println("bad NMEA");
    Serial.println(nmea);
	  ok=false;
  }

  //test transmit chain
  numBytes=buildPacket(nrzi,aisTest);
  hexbuf2str(buf,nrzi,numBytes);
  if (strcmp(buf,goodNRZI)) {
	  Serial.println("bad NRZI");
    Serial.println(buf);
   ok=false;
  }

  //test decode chain
  unNRZI(buffer,nrzi,numBytes);                     //decode previously built NRZI
  numBytes=frame(hdlc,buffer,numBytes);             //find valid hdlc packet
  if (numBytes!=25) {                               //position reports are 21 bytes info, 2 flags, 2 bytes crc
      hexbuf2str(buf, hdlc, numBytes);              //printable hdlc
      Serial.printf("bad unNRZI or frame - destuffed, reversed hdlc:\n%s\n",buf);
      ok=false;
  }

  return(ok);
}

/*
 * Builds and transmit AIS position report packet using lat, lon in degrees, mmsi and nav status
 * along with SV packet using mmsi, call, name and dest
 */
int transmitAIS(AISinfo ais){
  //build and transmit position packet on both frequencies
  uint16_t numBytes;
  int state;

  numBytes=buildPacket(nrzi,ais);
  radio.setFrequency(162.025);
  state = radio.transmit(nrzi,numBytes);
  radio.setFrequency(161.975);
  state |= radio.transmit(nrzi,numBytes);

  //build and transmit static/voyage packet
  numBytes=buildSVblock(block,ais);  
  numBytes=buildHDLCnew(block,numBytes);  
  buildNRZInew(nrzi,hdlc,numBytes);
  state |= radio.transmit(nrzi,numBytes);

  return(state);
}

//receive a packet, nrzi it, and try to frame an hdlc frame
//returns number of bytes including the flags or -1 if incomplete frame
int receiveAIS(void) {
  for (int i=0;i<ARRAYLEN;nrzi[i++]=0);
  
  if (radio.receive(nrzi,255)==0) {
    unNRZI(buffer,nrzi,ARRAYLEN);
    int numbytes=frame(hdlc,buffer,ARRAYLEN);
    if (numbytes==25) { //position reports are 21 bytes +2 crc +2 flags
      Serial.printf("%s\n",hexbuf2str(buf,hdlc,numbytes));
    }
    return(numbytes);
  }
  return(-1);
}
