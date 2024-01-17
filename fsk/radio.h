#ifndef _radioh
#define _radioh
//structure with AIS payload information
struct AISinfo {
  float    lat;
  float    lon;   
  uint32_t mmsi;
  uint8_t  nav;
  char     call[8];
  char     name[21];
  char     dest[21];
  uint32_t cog;
  uint32_t sog;
  uint32_t hdg;
};
#endif