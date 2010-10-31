#ifdef A2J
#include "a2j_lowlevel.h"

	#if defined(A2J_SERIAL) && defined(A2J_USB)
		#error "multiple a2j low level functions enabled. please define either A2J_SERIAL or A2J_USB"
	#endif
	#ifdef A2J_SERIAL
		#include "a2j_lowlevel_serial.h"
		A2J_LL_FUNC_DEFS(serial)
	#elif A2J_USB
		#include "a2j_lowlevel_usb.h"
		A2J_LL_FUNC_DEFS(usb)
	#else
		#error "no a2j low level functions selected. please define A2J_SERIAL or A2J_USB"
	#endif
#endif
