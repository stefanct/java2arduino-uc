/** \file
USB implementation of the Arduino2java lowlevel abstraction interface.*/

#include <util/delay.h>
#include <avr/pgmspace.h>
#include "a2j_lowlevel_usb.h"

#ifdef A2J_USB

inline void a2jInit(void){
	USB_Init();
}

void EVENT_USB_Device_ConfigurationChanged(void){
	Endpoint_ConfigureEndpoint(A2J_USB_IN_EPNUM, EP_TYPE_BULK, ENDPOINT_DIR_IN, A2J_USB_IN_EPSIZE, ENDPOINT_BANK_SINGLE);
	Endpoint_ConfigureEndpoint(A2J_USB_OUT_EPNUM, EP_TYPE_BULK, ENDPOINT_DIR_OUT, A2J_USB_OUT_EPSIZE, ENDPOINT_BANK_SINGLE);
}

inline void a2jTask(void){
	USB_USBTask();
}

inline uint8_t a2jReady(void){
	return USB_DeviceState == DEVICE_STATE_Configured;
}

uint8_t a2jAvailable(void){
	Endpoint_SelectEndpoint(A2J_USB_OUT_EPNUM);
	return Endpoint_BytesInEndpoint() != 0;
}

uint16_t a2jReadByte(){
	Endpoint_SelectEndpoint(A2J_USB_OUT_EPNUM);
	uint8_t cnt = A2J_TIMEOUT;
	for(; cnt>0; cnt--){
		if (Endpoint_BytesInEndpoint()){
			uint8_t data = Endpoint_Read_Byte();
			if (!(Endpoint_BytesInEndpoint()))
				Endpoint_ClearOUT();
			return data;
		}
		_delay_ms(1);
	}
	return -1;
}

void a2jWriteByte(uint8_t data){
	Endpoint_SelectEndpoint(A2J_USB_IN_EPNUM);
	if(!(Endpoint_IsReadWriteAllowed())){
		Endpoint_ClearIN();
		uint8_t err;
		if((err = Endpoint_WaitUntilReady()) != ENDPOINT_READYWAIT_NoError){
			return;
		}
	}

	Endpoint_Write_Byte(data);
	return;
}

void a2jFlush(void){
	Endpoint_SelectEndpoint(A2J_USB_IN_EPNUM);
	Endpoint_WaitUntilReady();
	Endpoint_ClearIN();
	Endpoint_WaitUntilReady();
	Endpoint_ClearIN();
}

USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
{
	.Header				 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

	.USBSpecification		= VERSION_BCD(01.10),
	.Class					= 0xff,
	.SubClass				= 0x13,
	.Protocol				= 0x00,

	.Endpoint0Size			= A2J_USB_C_EPSIZE,

	.VendorID				= A2J_USB_VENDORID,
	.ProductID				= A2J_USB_PRODUCTID,
	.ReleaseNumber			= A2J_USB_RELEASENUMBER,

	.ManufacturerStrIndex	= 0x01,
	.ProductStrIndex		= 0x02,
#ifdef A2J_USB_SERIAL
	.SerialNumStrIndex		= 0x03,
#else
	.SerialNumStrIndex		= USE_INTERNAL_SERIAL,
#endif

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

USB_Descriptor_String_t PROGMEM LanguageString =
{
	.Header				= {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},
	.UnicodeString		= L"Љ"
};

USB_Descriptor_String_t PROGMEM ManufacturerString =
{
	.Header				= {.Size = USB_STRING_LEN(A2J_USB_MANUFACTURERSTRING_LEN), .Type = DTYPE_String},
	.UnicodeString		= A2J_USB_MANUFACTURERSTRING
};

USB_Descriptor_String_t PROGMEM ProductString =
{
	.Header				= {.Size = USB_STRING_LEN(A2J_USB_PRODUCTSTRING_LEN), .Type = DTYPE_String},
	.UnicodeString		= A2J_USB_PRODUCTSTRING
};

#ifdef A2J_USB_SERIAL
USB_Descriptor_String_t PROGMEM SerialString =
{
	.Header				= {.Size = USB_STRING_LEN(A2J_USB_SERIAL_LEN), .Type = DTYPE_String},
	.UnicodeString		= A2J_USB_SERIAL
};
#endif


USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
	.Config = {
		.Header					= {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

		.TotalConfigurationSize	= sizeof(USB_Descriptor_Configuration_t),
		.TotalInterfaces		= 1,

		.ConfigurationNumber	= 1,
		.ConfigurationStrIndex	= NO_DESCRIPTOR,

		.ConfigAttributes		= USB_CONFIG_ATTR_BUSPOWERED,

		.MaxPowerConsumption	= USB_CONFIG_POWER_MA(100)
	},

	.Interface = {
		.Header				 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

		.InterfaceNumber		= 0,
		.AlternateSetting		= 0,
		.TotalEndpoints			= 2,

		.Class					= A2J_USB_INTERFACE_CLASS,
		.SubClass				= A2J_USB_INTERFACE_SUBCLASS,
		.Protocol				= A2J_USB_INTERFACE_PROTOCOL,

		.InterfaceStrIndex		= NO_DESCRIPTOR
	},

	.DataInEndpoint = {
		.Header					= {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

		.EndpointAddress		= (ENDPOINT_DESCRIPTOR_DIR_IN | A2J_USB_IN_EPNUM),
		.Attributes				= (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize			= A2J_USB_IN_EPSIZE,
		.PollingIntervalMS		= 0x00
	},

	.DataOutEndpoint = {
		.Header					= {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

		.EndpointAddress		= (ENDPOINT_DESCRIPTOR_DIR_OUT | A2J_USB_OUT_EPNUM),
		.Attributes				= (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize			= A2J_USB_OUT_EPSIZE,
		.PollingIntervalMS		= 0x00
	}
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex, const void** const DescriptorAddress){
	const uint8_t DescriptorType = (wValue >> 8);
	const uint8_t DescriptorNumber = (wValue & 0xFF);

	PGM_VOID_P Address = NULL;
	uint16_t Size = NO_DESCRIPTOR;
	(void)wIndex;

	switch (DescriptorType) {
		case DTYPE_Device:
			Address = (PGM_VOID_P)&DeviceDescriptor;
			Size = sizeof(USB_Descriptor_Device_t);
			break;
		case DTYPE_Configuration:
			Address = (PGM_VOID_P)&ConfigurationDescriptor;
			Size = sizeof(USB_Descriptor_Configuration_t);
			break;
		case DTYPE_String:
			switch (DescriptorNumber) {
				case 0x00:
					Address = (PGM_VOID_P)&LanguageString;
					Size = pgm_read_byte(&LanguageString.Header.Size);
					break;
				case 0x01:
					Address = (PGM_VOID_P)&ManufacturerString;
					Size = pgm_read_byte(&ManufacturerString.Header.Size);
					break;
				case 0x02:
					Address = (PGM_VOID_P)&ProductString;
					Size = pgm_read_byte(&ProductString.Header.Size);
					break;
#ifdef A2J_USB_SERIAL
				case 0x03:
					Address = (PGM_VOID_P)&SerialString;
					Size = pgm_read_byte(&SerialString.Header.Size);
					break;
#endif
			}
			break;
	}

	*DescriptorAddress = Address;
	return Size;
}
#endif // A2J_USB