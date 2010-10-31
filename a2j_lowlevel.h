/** \file
Arduino2java gerneric lowlevel abstraction interface.*/

#ifndef A2J_LL_H
#define A2J_LL_H

#include <stdint.h>

/** Timeout for reads from the stream (in centiseconds). */
#define A2J_TIMEOUT 5

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

void a2jLLInit(void);
void a2jLLTask(void);
uint8_t a2jLLReady(void);
uint8_t a2jLLAvailable(void);
uint8_t a2jReadByte(void);

/** Reads one byte from the stream.

Tries to read a byte from the stream with a timeout of #A2J_TIMEOUT centiseconds.
If that byte indicates escaping, another byte is read and returned, after it has been incremented.
@return The next byte after de-escaping
@see arduino2framing*/
uint16_t a2jReadEscapedByte(void);

void a2jWriteByte(uint8_t data);

/** Writes a byte to the serial line.
If the given argument has to be escaped, it writes #A2J_ESC first and then \a data-1.
@param data the byte to write
@see arduino2framing*/
void a2jWriteEscapedByte(uint8_t data);
void a2jFlush(void);

#define A2J_LL_FUNC_DECS(SUFFIX) \
void a2jLLInit_##SUFFIX(void);\
void a2jLLTask_##SUFFIX(void);\
uint8_t a2jLLReady_##SUFFIX(void);\
uint8_t a2jLLAvailable_##SUFFIX(void);\
uint8_t a2jReadByte_##SUFFIX(void);\
uint16_t a2jReadEscapedByte_##SUFFIX(void);\
void a2jWriteByte_##SUFFIX(uint8_t data);\
void a2jWriteEscapedByte_##SUFFIX(uint8_t data);\
void a2jFlush_##SUFFIX(void);


#define A2J_LL_FUNC_DEFS(SUFFIX) \
inline void a2jLLInit(void){\
	a2jLLInit_##SUFFIX();\
}\
inline void a2jLLTask(void){\
	a2jLLTask_##SUFFIX();\
}\
inline uint8_t a2jLLReady(void){\
	return a2jLLReady_##SUFFIX();\
}\
inline uint8_t a2jLLAvailable(void){\
	return a2jLLAvailable_##SUFFIX();\
}\
inline uint8_t a2jReadByte(void){\
	return a2jReadByte_##SUFFIX();\
}\
inline uint16_t a2jReadEscapedByte(void){\
	return a2jReadEscapedByte_##SUFFIX();\
}\
inline void a2jWriteByte(uint8_t data){\
	a2jWriteByte_##SUFFIX(data);\
}\
inline void a2jWriteEscapedByte(uint8_t data){\
	a2jWriteEscapedByte_##SUFFIX(data);\
}\
inline void a2jFlush(void){\
	a2jFlush_##SUFFIX();\
}
#endif