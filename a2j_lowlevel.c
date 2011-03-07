/** \file
Arduino2java gerneric lowlevel abstraction interface implementation.*/

#include "a2j_lowlevel.h"

uint16_t a2jReadEscapedByte(){
	// we need either one unescaped byte...
	uint16_t data = a2jReadByte();
	//if(data == A2J_SOF)
		// TODO: return "malformed frame", but checksum will save us, hopefully.
		// problem: we don't know the sequence number here.
	if(data != A2J_ESC){ // this includes errors
		return data;
	}

	// ... or an escape character + the escaped byte
	if((data = a2jReadByte()) > 0xFF){
		return -1;
	}
	return data+1;
}

void a2jWriteEscapedByte(uint8_t data){
	if(data == A2J_SOF || data == A2J_ESC){
		a2jWriteByte(A2J_ESC);
		a2jWriteByte(data-1);
	}else
		a2jWriteByte(data);
}

