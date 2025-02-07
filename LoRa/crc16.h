#ifndef _crch
#define _crch
uint16_t crcTable[256];
uint16_t crcINIT=0,crcEND=0;
bool     crcInited=false;

//initializes the crc values - defaults to IBM_SDLC or what AIS uses
void initCRC(uint16_t poly=0x8408, uint16_t init=0xffff, uint16_t ending=0xffff) { //defaults for AIS' CRC
  crcINIT=init;   //beginning CRC value
  crcEND=ending;  //xor value of result
  //build table using poly
  for (int i=0;i<256;i++) {
    uint16_t crc=i;
    for (int j=0;j<8;j++){
      crc=(crc>>1) ^ (poly & (-(crc&1)));
    }
    crcTable[i]=crc;
  }
  crcInited=true;
}

/*
returns CRC-16 of buf, length len using parameters initialized by crcInit()
*/
int crc16(const uint8_t *buf, int len)
{
    uint16_t crc;
    int i;

    if (!crcInited) initCRC();
    
    crc = crcINIT;
    for (i = 0;  i < len;  i++)
        crc = (crc >> 8) ^ crcTable[(crc ^ buf[i]) & 0xFF];
    return (crc ^ crcEND);
}
#endif
