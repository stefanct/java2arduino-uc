/** \file
Debugging driver header.*/
#ifndef DEBUG_H
#define DEBUG_H


#ifdef A2J_DBG

uint8_t rdCnt(void);
uint8_t wrCnt(void);
uint8_t wr(char c);
uint8_t wrDec16(uint16_t val);
uint8_t wrHex(uint8_t val);
uint8_t wrHex16(uint16_t val);
uint8_t wrStr(char *str);
uint8_t wrStr_P(const char *str);
uint8_t rd(void);

#else

#define rdCnt() 0
#define wrCnt() 0
#define wr(c) 0 /* c */
#define wrDec16(c) 0 /* c */
#define wrHex(c) 0 /* c */
#define wrHex16(c) 0 /* c */
#define wrStr(c) 0 /* c */
#define wrStr_P(c) 0 /* c */
#define rd() 0

#endif // A2J_DBG
#endif // DEBUG_H
