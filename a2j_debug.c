/** \file
Debugging driver.*/

#include <inttypes.h>

#ifdef A2J_DBG
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "debug.h"

/** @name Buffer related variables
These are responsible to control access to the debug buffer */
//@{
/** The number of characters the debug buffer can hold.
Needs to be a number of two. */
#define A2J_DBG_CNT 256
/** All index operations are ANDed with this mask to emulate modulo.
Therefore it has to match #A2J_DBG_CNT, and #A2J_DBG_CNT needs to be a power of two.*/
#define A2J_DBG_MSK (0xFF)
/** Buffer of #A2J_DBG_CNT bytes/characters organized as ring buffer */
static uint8_t buf[A2J_DBG_CNT];
/** Index for read operations.*/
static uint8_t rdOff = 0;
/** Index for write operations.*/
static uint8_t wrOff = 0;
//@}

/** Returns the number of yet unread characters. */
inline uint8_t rdCnt(){
	return (wrOff - rdOff)&A2J_DBG_MSK;
}

/** Returns the number of free bytes in the buffer #buf. */
inline uint8_t wrCnt(){
	return (rdOff - 1 - wrOff)&A2J_DBG_MSK;
}

/** Appends character \a c to the buffer.
Characters are only added if there is space left in #buf.*/
uint8_t wr(char c){
	if(wrCnt()){
		buf[(wrOff++)&A2J_DBG_MSK] = c;
		return 1;
	}
	return 0;
}

/** Formats \a val as decimal w/o leading zeros and appends the characters to the buffer.
Upto 5 characters are added to #buf.
Characters are only added if there is space left in #buf for all.
@return the number of characters written. */
uint8_t wrDec16(uint16_t val){
	if(wrCnt()>=5){
		uint8_t dec[5];
		for(uint8_t i=0; i<=4; i++){
			dec[i] = val % 10;
			val = val / 10;
		}
		uint8_t i=4;
		uint8_t cnt=0;
		while(i){
			if(dec[i]!=0){
				buf[(wrOff++)&A2J_DBG_MSK] = '0'+dec[i];
				cnt++;
			}
			i--;
		}
		buf[(wrOff++)&A2J_DBG_MSK] = '0'+dec[0];
		return 1+cnt;
	}
	return 0;
}

/** Formats \a val as hexadecimal with prefix '0x' and appends the characters to the buffer.
Characters are only added if there is space left in #buf for all.
@return the number of characters written (always 6). */
uint8_t wrHex16(uint16_t val){
	if(wrCnt()>=6){
		buf[(wrOff++)&A2J_DBG_MSK] = '0';
		buf[(wrOff++)&A2J_DBG_MSK] = 'x';

		for(uint8_t i=3; i!=0xFF; i--){
			uint8_t test = (val>>(4*i))&0xF;
			buf[(wrOff++)&A2J_DBG_MSK] = (test<0x0A)?'0'+(test):'A'-10+(test);
		}
		return 1;
	}
	return 0;
}

/** Formats \a val as hexadecimal with prefix '0x' and appends the characters to the buffer.
Characters are only added if there is space left in #buf for all.
@return the number of characters written (always 4). */
uint8_t wrHex(uint8_t val){
	if(wrCnt()>=4){
		buf[(wrOff++)&A2J_DBG_MSK] = '0';
		buf[(wrOff++)&A2J_DBG_MSK] = 'x';

		buf[(wrOff++)&A2J_DBG_MSK] = (val<0xA0)?'0'+((val&0xF0)>>4):'A'-10+((val&0xF0)>>4);
		val &= 0x0F;
		buf[(wrOff++)&A2J_DBG_MSK] = (val<0x0A)?'0'+(val):'A'-10+(val);
		return 4;
	}
	return 0;
}

/** Reads the oldest unread character from the buffer.
@return the oldest unread character or 0 if there is none. */
uint8_t rd(){
	if(rdCnt())
		return buf[(rdOff++)&A2J_DBG_MSK];
	else
		return 0;
}

/** Appends \a str to the buffer.
Characters are added to the buffer as long as there is space in #buf.
@return the number of characters written. */
uint8_t wrStr(char *str){
	uint8_t i=0;
	char c;
	while((c = str[i++])){
		if(!wr(c))
			break;
	}
	return i;
}

// todo test
/** Reads \a str from flash and appends it to the buffer.
Characters are added to the buffer as long as there is space in #buf.
@return the number of characters written. */
uint8_t wrStr_P(const char *str){
	uint8_t i=0;
	char c;
	while ((c = pgm_read_byte(str+i++))){
		if(!wr(c))
			break;
	}
	return i;
}
#endif
