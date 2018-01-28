/*
 * Copyright (c) 2016, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*- Includes ----------------------------------------------------------------*/
#include "usb.h"
#include "usb_descriptors.h"
#include "dm_bsp.h"

/*- Variables ---------------------------------------------------------------*/
const usb_device_descriptor_t usb_device_descriptor __attribute__ ((aligned (4))) =
{
  .bLength            = sizeof(usb_device_descriptor_t),
  .bDescriptorType    = USB_DEVICE_DESCRIPTOR,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = DAP_PACKET_SIZE,
  .idVendor           = 0x1209,
  .idProduct          = 0x0001,
  .bcdDevice          = 0x0100,
  .iManufacturer      = USB_STR_ZERO,
  .iProduct           = USB_STR_PRODUCT,
  .iSerialNumber      = USB_STR_SERIAL_NUMBER,
  .bNumConfigurations = 1,
};

const usb_configuration_hierarchy_t usb_configuration_hierarchy __attribute__ ((aligned (4))) =
{
  .configuration =
  {
    .bLength             = sizeof(usb_configuration_descriptor_t),
    .bDescriptorType     = USB_CONFIGURATION_DESCRIPTOR,
    .wTotalLength        = sizeof(usb_configuration_hierarchy_t),
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = USB_STR_ZERO,
    .bmAttributes        = 0x80,
    .bMaxPower           = 200, // 400 mA
  },

  .interface =
  {
    .bLength             = sizeof(usb_interface_descriptor_t),
    .bDescriptorType     = USB_INTERFACE_DESCRIPTOR,
    .bInterfaceNumber    = 0,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 2,
    .bInterfaceClass     = 0x03,
    .bInterfaceSubClass  = 0x00,
    .bInterfaceProtocol  = 0x00,
    .iInterface          = USB_STR_ZERO,
  },

  .hid =
  {
    .bLength             = sizeof(usb_hid_descriptor_t),
    .bDescriptorType     = USB_HID_DESCRIPTOR,
    .bcdHID              = 0x0111,
    .bCountryCode        = 0,
    .bNumDescriptors     = 1,
    .bDescriptorType1    = USB_HID_REPORT_DESCRIPTOR,
    .wDescriptorLength   = sizeof(usb_hid_report_descriptor),
  },

  .ep_in =
  {
    .bLength             = sizeof(usb_endpoint_descriptor_t),
    .bDescriptorType     = USB_ENDPOINT_DESCRIPTOR,
    .bEndpointAddress    = USB_IN_ENDPOINT | 1,
    .bmAttributes        = USB_INTERRUPT_ENDPOINT,
    .wMaxPacketSize      = DAP_PACKET_SIZE,
    .bInterval           = 1,
  },

  .ep_out =
  {
    .bLength             = sizeof(usb_endpoint_descriptor_t),
    .bDescriptorType     = USB_ENDPOINT_DESCRIPTOR,
    .bEndpointAddress    = USB_OUT_ENDPOINT | 2,
    .bmAttributes        = USB_INTERRUPT_ENDPOINT,
    .wMaxPacketSize      = DAP_PACKET_SIZE,
    .bInterval           = 1,
  },
};

const uint8_t usb_hid_report_descriptor[33] __attribute__ ((aligned (4))) =
{
  0x06, 0x00, 0xFF,      // Usage Page = 0xFF00 (Vendor Defined Page 1)
  0x09, 0x01,            // Usage (Vendor Usage 1)
  0xA1, 0x01,            // Collection (Application)
  0x15, 0x00,            // Logical Minimum
  0x26, 0xFF, 0x00,      // Logical Maximum
  0x75, 0x08,            // Report Size: 8-bit
  0x95, DAP_PACKET_SIZE, // Report Count
  0x09, 0x01,            // Usage (Vendor Usage 1)
  0x81, 0x02,            // Input: variable
  0x95, DAP_PACKET_SIZE, // Report Count
  0x09, 0x01,            // Usage (Vendor Usage 1)
  0x91, 0x02,            // Output: variable
  0x95, 0x01,            // Report Count
  0x09, 0x01,            // Usage (Vendor Usage 1)
  0xB1, 0x02,            // Feature: Variable
  0xC0,                  // End Collection
};

const usb_string_descriptor_zero_t usb_string_descriptor_zero __attribute__ ((aligned (4))) =
{
  .bLength               = sizeof(usb_string_descriptor_zero_t),
  .bDescriptorType       = USB_STRING_DESCRIPTOR,
  .wLANGID               = 0x0409, // English (United States)
};

char usb_serial_number[16];

const char *const usb_strings[] = 
{
  [USB_STR_PRODUCT]       = "CMSIS-DAP",
  [USB_STR_SERIAL_NUMBER] = usb_serial_number,
};

uint8_t usb_string_descriptor_buffer[64] __attribute__ ((aligned (4)));
