#ifdef A2J
#ifdef A2J_SERIAL

#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include "a2j_lowlevel.h"
#include "a2j_lowlevel_serial.h"
#include "serial.h"

// use -D SERIAL_BAUD <baudrate> as compiler flag
inline void a2jInit(void){
	serialInit();
}

void a2jTask(void){
	;
}

uint8_t a2jReady(void){
    return true;
}

uint8_t a2jAvailable(void){
    return serialReadAvailableCnt();
}

uint16_t a2jReadByte(){
	uint8_t cnt = A2J_TIMEOUT;
	for(;cnt>0;cnt--){
		if (serialReadIsAvailable()){
			return serialReadNoWait();
		}
		_delay_ms(1);
	}
	return -1;
}

void a2jWriteByte(uint8_t data){
	serialWriteBlock(data);
}

void a2jFlush(void){
	// TODO
}

#endif // A2J_SERIAL
#endif // A2J
