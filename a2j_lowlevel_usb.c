/** \file
USB implementation of the Arduino2java lowlevel abstraction interface.*/

#include <stdint.h>
#include "util/delay.h"
#include "a2j_lowlevel.h"
#include "serial.h"
#include "a2j_lowlevel_usb.h"

#ifdef A2J_USB


uint8_t Endpoint_BytesInEndpointCntWait(uint8_t bytes, uint8_t centiSeconds);

/** Returns 1, if there are \a bytes bytes available in the buffer before \a centiSeconds centiseconds have passed, 0 otherwise.*/
uint8_t Endpoint_BytesInEndpointCntWait(uint8_t bytes, uint8_t centiSeconds){
	if(centiSeconds == 0)
		centiSeconds = 1;
	uint16_t cnt = centiSeconds * 10; // 10 * 1ms == 10ms = centisecond
	for(;cnt>0;cnt--){
		if(Endpoint_BytesInEndpoint() >= bytes)
			return 1;
		_delay_ms(1);
	}
	return 0;
}


inline void a2jLLInit_usb(void){
	USB_Init();
}

void EVENT_USB_Device_ConfigurationChanged(void){
	Endpoint_ConfigureEndpoint(A2J_USB_IN_EPNUM, EP_TYPE_BULK, ENDPOINT_DIR_IN, A2J_USB_IN_EPSIZE, ENDPOINT_BANK_SINGLE);
	Endpoint_ConfigureEndpoint(A2J_USB_OUT_EPNUM, EP_TYPE_BULK, ENDPOINT_DIR_OUT, A2J_USB_OUT_EPSIZE, ENDPOINT_BANK_SINGLE);
}

inline void a2jLLTask_usb(void){
	USB_USBTask();
}
inline uint8_t a2jLLReady_usb(void){
    return USB_DeviceState == DEVICE_STATE_Configured;
}

uint8_t a2jLLAvailable_usb(void){
	Endpoint_SelectEndpoint(A2J_USB_OUT_EPNUM);
	return Endpoint_BytesInEndpoint() != 0;
}

uint8_t a2jReadByte_usb(){
	Endpoint_SelectEndpoint(A2J_USB_OUT_EPNUM);
	uint8_t data = 0;
	if (Endpoint_BytesInEndpoint())
		data = Endpoint_Read_Byte();

	if (!(Endpoint_BytesInEndpoint()))
		Endpoint_ClearOUT();
	printf_P(PSTR("|%x|"), data);
	return data;
}

uint16_t a2jReadEscapedByte_usb(){
	Endpoint_SelectEndpoint(A2J_USB_OUT_EPNUM);
	if(!Endpoint_BytesInEndpointCntWait(1, A2J_TIMEOUT)){
		return -1;
	}

	// we need either one unescaped byte...
	uint8_t data = a2jReadByte_usb();
	//if(data == A2J_SOF)
		// TODO: return "malformed frame", but checksum will save us, hopefully.
		// problem: we don't know the sequence number here.
	if(data != A2J_ESC){
		return data;
	}

	// ... or an escape character + the escaped byte
	if(!Endpoint_BytesInEndpointCntWait(1, A2J_TIMEOUT)){
		return -1;
	}
	return a2jReadByte_usb()+1;
}

void a2jWriteByte_usb(uint8_t data){
	Endpoint_SelectEndpoint(A2J_USB_IN_EPNUM);
	if(!(Endpoint_IsReadWriteAllowed())){
		printf_P(PSTR("(Clr)"));
		Endpoint_ClearIN();

		//uint8_t err;
		//if((err = Endpoint_WaitUntilReady()) != ENDPOINT_READYWAIT_NoError)
			//return err;
			printf_P(PSTR("(w=%x)"),Endpoint_WaitUntilReady());
	}

	Endpoint_Write_Byte(data);
	printf_P(PSTR("|%x|"),data);
	return;
	//return ENDPOINT_READYWAIT_NoError;
}

void a2jWriteEscapedByte_usb(uint8_t data){
	if(data == A2J_SOF || data == A2J_ESC){
		a2jWriteByte_usb(A2J_ESC);
		a2jWriteByte_usb(data-1);
	}else
		a2jWriteByte_usb(data);
}

