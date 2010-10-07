/** \file
Arduino2j header.*/

#ifndef A2J_H
#define A2J_H

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

/** Timeout for serial reads (in centiseconds). */
#define A2J_TIMEOUT 5

#define A2J_MAX_PAYLOAD 255 /**< Maximum number of bytes to be transmitted as payload in arduino2j packets.*/
/** @addtogroup j2amany*/
//@{
#define A2J_MANY_HEADER 6 /**< Number of bytes of the a2jMany header. Consisting of the function offset, isLast and 4B offset.*/
#define A2J_MANY_PAYLOAD (A2J_MAX_PAYLOAD - A2J_MANY_HEADER) /**< Maximum number of bytes to be transmitted as payload in a2jMany packets.*/
//@}

/** @addtogroup j2acrc java2arduino crc constants
\see \ref crc */
//@{
#define A2J_CRC_CMD 11 /**< Constant to be added to the command offset byte. */
#define A2J_CRC_LEN 97 /**< Constant to be added to the length byte. */
//@}

/** @addtogroup j2aerrors java2arduino error values */
//@{
#define A2J_RET_OOB 0xF0 /**< Out of bounds of the arduino2j jump table.*/
#define A2J_RET_TO 0xF2 /**< Timeout while waiting for a byte.*/
#define A2J_RET_CHKSUM 0xF3 /**< Checksum error.*/
//@}

/** @addtogroup j2aframing java2arduino framing characters
\see \ref framing */
//@{
#define A2J_SOF 0x12 /**< Start of frame.*/
#define A2J_ESC 0x7D /**< Escape character.*/
//@}

/** @name arduino2j properties macros
Arduino2j properties are a mapping between strings readable by the host computer.
E.g. this can be used to propagate the presence or property of devices attached to the Arduino.
Arduino2j properties are saved to flash as sequence of c-string pairs (key and corresponding value).
To create the mapping one has to call the needed macros in succession.*/
//@{
/** Header for the properties. Needs to be called first.*/
#define STARTPROPS const char PROGMEM a2j_props[] = {
/** helper macro */
#define EXPSTRFY(x) #x 
/** Adds a mapping from \a key to \a value.
The second parameter will be expanded once to enable the use of macros as parameters, e.g.:
\code
#define MACRO 13
ADDPROP(MACRO, MACRO)
\endcode will create the mapping "MACRO"->"13".*/
#define ADDPROP(key,value) #key "\0" EXPSTRFY(value) "\0"
/** Finalizes the properties. */
#define ENDPROPS }; const uint8_t a2j_props_size = sizeof(a2j_props);
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
	#define FUNCMAP(funcName, alias) static char PROGMEM funcName##_map[] = #alias;
	/** Appends an entry to the jumptable */
	#define ADDJT(funcName) , {&funcName, funcName##_map}
	/** Appends an entry to the jumptable, to be used for a2jMany functions. @see CMD_P_MANY */
	#define ADDLJT(funcName) , {(CMD_P)&funcName, funcName##_map}
	/** finalizes the jumptable/function mapping */
	#define ENDJT }; const uint8_t a2j_jt_elems = sizeof(a2j_jt)/sizeof(jt_entry);

	#ifdef DBG
		/** start of the jumptable including entries for various (default) arduino2j functions.*/
		#define STARTJT \
		FUNCMAP(a2jGetMapping, a2jGetMapping) \
		FUNCMAP(a2jMany, a2jMany) \
		FUNCMAP(a2jGetProperties, a2jGetProperties) \
		FUNCMAP(a2jDebug, a2jDebug) \
		FUNCMAP(a2jReset, a2jReset) \
		FUNCMAP(a2jEcho, a2jEcho) \
		const jt_entry PROGMEM a2j_jt[] = { \
		{&a2jGetMapping, a2jGetMapping##_map} \
		ADDJT(a2jMany) \
		ADDJT(a2jGetProperties) \
		ADDJT(a2jDebug) \
		ADDJT(a2jEcho)
		
	#else // DBG

		#define STARTJT \
		FUNCMAP(a2jGetMapping, a2jGetMapping) \
		FUNCMAP(a2jGetProperties, a2jGetProperties) \
		FUNCMAP(a2jReset, a2jReset) \
		FUNCMAP(a2jEcho, a2jEcho) \
		const jt_entry PROGMEM a2j_jt[] = { \
		{&a2jGetMapping, a2jGetMapping##_map} \
		ADDJT(a2jGetProperties) \
		ADDJT(a2jEcho)
	#endif // DBG
//@}

#else // A2J_FMAP

	typedef CMD_P jt_entry;

	#define FUNCMAP(ignored, ignored2) ;
	#define ADDJT(funcName) , &funcName
	#define ADDLJT(funcName) , (CMD_P)&funcName
	#define ENDJT }; const uint8_t a2j_jt_elems = sizeof(a2j_jt)/sizeof(jt_entry);

	#ifdef DBG
		#define STARTJT const CMD_P PROGMEM a2j_jt[] = { \
		&a2jGetProperties \
		ADDJT(a2jDebug) \
		ADDJT(a2jEcho)
	#else // DBG
		#define STARTJT const CMD_P PROGMEM a2j_jt[] = { \
		&a2jGetProperties \
		ADDJT(a2jEcho)
	#endif // DBG

#endif // A2J_FMAP

void a2jProcess(void);
void a2jInit(void);

/**	@name default functions */
//@{
#ifdef A2J_FMAP
uint8_t a2jGetMapping(uint8_t *const lenp, uint8_t* *const datap);
#endif

#ifdef DBG
uint8_t a2jDebug(uint8_t *const lenp, uint8_t* *const datap);
#endif

uint8_t a2jMany(uint8_t *const lenp, uint8_t* *const datap);
uint8_t a2jGetProperties(uint8_t *const lenp, uint8_t* *const datap);
uint8_t a2jEcho(uint8_t *const,  uint8_t * *const);
//@}

#endif // A2J_H
