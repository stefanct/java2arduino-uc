#ifdef A2J

#include <stdint.h>

#ifdef A2J_SERIAL
#include "debug.h"
#include "a2j_lowlevel.h"
#include "a2j_lowlevel_serial.h"

inline void a2jLLInit_serial(void){
	serialInit(115200);
}

void a2jLLTask_serial(void){
	;
}

uint8_t a2jLLReady_serial(void){
    return 1;
}

uint8_t a2jLLAvailable_serial(void){
    return serialAvailableCnt();
}

uint8_t a2jReadByte_serial(){
	return serialReadNoWait();
}

uint16_t a2jReadEscapedByte_serial(){
	if(!serialAvailCntWait(1, A2J_TIMEOUT)){
		return -1;
	}

	// we need either one unescaped byte...
	uint8_t data = serialReadNoWait();
	//if(data == A2J_SOF)
		// TODO: return "malformed frame", but checksum will save us, hopefully.
		// problem: we don't know the sequence number here.
	if(data != A2J_ESC){
		return data;
	}

	// ... or an escape character + the escaped byte
	if(!serialAvailCntWait(1, A2J_TIMEOUT)){
		return -1;
	}
	return serialReadNoWait()+1;
}

void a2jWriteByte_serial(uint8_t data){
	serialWriteBlock(data);
}

void a2jWriteEscapedByte_serial(uint8_t data){
	if(data == A2J_SOF || data == A2J_ESC){
		a2jWriteByte_serial(A2J_ESC);
		a2jWriteByte_serial(data-1);
	}else
		a2jWriteByte_serial(data);
}

void a2jFlush_serial(void){
	// TODO
}

#endif // A2J_SERIAL
#endif // A2J
