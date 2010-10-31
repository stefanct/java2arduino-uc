/** \file
Arduino2java USB lowlevel abstraction header.*/

#ifndef A2J_LL_USB
	#define A2J_LL_USB

	#ifdef A2J
		#ifdef A2J_USB

		#include "a2j_lowlevel.h"
		A2J_LL_FUNC_DECS(usb)

		#include <LUFA/Drivers/USB/USB.h>

		#define A2J_USB_C_EPSIZE    FIXED_CONTROL_ENDPOINT_SIZE
		#define A2J_USB_IN_EPSIZE	64
		#define A2J_USB_OUT_EPSIZE	64
		#define A2J_USB_IN_EPNUM	1
		#define A2J_USB_OUT_EPNUM	2

		typedef struct {
			USB_Descriptor_Configuration_Header_t Config;
			USB_Descriptor_Interface_t            Interface;
			USB_Descriptor_Endpoint_t             DataInEndpoint;
			USB_Descriptor_Endpoint_t             DataOutEndpoint;
		} USB_Descriptor_Configuration_t;

		#endif // A2J_USB
	#endif // A2J
#endif