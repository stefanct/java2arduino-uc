/** \file
Arduino2j gateway code.

This file contains the most basic functions to allow communication between 
%j2arduino Java applications and arduino2j enabled functions.
It also includes some very basic arduino2j enabled convenience functions
(e.g. for debugging, retrieving a mapping between arduino2j jumptable offsets and the corresponding function names etc.).

Arduino2j provides an easy way to control Arduino applications from remote machines.
For documentation of the code of the "other" side please see j2arduino and especially {@link j2arduino.devices.Arduino Arduino}. */

#ifdef A2J

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "a2j_lowlevel.h"
#include "arduino2j.h"

static void a2jSendErrorFrame(uint8_t ret, uint8_t seq, uint16_t line);

#ifdef A2J_OPTS

	/** @name External jump table */
	//@{
	extern const PROGMEM jt_entry a2j_jt[];
	extern const uint8_t a2j_jt_elems;
	//@}

	/** @name External properties */
	//@{
	extern unsigned char* PROGMEM a2j_props[];
	extern const uint8_t a2j_props_size;
	//@}

#else // A2J_OPTS

	#ifdef __GNUC__
		#warning "Using default settings for arduino2j. To use your own, define them and call gcc with -D A2J_OPTS."
	#endif

	STARTJT
	ENDJT

	/** @name Default properties
	@see ::a2jGetProperties */
	//@{
	static unsigned char PROGMEM *a2j_props = NULL;
	static const uint8_t a2j_props_size = 0;
	//@}
#endif // A2J_OPTS

/** @name Default arduino2j functions*/
//@{
#ifdef A2J_FMAP
/** Fetches the function <-> name mapping from flash.
The mappings are stored as C-strings in flash, first function (index 0) of the jumptable first, then second etc.
This method retrieves them and puts them sequentially into memory starting at *datap.
\todo redo with a2jMany*/
uint8_t a2jGetMapping(uint8_t *const lenp, uint8_t* *const datap){
	uint8_t retlen = 0;
	// get total length of mapping strings
	for(uint8_t off=0; off<a2j_jt_elems; off++){
		retlen += strlen_P((PGM_P)pgm_read_word(&(a2j_jt[off].name)))+1;
	}

	char* data = (char*) *datap;
	// copy all mapping strings into *data
	*lenp = retlen;
	uint16_t space = retlen;
	for(uint8_t off=0; off<a2j_jt_elems; off++){
		uint8_t cpylen = strlcpy_P(data, (PGM_P)pgm_read_word(&(a2j_jt[off].name)), space) + 1;
		data += cpylen;
		if(cpylen > space){
			return -1;
		}
		space -= cpylen;
	}
	return 0;
}
#endif // A2J_FMAP

#ifdef A2J_DBG
/** Retrieves available characters from the debug buffer and puts them into memory starting at *datap.
@see debug.c#buf */
uint8_t a2jDebug(uint8_t *const lenp, uint8_t* *const datap){
	uint8_t len = rdCnt();
	uint8_t* buf = *datap;
	
	for(uint8_t i = 0; i < len; i++){
		buf[i] = rd();
	}
	*lenp = len;
	return 0;
}
#endif

/** Fetches the property strings from flash.
The properties are stored in pairs as consecutive C-strings in flash.
This method retrieves them and puts them sequentially in the memory starting at *datap. */
uint8_t a2jGetProperties(uint8_t *const lenp, uint8_t* *const datap){
	memcpy_P(*datap, a2j_props, a2j_props_size);
	*lenp = a2j_props_size;
	return 0;
}

/** Echoes back the data array sent over the stream. */
uint8_t a2jEcho(uint8_t *const lenp, uint8_t* *const datap){
	(void)datap;
	(void)lenp;
	return 0xBA;
}

/**@ingroup j2amany
Echoes back the data sent over the stream. */
uint8_t a2jEchoMany(uint8_t* isLastp, uint32_t *const offset, uint8_t *const lenp, uint8_t* *const datap){
	(void)isLastp;
	(void)offset;
	(void)lenp;
	(void)datap;
	return 0xBE;
}

/**@ingroup j2amany
Reads out the a2jMany-specific header from the start of \a *datap and 
calls the #CMD_P_MANY method accordingly.

This method uses a special \ref manyheader "header" to fetch an additional function pointer from \c a2j_jt
and to acquire the arguments needed to call the corresponding CMD_P_MANY method.

The callee can also send data back by just changing the "content" of the pointers.
After it returned the pointers for #a2jProcess will be set up correctly. */
uint8_t a2jMany(uint8_t *const lenp, uint8_t* *const datap){
	uint8_t len = *lenp;
	if(len < A2J_MANY_HEADER)
		return -1;

	len -= A2J_MANY_HEADER;
	uint8_t func = (*datap)[0];

	// limit offset to the size of the jumptable
	if(func >= a2j_jt_elems){
		*lenp = 0;
		return A2J_RET_OOB;
	}

	uint32_t offset = (uint32_t)(*datap)[2];
	uint8_t* ndatap = *datap + A2J_MANY_HEADER;

	#ifdef A2J_FMAP
	CMD_P_MANY cmd = (CMD_P_MANY)pgm_read_word(&(a2j_jt[func].cmd)); 
	#else
	CMD_P_MANY cmd = (CMD_P_MANY)pgm_read_word(&a2j_jt[func]);
	#endif

	(*datap)[0] = (*cmd)(*datap+1, &offset, &len, &ndatap);
	*lenp = len + A2J_MANY_HEADER;
	return 0;
}
//@}

