//0...4 are sat and rest to tv
#include <avr/pgmspace.h>
const uint8_t rc5_code_data[] PROGMEM ={
0x45,
0x49,
0x50,
0x51,
0x57,
0x41,
0x42,
0x43,
0x4A,
0x4C,
0x4D,
0x53,
0x54,
0x55,
0x56,
0x59,
0x60,
0x61,
0x6A,
0x70,
0x71,
0x73,
0x75,
0x78	}; //0x78	

const uint16_t nec_code_data[] PROGMEM ={
0xA05F ,
0x906F ,
0xA45B ,
0x18E7 ,
0x48B7 ,
0x738C,
0x33CC ,
0x9768 ,
0x14EB ,
0x10EF ,
0x906F ,
0x827D ,
0x02FD ,
0xE01F ,
0x609F ,
0x0DF2 ,
0x40BF ,
0xC03F ,
0xA956 ,
0x3EC1 ,
0x5DA2 ,
0xDA25 ,
0x22DD ,
0xD02F 		}; //0xD02F 