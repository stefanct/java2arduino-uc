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
#include <stdbool.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include "a2j_lowlevel.h"
#include "arduino2j.h"

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

static void a2jSendErrorFrame(uint8_t ret, uint8_t seq, uint16_t line);

#ifdef A2J_OPTS

	/** @name External jump table */
	//@{
	extern const PROGMEM jt_entry a2j_jt[];
	extern const uint8_t a2j_jt_elems;
	//@}

#ifdef A2J_PROPS
	/** @name External properties */
	//@{
	extern char const PROGMEM a2j_props[];
	extern const uint32_t a2j_props_size;
	//@}
#endif // A2J_PROPS

#else // A2J_OPTS

	#ifdef __GNUC__
		#warning "Using default settings for arduino2j. To use your own, define them and call gcc with -D A2J_OPTS."
	#endif

	STARTJT
	ENDJT
#endif // A2J_OPTS

uint16_t a2jReadEscapedByte(){
	// we need either one unescaped byte...
	uint16_t data;
	if((data = a2jReadByte()) > 0xFF)
		return data;
	if(data == A2J_ESC){
		// ... or an escape character + the escaped byte
		if((data = a2jReadByte()) > 0xFF){
			return data;
		}
		data += 1;
	} else if (data == A2J_SOF || data == A2J_SOS)
		return -A2J_RET_ESC; // Unescaped delimiter character inside frame

	return data;
}

uint8_t a2jWriteEscapedByte(uint8_t data){
	if(data == A2J_SOF || data == A2J_SOS || data == A2J_ESC){
		uint8_t err = a2jWriteByte(A2J_ESC);
		if(err)
			return err;
		return a2jWriteByte(data-1);
	}else
		return a2jWriteByte(data);
}

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

#ifdef A2J_PROPS
/** Fetches the property strings from flash.
The properties are stored in pairs as consecutive C-strings in flash.
This method retrieves them using a2jMany in the most obvious way,
namely by sliding the a2jMany window over the string as requested.
Writes are currently not possible. */
uint8_t a2jGetProperties(bool* isLastp, bool isWrite, uint32_t *const offset, uint8_t *const lenp, uint8_t* *const datap){
	if (isWrite || (*offset >= a2j_props_size))
		return -1;
	*lenp = min(a2j_props_size - *offset, A2J_MANY_PAYLOAD);
	memcpy_P(*datap, a2j_props + *offset, *lenp);
	*isLastp = (*offset + *lenp) == a2j_props_size;

	return 0;
}
#endif // A2J_PROPS

static uint8_t a2jSend_int(uint8_t start_byte, uint8_t cmd, uint8_t seq, uint8_t len, uint8_t* const data){
	uint16_t i = 1;
	while(!a2jReady()){
		a2jTask();
		if(i-- == 0) {
			return 10;
		}
	}

	if(a2jWriteByte(start_byte)) {
		return 11;
	}

	if(a2jWriteEscapedByte(seq)){ // sequence number
		return 12;
	}
	if(a2jWriteEscapedByte(cmd)){ // client id
		return 13;
	}
	if(a2jWriteEscapedByte(len)){ // length
		return 14;
	}
	uint8_t csum = (uint8_t)(seq ^ (cmd + A2J_CRC_CMD) ^ (len + A2J_CRC_LEN));
	for(i = 0; i < len; i++){
		uint8_t tmp =  data[i];
		a2jWriteEscapedByte(tmp);
		csum ^= tmp;
	}
	if(a2jWriteEscapedByte(csum)){ // checksum
		return 15;
	}
	a2jFlush();
	return 0;
}

#ifdef A2J_SIF
static volatile bool sif_mutex = 0;

/** Sends a server-initiated frame (i.e. without being polled by the client). */
uint8_t a2jSendSif(uint8_t cmd, uint8_t len, uint8_t* const data){
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		if(sif_mutex == 1){
			return -1;
		}
		sif_mutex = 1;
	}
	static uint8_t seq = 0;
	uint8_t ret = a2jSend_int(A2J_SOS, cmd, seq, len, data);
	seq++;
	sif_mutex = 0;
	return ret;
}
#endif // A2J_SIF

