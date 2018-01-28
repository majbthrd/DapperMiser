/*
    CMSIS-DAP implementation for PIC16F1454/PIC16F1455/PIC16F1459 microcontroller

    Copyright (C) 2013-2018 Peter Lawrence.

    based on top of M-Stack USB driver stack by Alan Ott, Signal 11 Software

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "usb_config.h"
#include "usb.h"
#include "usb_ch9.h"
#include "usb_hid.h"
#include "usb_cdc.h"

#ifdef __C18
#define ROMPTR rom
#else
#define ROMPTR
#endif

struct configuration_1_packet {
	struct configuration_descriptor  config;

	/* HID */
	struct interface_descriptor      interface;
	struct hid_descriptor            hid;
	struct endpoint_descriptor       ep;
	struct endpoint_descriptor       ep1_out;
};

/* Device Descriptor */
const ROMPTR struct device_descriptor this_device_descriptor =
{
	sizeof(struct device_descriptor), // bLength
	DESC_DEVICE, // bDescriptorType
	0x0200, // 0x0200 = USB 2.0, 0x0110 = USB 1.1
	0x00, /* Device class */
	0x00, /* Device Subclass. */
	0x00, /* Protocol. */
	EP_0_LEN, // bMaxPacketSize0
	0x1209, // Vendor
	0x0001, // Product
	0x0003, // device release
	0, // Manufacturer
	1, // Product
	2, // Serial
	NUMBER_OF_CONFIGURATIONS // NumConfigurations
};

/* HID Report descriptor */
static const ROMPTR uint8_t custom_report_descriptor[] =
{
 0x06, 0x00, 0xFF,   // Usage Page = 0xFF00 (Vendor Defined Page 1)
 0x09, 0x01,         // Usage (Vendor Usage 1)
 0xA1, 0x01,         // Collection (Application)
 0x15, 0x00,         // Logical Minimum
 0x26, 0xFF, 0x00,   // Logical Maximum
 0x75, 0x08,         // Report Size: 8-bit
 0x95, EP_1_IN_LEN,  // Report Count
 0x09, 0x01,         // Usage (Vendor Usage 1)
 0x81, 0x02,         // Input: variable
 0x95, EP_1_OUT_LEN, // Report Count
 0x09, 0x01,         // Usage (Vendor Usage 1)
 0x91, 0x02,         // Output: variable
 0x95, 0x01,         // Report Count
 0x09, 0x01,         // Usage (Vendor Usage 1)
 0xB1, 0x02,         // Feature: Variable
 0xC0,               // End Collection
};

/* Configuration 1 Descriptor */
static const ROMPTR struct configuration_1_packet configuration_1 =
{
	{
	// Members from struct configuration_descriptor
	sizeof(struct configuration_descriptor),
	DESC_CONFIGURATION,
	sizeof(configuration_1), // wTotalLength (length of the whole packet)
	1, // bNumInterfaces (1 for CMSIS-DAP)
	1, // bConfigurationValue
	0, // iConfiguration (index of string descriptor)
	0b10000000,
	100/2,   // 100/2 indicates 100mA
	},

	{
	// Members from struct interface_descriptor
	sizeof(struct interface_descriptor), // bLength;
	DESC_INTERFACE,
	0, // InterfaceNumber
	0x0, // AlternateSetting
	0x2, // bNumEndpoints (num besides endpoint 0)
	HID_INTERFACE_CLASS, // bInterfaceClass 3=HID, 0xFF=VendorDefined
	0x00, // bInterfaceSubclass (0=NoBootInterface for HID)
	0x00, // bInterfaceProtocol (HID Custom Protocol)
	0x00, // iInterface (index of string describing interface)
	},

	{
	// Members from struct hid_descriptor
	sizeof(struct hid_descriptor),
	DESC_HID,
	0x0101, // bcdHID
	0x0, // bCountryCode
	1,   // bNumDescriptors
	DESC_REPORT, // bDescriptorType2
	sizeof(custom_report_descriptor), // wDescriptorLength
	},

	{
	// Members of the Endpoint Descriptor (EP1 IN)
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x01 | 0x80, // endpoint #1 0x80=IN
	EP_INTERRUPT, // bmAttributes
	EP_1_IN_LEN, // wMaxPacketSize
	1, // bInterval in ms.
	},


	{
	// Members of the Endpoint Descriptor (EP1 OUT)
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x01 /*| 0x00*/, // endpoint #1 0x00=OUT
	EP_INTERRUPT, // bmAttributes
	EP_1_OUT_LEN, // wMaxPacketSize
	1, // bInterval in ms.
	},
};

/* String Descriptors */

/* String index 0, only has one character in it, which is to be set to the
   language ID of the language which the other strings are in. */
static const ROMPTR struct {uint8_t bLength;uint8_t bDescriptorType; uint16_t lang; } str00 = {
	sizeof(str00),
	DESC_STRING,
	0x0409 // US English
};

static const ROMPTR struct {uint8_t bLength;uint8_t bDescriptorType; uint16_t chars[9]; } product_string = {
	sizeof(product_string),
	DESC_STRING,
	{'C','M','S','I','S','-','D','A','P'}
};

/* Get String function
 *
 * This function is called by the USB stack to get a pointer to a string
 * descriptor.  If using strings, USB_STRING_DESCRIPTOR_FUNC must be defined
 * to the name of this function in usb_config.h.  See
 * USB_STRING_DESCRIPTOR_FUNC in usb.h for information about this function.
 * This is a function, and not simply a list or map, because it is useful,
 * and advisable, to have a serial number string which may be read from
 * EEPROM or somewhere that's not part of static program memory.
 */
int16_t usb_application_get_string(uint8_t string_number, const void **ptr)
{
	if (string_number == 0) {
		*ptr = &str00;
		return sizeof(str00);
	}
	else if (string_number == 1) {
		*ptr = &product_string;
		return sizeof(product_string);
	}
	else if (string_number == 2) {
		/* leverage built-in SN from 'USB DFU Bootloader for PIC16F1454/5/9' */
		*ptr = (void *)0x81EE;
		return 18;
	}

	return -1;
}

/* Configuration Descriptor List */
const struct configuration_descriptor *usb_application_config_descs[] =
{
	(struct configuration_descriptor*) &configuration_1,
};
STATIC_SIZE_CHECK_EQUAL(USB_ARRAYLEN(USB_CONFIG_DESCRIPTOR_MAP), NUMBER_OF_CONFIGURATIONS);
STATIC_SIZE_CHECK_EQUAL(sizeof(USB_DEVICE_DESCRIPTOR), 18);

/* HID Descriptor Function */
int16_t usb_application_get_hid_descriptor(uint8_t interface, const void **ptr)
{
	/* Only one interface in this demo. The two-step assignment avoids an
	 * incorrect error in XC8 on PIC16. */
	const void *p = &configuration_1.hid;
	*ptr = p;
	return sizeof(configuration_1.hid);
}

/** HID Report Descriptor Function */
int16_t usb_application_get_hid_report_descriptor(uint8_t interface, const void **ptr)
{
	*ptr = custom_report_descriptor;
	return sizeof(custom_report_descriptor);
}
