/** \file
Arduino2java generic lowlevel abstraction interface.*/

#ifndef A2J_LL_H
#define A2J_LL_H

#include <stdint.h>
#include "arduino2j.h"

#ifdef A2J
	#if defined(A2J_SERIAL) && defined(A2J_USB)
		#error "multiple a2j low level functions enabled. please define either A2J_SERIAL _or_ A2J_USB"
	#endif
	#if !(defined(A2J_SERIAL) || defined(A2J_USB))
		#error "no a2j low level implementation selected. please define A2J_SERIAL or A2J_USB"
	#endif

/** Indicates wheter the underlying stream layer is connected and ready.

@return 0 if not ready */
uint8_t a2jReady(void);

/** Indicates wheter there is at least one byte available to read from the underlying stream.

@return 0 if not available */
uint8_t a2jAvailable(void);

/** Reads one byte from the stream.

Tries to read a raw byte from the stream with a timeout of #A2J_TIMEOUT milliSeconds.
@return a value [0; 255] on success*/
uint16_t a2jReadByte(void);

/** Reads one byte from the stream.

Tries to read a raw byte from the stream with a timeout of #A2J_TIMEOUT milliSeconds.
If that byte indicates escaping, another raw byte is read and returned, after it has been incremented.
@return the next byte [0; 255] after de-escaping on success
@see arduino2framing*/
uint16_t a2jReadEscapedByte(void);

/** Writes one byte to the stream.

@param data the byte to write
@return 0 on success */
uint8_t a2jWriteByte(uint8_t data);

/** Writes a byte to the underlying stream.
If the given argument has to be escaped, it writes #A2J_ESC first and then \a data-1.
@param data the byte to write
@return 0 on success
@see arduino2framing */
uint8_t a2jWriteEscapedByte(uint8_t data);

/** Ensures any written byte before is really pushed to the underlying stream.*/
void a2jFlush(void);

#endif // A2J
#endif // A2J_LL_H