/** Echoes back the data array sent over the stream. */
uint8_t a2jEcho(uint8_t *const lenp, uint8_t* *const datap){
	(void)datap;
	(void)lenp;
	return 0xBA;
}

/**@ingroup j2amany
Echoes back the data sent over the stream. */
uint8_t a2jEchoMany(bool* isLastp, bool isWrite, uint32_t *const offset, uint8_t *const lenp, uint8_t* *const datap){
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

	uint8_t flags = (*datap)[1];
	bool isLast = flags & A2J_MANY_ISLAST_MASK;
	bool isWrite = flags & A2J_MANY_ISWRITE_MASK;
	(*datap)[0] = (*cmd)(&isLast, isWrite, &offset, &len, &ndatap);
	*lenp = len + A2J_MANY_HEADER;
	(*datap)[1] = isLast << A2J_MANY_ISLAST_BIT;
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
	if(!a2jReady() || !a2jAvailable())
		return;

#ifdef A2J_SIF
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		if(sif_mutex == 1){
			return;
		}
		sif_mutex = 1;
	}
#endif // A2J_SIF
	if (a2jReadByte() != A2J_SOF)
		goto out;

	static uint8_t buf[256];
	uint8_t* payload = buf;

	// sequence number
	uint16_t tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, (uint8_t)tmp, __LINE__);
		goto out;
	}
	uint8_t seq = (uint8_t)tmp;

	 // function offset
	tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
		goto out;
	}
	uint8_t off = (uint8_t)tmp;

	// limit offset to the size of the jumptable
	if(off >= a2j_jt_elems){
		a2jSendErrorFrame(A2J_RET_OOB, seq, __LINE__);
		goto out;
	} else {
		#ifndef A2J_FMAP
			if (off == 0) {
				a2jSendErrorFrame(A2J_RET_OOB, seq, __LINE__);
				goto out;
			}
		#endif
	}
	
	// length of the data array
	tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
		goto out;
	}
	uint8_t len = (uint8_t)tmp;

	uint8_t csum = (uint8_t)(seq ^ (off + A2J_CRC_CMD) ^ (len + A2J_CRC_LEN));
	// read in payload // TODO 255B limit...?
	for(uint16_t i = 0; i < len; i++){
		tmp = a2jReadEscapedByte();
		if(tmp > 0xFF){
			a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
			goto out;
		}
		payload[i] = (uint8_t)tmp;
		csum ^= (uint8_t)tmp;
	}

	// read and compare checksum
	tmp = a2jReadEscapedByte();
	if(tmp > 0xFF){
		a2jSendErrorFrame(A2J_RET_TO, seq, __LINE__);
		goto out;
	}

	uint8_t rsum = (uint8_t)tmp;
	if(csum != rsum){
		a2jSendErrorFrame(A2J_RET_CHKSUM, seq, __LINE__);
		goto out;
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
		goto out;
	}

	if (a2jSend_int(A2J_SOF, ret, seq, len, payload))
		goto out;

out:
#ifdef A2J_SIF
	sif_mutex = 0;
#endif // A2J_SIF
	return;
}

/** Sends a frame indicating, that an error occurred.
@see arduino2jerrors*/
static void a2jSendErrorFrame(uint8_t err, uint8_t seq, uint16_t line){
	if(a2jWriteByte(A2J_SOF))
		return;
	if(a2jWriteEscapedByte(seq)) // sequence number
		return;
	if(a2jWriteEscapedByte(err)) // return value
		return;
	uint8_t len = 2;
	if(a2jWriteEscapedByte(len)) // length
		return;
	uint8_t lineu = (line >> 8) & 0xFF;
	if(a2jWriteEscapedByte(lineu)) // line
		return;
	uint8_t linel = line & 0xFF;
	if(a2jWriteEscapedByte(linel)) // line
		return;
	if(a2jWriteEscapedByte((uint8_t)(seq ^ (A2J_CRC_CMD + err) ^ (A2J_CRC_LEN+len) ^ lineu ^ linel))) // checksum
		return;
	a2jFlush();
}

#endif // A2J
