/** \file
Arduino2java USB lowlevel abstraction header.*/

#ifndef A2J_LL_USB
	#define A2J_LL_USB

	#ifdef A2J
		#ifdef A2J_USB

			#include <LUFA/Drivers/USB/USB.h>
			#include "j2a_const.h"
			#include "a2j_lowlevel.h"


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

			#if defined (A2J_USB_SERIAL) && !defined (A2J_USB_SERIAL_LEN)
				#error "Missing A2J_USB option! Need:  A2J_USB_SERIAL_LEN if A2J_USB_SERIAL is defined"
			#endif

			#if defined (A2J_USB_CUSTOM_IF)
				#if !defined (A2J_USB_DESC_DECL) || !defined (A2J_USB_DESC_DEF) || !defined (A2J_USB_CONFIG) || !defined (A2J_USB_NUM_CUSTOM_IF)
					#error "Missing macros for the custom USB interface(s)! Need: A2J_USB_DESC_DECL, A2J_USB_DESC_DEF, A2J_USB_CONFIG, A2J_USB_NUM_CUSTOM_IF"
				#endif
			#else
				#define A2J_USB_DESC_DECL
				#define A2J_USB_DESC_DEF
				#define A2J_USB_CONFIG
				#define A2J_USB_NUM_CUSTOM_IF 0
			#endif

			typedef struct {
				USB_Descriptor_Configuration_Header_t Config;
				USB_Descriptor_Interface_t            A2J_Interface;
				USB_Descriptor_Endpoint_t             A2J_DataInEndpoint;
				USB_Descriptor_Endpoint_t             A2J_DataOutEndpoint;
				A2J_USB_DESC_DECL
			} USB_Descriptor_Configuration_t;

		#endif // A2J_USB
	#endif // A2J
#endif // A2J_LL_USB