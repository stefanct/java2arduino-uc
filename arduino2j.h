/** \file
Arduino2j header.*/

#ifndef A2J_H
#define A2J_H

#include <stdint.h>
#include <stddef.h>
#include <avr/pgmspace.h>

/** Function pointer for data transfers upto #A2J_MAX_PAYLOAD bytes.
On entry \a *datap points at a byte array of length \a *lenp,
which is filled with the payload of the host computer.
Upto #A2J_MAX_PAYLOAD bytes following \a *datap may be written by the callee.
After return the returned \a uint8_t and \a *lenp bytes of the array at \a *datap will be sent back to the host.*/
typedef uint8_t (*const CMD_P)(uint8_t *const lenp, uint8_t* *const datap);

/**@ingroup j2amany
Function pointer for data transfers upto 2^32B (4GB).
On entry \a *datap points at a byte array of length \a *lenp,
which is filled with the payload of the host computer.
\a offsetp tells the callee the offset of the chunk in \a *datap inside the big block and
\a isLastp indicates if this is the last chunk.
Upto #A2J_MANY_PAYLOAD bytes following \a *datap may be written by the callee.
After return the returned \a uint8_t, the offset, \a *isLastp and \a *lenp bytes of the array at \a *datap will be sent back to the host.*/
typedef uint8_t (*const CMD_P_MANY)(uint8_t* isLastp, uint32_t *const offsetp, uint8_t *const lenp, uint8_t* *const datap);

void a2jProcess(void);

/** Initializes a2j and drivers it depends on.*/
void a2jInit(void);

/** Background task that maintains the low level connections.
Has to be called in a timely manner depending on the underlying protocol:
- USB: at least every 30ms when connected
- Serial: not at all (equals nop)*/
void a2jTask(void);

/**	@name default functions */
//@{
#ifdef A2J_FMAP
uint8_t a2jGetMapping(uint8_t *const lenp, uint8_t* *const datap);
#endif

#ifdef A2J_DBG
uint8_t a2jDebug(uint8_t *const lenp, uint8_t* *const datap);
#endif

uint8_t a2jMany(uint8_t *const lenp, uint8_t* *const datap);
uint8_t a2jGetProperties(uint8_t *const lenp, uint8_t* *const datap);
uint8_t a2jEcho(uint8_t *const,  uint8_t * *const);
uint8_t a2jEchoMany(uint8_t* isLastp, uint32_t *const offset, uint8_t *const lenp, uint8_t* *const datap);
//@}

#define A2J_MAX_PAYLOAD 255 /**< Maximum number of bytes to be transmitted as payload in arduino2j packets.*/
/** @addtogroup j2amany*/
//@{
#define A2J_MANY_HEADER 6 /**< Number of bytes of the a2jMany header. Consisting of the function offset, isLast and 4B offset.*/
#define A2J_MANY_PAYLOAD (A2J_MAX_PAYLOAD - A2J_MANY_HEADER) /**< Maximum number of bytes to be transmitted as payload in a2jMany packets.*/
//@}

/** @addtogroup j2aerrors java2arduino error values */
//@{
#define A2J_RET_OOB 0xF0 /**< Out of bounds of the arduino2j jump table.*/
#define A2J_RET_TO 0xF2 /**< Timeout while waiting for a byte.*/
#define A2J_RET_CHKSUM 0xF3 /**< Checksum error.*/
//@}

/**\name Native endianess to byte array macros
\anchor lilendianmacros */
//@{
/** Write a multibyte value of native endianess into a byte array. */
#define toArray(type, source, destArray, offset) { type* ntoh_temp_var = (type*)(&(destArray)[offset]);ntoh_temp_var[0] = (source); }
/** Read a multibyte value from a byte array. */
#define fromArray(type, source, offset) *(type*)(&(source)[offset])
// @}

#ifdef A2J
#ifdef A2J_PROPS
/** @name arduino2j properties macros
Arduino2j properties are a mapping between strings readable by the host computer.
E.g. this can be used to propagate the presence or property of devices attached to the Arduino.
Arduino2j properties are saved to flash as sequence of c-string pairs (key and corresponding value).
To create the mapping one has to call the needed macros in succession.*/
//@{
/** Header for the properties. Needs to be called first.*/
#define STARTPROPS unsigned char const PROGMEM a2j_props[] = {
/** Adds a mapping from \a key to \a value. */
#define ADDPROP(key,value)  #key "\0"  #value "\0"
#define STRFY(x) #x
/** Adds a mapping from \a key to the stringified version of \a value.
 *
 * The second parameter will be expanded once to enable the use of macros as parameters, e.g.:
\code
#define MACRO 13
ADDPROPEXP(MACRO, MACRO)
\endcode will create the mapping "MACRO"->"13".*/
#define ADDPROPEXP(key,value) #key "\0" STRFY(value) "\0"
/** Finalizes the properties. */
#define ENDPROPS }; const uint8_t a2j_props_size = sizeof(a2j_props) - 1; /* C puts an additional \0 at the end of a "string" */

