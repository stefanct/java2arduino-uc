/** \file
Arduino2java gerneric lowlevel abstraction interface implementation.*/

#include "a2j_lowlevel.h"

uint16_t a2jReadEscapedByte(){
	// we need either one unescaped byte...
	uint16_t data = a2jReadByte();
	if(data == A2J_SOF){
		return -1;
	}
	if(data != A2J_ESC){
		return data;
	}

	// ... or an escape character + the escaped byte
	if((data = a2jReadByte()) > 0xFF){
		return -1;
	}
	return data+1;
}

uint8_t a2jWriteEscapedByte(uint8_t data){
	if(data == A2J_SOF || data == A2J_ESC){
		uint8_t err = a2jWriteByte(A2J_ESC);
		if(err)
			return err;
		return a2jWriteByte(data-1);
	}else
		return a2jWriteByte(data);
}

