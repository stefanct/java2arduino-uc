/** \file
Arduino2java gerneric lowlevel abstraction interface.*/

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

/** Timeout for reads from the stream (in centiseconds). */
#define A2J_TIMEOUT 50

/** @addtogroup j2aframing java2arduino framing characters
\see \ref framing */
//@{
#define A2J_SOF 0x12 /**< Start of frame.*/
#define A2J_ESC 0x7D /**< Escape character.*/
//@}

/** @addtogroup j2acrc java2arduino crc constants
\see \ref crc */
//@{
#define A2J_CRC_CMD 11 /**< Constant to be added to the command offset byte. */
#define A2J_CRC_LEN 97 /**< Constant to be added to the length byte. */
//@}

uint8_t a2jReady(void);
uint8_t a2jAvailable(void);
uint8_t a2jReadByte(void);

/** Reads one byte from the stream.

Tries to read a byte from the stream with a timeout of #A2J_TIMEOUT centiseconds.
If that byte indicates escaping, another byte is read and returned, after it has been incremented.
@return The next byte after de-escaping
@see arduino2framing*/
uint16_t a2jReadEscapedByte(void);

void a2jWriteByte(uint8_t data);

/** Writes a byte to the underlying stream.
If the given argument has to be escaped, it writes #A2J_ESC first and then \a data-1.
@param data the byte to write
@see arduino2framing*/
void a2jWriteEscapedByte(uint8_t data);
void a2jFlush(void);

#endif // A2J
#endif // A2J_LL_H
