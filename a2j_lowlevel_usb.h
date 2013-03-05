/** \file
Arduino2java USB lowlevel abstraction header.*/

#ifndef A2J_LL_USB
	#define A2J_LL_USB

	#ifdef A2J
		#ifdef A2J_USB

			#include <LUFA/Drivers/USB/USB.h>
			#include "a2j_lowlevel.h"

			#define A2J_USB_IN_ADDR		(ENDPOINT_DIR_IN | 1)
			#define A2J_USB_OUT_ADDR	(ENDPOINT_DIR_OUT | 2)
			#define A2J_USB_C_EPSIZE	FIXED_CONTROL_ENDPOINT_SIZE

			#define A2J_USB_VENDORID	0x6666
			#define A2J_USB_PRODUCTID	0xCAFE
			#define A2J_USB_RELEASENUMBER	0xBABE

			#define A2J_USB_INTERFACE_CLASS		0xff
			#define A2J_USB_INTERFACE_SUBCLASS	0x12
			#define A2J_USB_INTERFACE_PROTOCOL	0xef

			#ifndef A2J_LL_OPTS
				#warning "Using default settings for arduino2java USB. To use your own, define them and call gcc with -D A2J_LL_OPTS."

				#define A2J_USB_IN_EPSIZE	64
				#define A2J_USB_OUT_EPSIZE	64

				#define A2J_USB_MANUFACTURERSTRING	L"manufacturer"
				#define A2J_USB_MANUFACTURERSTRING_LEN	12
				#define A2J_USB_PRODUCTSTRING	L"product"
				#define A2J_USB_PRODUCTSTRING_LEN	7
				#define A2J_USB_SERIAL	L"123456"
				#define A2J_USB_SERIAL_LEN 6

			#else
				#include "a2j_opts.h"
			#endif // A2J_LL_OPTS

			#if !defined (A2J_USB_IN_EPSIZE)
				#error "Missing A2J_USB option! Need:  A2J_USB_IN_EPSIZE"
			#endif

			#if !defined (A2J_USB_OUT_EPSIZE)
				#error "Missing A2J_USB option! Need:  A2J_USB_OUT_EPSIZE"
			#endif

			#if !defined (A2J_USB_MANUFACTURERSTRING)
				#error "Missing A2J_USB option! Need:  A2J_USB_MANUFACTURERSTRING"
			#endif

			#if !defined (A2J_USB_MANUFACTURERSTRING_LEN)
				#error "Missing A2J_USB option! Need:  A2J_USB_MANUFACTURERSTRING_LEN"
			#endif

			#if !defined (A2J_USB_PRODUCTSTRING)
				#error "Missing A2J_USB option! Need:  A2J_USB_PRODUCTSTRING"
			#endif

			#if !defined (A2J_USB_PRODUCTSTRING_LEN)
				#error "Missing A2J_USB option! Need:  A2J_USB_PRODUCTSTRING_LEN"
			#endif

			#if !defined (A2J_USB_SERIAL)
				#warn "A2J_USB_SERIAL was not set. The internal serial will be used if available, else the host may generate one."
			#else
				#if !defined (A2J_USB_SERIAL_LEN)
					#error "Missing A2J_USB option! Need:  A2J_USB_SERIAL_LEN if A2J_USB_SERIAL is defined"
				#endif
			#endif


			typedef struct {
				USB_Descriptor_Configuration_Header_t Config;
				USB_Descriptor_Interface_t            Interface;
				USB_Descriptor_Endpoint_t             DataInEndpoint;
				USB_Descriptor_Endpoint_t             DataOutEndpoint;
			} USB_Descriptor_Configuration_t;

		#endif // A2J_USB
	#endif // A2J
#endif // A2J_LL_USB