void a2jFlush_usb(void){
	Endpoint_SelectEndpoint(A2J_USB_IN_EPNUM);
	printf_P(PSTR("(w=%x)"),Endpoint_WaitUntilReady());
	Endpoint_ClearIN();
	printf_P(PSTR("(w=%x)"),Endpoint_WaitUntilReady());
	Endpoint_ClearIN();
}

USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
{
	.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

	.USBSpecification       = VERSION_BCD(01.10),
	//.Class                  = 0x00,
	//.SubClass               = 0x00,
	//.Protocol               = 0x00,
	.Class                  = 0xff,
	.SubClass               = 0x13,
	.Protocol               = 0x00,

	.Endpoint0Size          = A2J_USB_C_EPSIZE,

	.VendorID               = 0x6666,
	.ProductID              = 0xCAFE,
	.ReleaseNumber          = 0xBABE,

	.ManufacturerStrIndex   = 0x01,
	.ProductStrIndex        = 0x02,
	.SerialNumStrIndex      = USE_INTERNAL_SERIAL,
	//.SerialNumStrIndex      = NO_DESCRIPTOR,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

USB_Descriptor_String_t PROGMEM LanguageString =
{
	.Header                 = {.Size = USB_STRING_LEN(1), .Type = DTYPE_String},
	.UnicodeString          = L"Љ"
};

USB_Descriptor_String_t PROGMEM ManufacturerString =
{
	.Header                 = {.Size = USB_STRING_LEN(16), .Type = DTYPE_String},
	.UnicodeString          = L"ims.tuwien.ac.at"
};

USB_Descriptor_String_t PROGMEM ProductString =
{
	.Header                 = {.Size = USB_STRING_LEN(9), .Type = DTYPE_String},
	.UnicodeString          = L"USB Board"
};


USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
{
	.Config = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

		.TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
		.TotalInterfaces        = 1,

		.ConfigurationNumber    = 1,
		.ConfigurationStrIndex  = NO_DESCRIPTOR,

		.ConfigAttributes       = USB_CONFIG_ATTR_BUSPOWERED,

		.MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
	},

	.Interface = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

		.InterfaceNumber        = 0,
		.AlternateSetting       = 0,
		.TotalEndpoints         = 2,

		.Class                  = 0xff,
		.SubClass               = 0xde,
		.Protocol               = 0xad,

		.InterfaceStrIndex      = NO_DESCRIPTOR
	},

	.DataInEndpoint = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

		.EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_IN | A2J_USB_IN_EPNUM),
		.Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = A2J_USB_IN_EPSIZE,
		.PollingIntervalMS      = 0x00
	},

	.DataOutEndpoint = {
		.Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

		.EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_OUT | A2J_USB_OUT_EPNUM),
		.Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize           = A2J_USB_OUT_EPSIZE,
		.PollingIntervalMS      = 0x00
	}
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint8_t wIndex, const void** const DescriptorAddress){
    const uint8_t  DescriptorType   = (wValue >> 8);
    const uint8_t  DescriptorNumber = (wValue & 0xFF);

    PGM_VOID_P    Address = NULL;
    uint16_t Size    = NO_DESCRIPTOR;
    (void)wIndex;

    switch (DescriptorType) {
        case DTYPE_Device:
            Address = (PGM_VOID_P)&DeviceDescriptor;
            Size    = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_Configuration:
            Address = (PGM_VOID_P)&ConfigurationDescriptor;
            Size    = sizeof(USB_Descriptor_Configuration_t);
            break;
        case DTYPE_String:
            switch (DescriptorNumber) {
                case 0x00:
                    Address = (PGM_VOID_P)&LanguageString;
                    Size    = pgm_read_byte(&LanguageString.Header.Size);
                    break;
                case 0x01:
                    Address = (PGM_VOID_P)&ManufacturerString;
                    Size    = pgm_read_byte(&ManufacturerString.Header.Size);
                    break;
                case 0x02:
                    Address = (PGM_VOID_P)&ProductString;
                    Size    = pgm_read_byte(&ProductString.Header.Size);
                    break;
            }
            break;
    }

    *DescriptorAddress = Address;
    return Size;
}
#endif // A2J_USB