/** Calls the method determined by the command field read from the stream and sends its reply back.
This function tries to read a packet according to the \ref prot "java2arduino protocol".
If this is successful the payload is read into the static array \c buf and
the function pointer at the offset equal to the command field is read out from the jump table \c a2j_jt.

The function pointer of type {@link #CMD_P} is then dereferenced with the properties of the payload as arguments.
Afterwards the method sends the return value of the callee, the length of the reply data and optionally
the reply data itself back and returns.
In the case of an error a special packet (see \ref j2aerrors, #a2jSendErrorFrame) is sent if possible before returning.*/
void a2jProcess(){
	if(!a2jReady() || !a2jAvailable() || a2jReadByte() != A2J_SOF)
		return;

	static uint8_t buf[256];
	uint8_t* payload = buf;

	// sequence number
	uint16_t tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, (uint8_t)tmp, __LINE__);
		return;
	}
	uint8_t seq = (uint8_t)tmp;

	 // function offset
	tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
		return;
	}

	// limit offset to the size of the jumptable
	if(tmp >= a2j_jt_elems){
		a2jSendErrorFrame(A2J_RET_OOB, seq, __LINE__);
		return;
	}
	uint8_t off = (uint8_t)tmp;
	
	// length of the data array
	tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
		return;
	}
	uint8_t len = (uint8_t)tmp;

	uint8_t csum = (uint8_t)(seq ^ (off + A2J_CRC_CMD) ^ (len + A2J_CRC_LEN));
	// read in payload // TODO 255B limit...?
	for(uint16_t i = 0; i < len; i++){
		tmp = a2jReadEscapedByte();
		if(tmp > 0xFF){
			a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
			return;
		}
		payload[i] = (uint8_t)tmp;
		csum ^= (uint8_t)tmp;
	}

	// read and compare checksum
	tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
		return;
	}

	uint8_t rsum = (uint8_t)tmp;
	if(csum != rsum){
		a2jSendErrorFrame(A2J_RET_CHKSUM, seq, __LINE__);
		return;
	}
	
	uint8_t *const lenp = &len; // const pointer to len
	uint8_t **bufp = &payload; // pointer to the data array
	
	// reading out the jump address from struct/pointer array in flash and calling it
	#ifdef A2J_FMAP
	CMD_P cmd = (CMD_P)pgm_read_word(&(a2j_jt[off].cmd)); 
	#else
	CMD_P cmd = (CMD_P)pgm_read_word(&a2j_jt[off]);
	#endif

	uint8_t ret = (*cmd)(lenp, bufp);
	if(ret == A2J_RET_OOB && cmd == &a2jMany){
		a2jSendErrorFrame(A2J_RET_OOB, seq, __LINE__);
		return;
	}
	
	// sending reply frame
	if(a2jWriteByte(A2J_SOF))
		return;
	if(a2jWriteEscapedByte(seq)) // sequence number
		return;
	if(a2jWriteEscapedByte(ret)) // return value
		return;
	if(a2jWriteEscapedByte(len)) // length
		return;

	// start new calculation of frame checksum
	csum = (uint8_t)(seq ^ (A2J_CRC_CMD + ret) ^ (A2J_CRC_LEN + len));

	// sending data array
	for(uint16_t i=0; i<len; i++){
		uint8_t tmp8 = payload[i];
		if(a2jWriteEscapedByte(tmp8))
			return;
		csum ^= tmp8;
	}
	if(a2jWriteEscapedByte(csum))
		return;
	a2jFlush();
}

/** Sends a frame indicating, that an error occurred.
@see arduino2jerrors*/
static void a2jSendErrorFrame(uint8_t ret, uint8_t seq, uint16_t line){
	if(a2jWriteByte(A2J_SOF))
		return;
	if(a2jWriteEscapedByte(seq)) // sequence number
		return;
	if(a2jWriteEscapedByte(ret)) // return value
		return;
	uint8_t len = 2;
	if(a2jWriteEscapedByte(len)) // length
		return;
	uint8_t lineu = (line>>8)%0xFF;
	if(a2jWriteEscapedByte(lineu)) // line
		return;
	uint8_t linel = line%0xFF;
	if(a2jWriteEscapedByte(linel)) // line
		return;
	if(a2jWriteEscapedByte((uint8_t)(seq ^ (A2J_CRC_CMD + ret) ^ (A2J_CRC_LEN+len) ^ lineu ^ linel))) // checksum
		return;
	a2jFlush();
}
#endif // A2J