#else // A2J_PROPS

#define STARTPROPS const unsigned char* PROGMEM a2j_props = NULL;\
const uint8_t a2j_props_size = 0;
#define ADDPROP(key,value)
#define ENDPROPS
#endif

//@}

#ifdef A2J_FMAP
	/** Struct type that stores a function pointer together with a string to enable function name mapping. */
	typedef struct{
		const CMD_P cmd; /**< Function pointer.*/
		const char* name; /**< Function name.*/
	}jt_entry;

/** @name arduino2j function mapping macros
\anchor jtmacros

These macros are used to add the function's name together with its offset to the jumptable. 
This enables the host computer to create a mapping between function names and offsets,
making function calls potentially independent from the concrete offsets.
The expanded output of #FUNCMAP has to be in scope of #ADDJT and #ADDLJT respectively.
#STARTJT, #ADDJT/#ADDLJT and #ENDJT have to be called in succession.*/
//@{
	/** Creates a string \a "alias" in flash accessible by variable \a FuncName_map */
	#define FUNCMAP(funcName, alias) static const char PROGMEM funcName##_map[] = #alias;
	/** Appends an entry to the jumptable */
	#define ADDJT(funcName) , {&funcName, funcName##_map}
	/** Appends an entry to the jumptable, to be used for a2jMany functions. @see CMD_P_MANY */
	#define ADDLJT(funcName) , {(CMD_P)&funcName, funcName##_map}
	/** finalizes the jumptable/function mapping */
	#define ENDJT }; const uint8_t a2j_jt_elems = sizeof(a2j_jt)/sizeof(jt_entry);

	#ifdef A2J_DBG
		/** start of the jumptable including entries for various (default) arduino2j functions.*/
		#define STARTJT \
		FUNCMAP(a2jGetMapping, a2jGetMapping) \
		FUNCMAP(a2jMany, a2jMany) \
		FUNCMAP(a2jGetProperties, a2jGetProperties) \
		FUNCMAP(a2jDebug, a2jDebug) \
		FUNCMAP(a2jEcho, a2jEcho) \
		FUNCMAP(a2jEchoMany, a2jEchoMany) \
		const jt_entry PROGMEM a2j_jt[] = { \
		{&a2jGetMapping, a2jGetMapping_map} \
		ADDJT(a2jMany) \
		ADDJT(a2jGetProperties) \
		ADDJT(a2jDebug) \
		ADDJT(a2jEcho)\
		ADDLJT(a2jEchoMany)
		
	#else // A2J_DBG

		#define STARTJT \
		FUNCMAP(a2jGetMapping, a2jGetMapping) \
		FUNCMAP(a2jMany, a2jMany) \
		FUNCMAP(a2jGetProperties, a2jGetProperties) \
		FUNCMAP(a2jEcho, a2jEcho) \
		FUNCMAP(a2jEchoMany, a2jEchoMany) \
		const jt_entry PROGMEM a2j_jt[] = { \
		{&a2jGetMapping, a2jGetMapping_map} \
		ADDJT(a2jMany) \
		ADDJT(a2jGetProperties) \
		ADDJT(a2jEcho)\
		ADDLJT(a2jEchoMany)
	#endif // A2J_DBG
//@}

#else // A2J_FMAP

	typedef CMD_P jt_entry;

	#define FUNCMAP(ignored, ignored2) ;
	#define ADDJT(funcName) , &funcName
	#define ADDLJT(funcName) , (CMD_P)&funcName
	#define ENDJT }; const uint8_t a2j_jt_elems = sizeof(a2j_jt)/sizeof(jt_entry);

	#ifdef A2J_DBG
		#define STARTJT const CMD_P PROGMEM a2j_jt[] = { \
			&a2jMany \
			ADDJT(a2jGetProperties) \
			ADDJT(a2jDebug) \
			ADDJT(a2jEcho) \
			ADDLJT(a2jEchoMany)
	#else // A2J_DBG
		#define STARTJT const CMD_P PROGMEM a2j_jt[] = { \
			&a2jMany \
			ADDJT(a2jGetProperties) \
			ADDJT(a2jEcho) \
			ADDLJT(a2jEchoMany)
	#endif // A2J_DBG
#endif // A2J_FMAP
#else // A2J
	void a2jProcess(void){};
	void a2jInit(void){};
	void a2jTask(void){};
#endif // A2J

#endif // A2J_H
