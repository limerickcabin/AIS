#define ARRAYLEN 100
uint8_t hdlc[ARRAYLEN];
uint8_t block[ARRAYLEN];
char    nmea[ARRAYLEN];
float    LAT=   20.7500;  //Punta Mita anchorage
float    LON= -105.5000;   
uint32_t MMSI= 367499000;

uint16_t buildHDLC(uint8_t *block, uint8_t length){
  //builds a bit stuffed hdlc packet with preamble and flags from block[] of length
  uint16_t bitCount,oneCount,bits,index,bitsinbits;

  //glue the crc on the end of block
  int crc=crc16_ibm_sdlc(block,length);
  block[length++]=crc      & 0xff;
  block[length++]=(crc>>8) & 0xff;

  //start building hdlc
  //note the bits in bytes are backwards from convention
  for (int i=0;i<ARRAYLEN;i++) hdlc[i]=0;
  storeBits(hdlc,0xaaaaaa,0,24); //preample
  storeBits(hdlc,0x7e,24,8);     //flag
  bitCount=32;
  index=4;
  for (int i=0;i<length;i++){
    uint8_t b=block[i];         //take a bite
    for (int j=0;j<8;j++) {     //spit it out bit by bit, lsb first
      if (b&(1<<j)) {
        oneCount+=1;
        bits=bits | (1<<bitsinbits);
        bitsinbits+=1;
      }
      else {
        oneCount=0;
        bitsinbits+=1;
      }

      if (oneCount==5) {        //stuff a bit if necessary
        Serial.printf("stuffed a bit at %d\n",bitCount+bitsinbits);
        oneCount=0;
        bitsinbits+=1;
      }
    } //end of incoming byte
    hdlc[index++]=bits&255;
    bitCount+=8;
    bits>>=8;
    bitsinbits-=8;
  } //end of incoming array
  bits|=0x7e<<bitsinbits;
  hdlc[index++]=bits&0xff;
  if (bitsinbits>8) hdlc[index]=bits>>8;
  bitCount+=bitsinbits;
  return(bitCount);
}

void buildNMEA(uint8_t *block) {  
//build NMEA sentence
//armor the block into a string, 6 bits at a time
  char s[88];
  char c;
  uint16_t charPos=0;
  uint32_t threeByte;

  for (int i=0;i<21;i+=3) { //do 3 bytes (24 bits) at a time creating 4 6-bit characters
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

void buildBlock(uint8_t *block, float lat, float lon, uint32_t mmsi){
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
}

uint8_t * testStore(void) {
  storeBits(block, -1, 0, 6);
  for (int i=0;i<8;i++){
    Serial.printf("%02x ",block[i]);
    block[i]=0;
  }
  return(block);
}

void testBlock(void) {
}

void tests(void) {
  float lat=47;
  float lon=-122;
  uint32_t mmsi=367499470;
  char goodNMEA[]="!AIVDO,1,1,,,15NNHkUP00GAQl0Jq<@>401p0<0m,0*21";

  buildBlock(block,lat,lon,mmsi);
  buildNMEA(block);
  if (strcmp(nmea,goodNMEA)==0) Serial.println("good NMEA");
  else Serial.println("bad NMEA");
  
  buildHDLC(block,21);
  for (int i=32;i>=0;i--) Serial.printf("%02x",hdlc[i]);
  Serial.println();

  buildBlock(block,20.75,-105.5,367499000); //punta
  buildNMEA(block);
  Serial.println(nmea);
  
  int count=buildHDLC(block,21);
  for (int i=0;i<(count/8+1);i++){
    Serial.printf("%02x",hdlc[i]);
  }
  Serial.println();
}

