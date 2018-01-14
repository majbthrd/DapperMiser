#ifndef __VENDORHID_HELPER_H
#define __VENDORHID_HELPER_H

#include <stdint.h>
#include "usbhelper.h"

/* macro to help generate VendorHID USB descriptors */

#define VENDORHID_DESCRIPTOR(HID_INTF, DATAOUT_EP, DATAIN_EP, HID_REPORT_DESC_SIZE) \
    { \
      { \
        /*Interface Descriptor */ \
        sizeof(struct interface_descriptor),             /* bLength: Interface Descriptor size */ \
        USB_DESC_TYPE_INTERFACE,                         /* bDescriptorType: Interface */ \
        HID_INTF,                                        /* bInterfaceNumber: Number of Interface */ \
        0x00,                                            /* bAlternateSetting: Alternate setting */ \
        0x02,                                            /* bNumEndpoints */ \
        0x03,                                            /* bInterfaceClass: HID */ \
        0x00,                                            /* bInterfaceSubClass: 1=BOOT, 0=no boot */ \
        0x00,                                            /* bInterfaceProtocol: 0=none, 1=keyboard, 2=mouse */ \
        0x00,                                            /* iInterface (string index) */ \
      }, \
 \
      { \
        sizeof(struct hid_functional_descriptor),      /* bLength */ \
        HID_DESCRIPTOR_TYPE,                           /* bDescriptorType */ \
        USB_UINT16(0x0111),                            /* bcdHID */ \
        0x00,                                          /* bCountryCode */ \
        0x01,                                          /* bNumDescriptors */ \
        HID_REPORT_DESC,                               /* bDescriptorType */ \
        USB_UINT16(HID_REPORT_DESC_SIZE),              /* wItemLength */ \
      }, \
 \
      { \
        sizeof(struct endpoint_descriptor),            /* bLength: Endpoint Descriptor size */ \
        USB_DESC_TYPE_ENDPOINT,                        /* bDescriptorType: Endpoint */ \
        DATAIN_EP,                                     /* bEndpointAddress */ \
        0x03,                                          /* bmAttributes: Interrupt */ \
        USB_UINT16(HID_EP_SIZE),                       /* wMaxPacketSize */ \
        HID_POLLING_INTERVAL,                          /* bInterval */ \
      }, \
 \
      { \
        sizeof(struct endpoint_descriptor),            /* bLength: Endpoint Descriptor size */ \
        USB_DESC_TYPE_ENDPOINT,                        /* bDescriptorType: Endpoint */ \
        DATAOUT_EP,                                    /* bEndpointAddress */ \
        0x03,                                          /* bmAttributes: Interrupt */ \
        USB_UINT16(HID_EP_SIZE),                       /* wMaxPacketSize */ \
        HID_POLLING_INTERVAL,                          /* bInterval */ \
      }, \
    },

struct vendorhid_interface
{
  struct interface_descriptor             ctl_interface;
  struct hid_functional_descriptor        hid_func;
  struct endpoint_descriptor              ep_in;
  struct endpoint_descriptor              ep_out;
};

#endif /* __VENDORHID_HELPER_H */
