/*
    CMSIS-DAP implementation for STM32F042/STM32F072

    Copyright (C) 2013-2018 Peter Lawrence.

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

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usbhelper.h"
#include "usbd_cdc.h"
#include "cdchelper.h"
#include "usbd_vendorhid.h"
#include "vendorhidhelper.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define USBD_VID                      0x1209
#define USBD_PID                      0x0001
#define USBD_LANGID_STRING            0x409
#define USBD_MANUFACTURER_STRING      "Acme"
#define USBD_PRODUCT_FS_STRING        "CMSIS-DAP"

/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint8_t *USBD_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static void IntToUnicode (uint32_t value, uint8_t *pbuf, uint8_t len);

/* Private variables ---------------------------------------------------------*/
const USBD_DescriptorsTypeDef USBD_Desc =
{
  USBD_DeviceDescriptor,
  USBD_LangIDStrDescriptor, 
  USBD_ManufacturerStrDescriptor,
  USBD_ProductStrDescriptor,
  USBD_SerialStrDescriptor,
};

/* USB Standard Device Descriptor */
static const struct device_descriptor hUSBDDeviceDesc =
{
  sizeof(hUSBDDeviceDesc),    /* bLength */
  USB_DESC_TYPE_DEVICE,       /* bDescriptorType */
  USB_UINT16(0x0200),         /* bcdUSB */
  0x00,                       /* bDeviceClass */
  0x00,                       /* bDeviceSubClass */
  0x00,                       /* bDeviceProtocol */
  USB_MAX_EP0_SIZE,           /* bMaxPacketSize */
  USB_UINT16(USBD_VID),       /* idVendor */
  USB_UINT16(USBD_PID),       /* idProduct */
  USB_UINT16(0x0200),         /* bcdDevice */
  USBD_IDX_MFC_STR,           /* Index of manufacturer string */
  USBD_IDX_PRODUCT_STR,       /* Index of product string */
  USBD_IDX_SERIAL_STR,        /* Index of serial number string */
  USBD_MAX_NUM_CONFIGURATION  /* bNumConfigurations */
};

/* USB device Configuration Descriptor */

/* bespoke struct for this device; struct members are added and removed as needed */
struct configuration_1
{
  struct configuration_descriptor config;
  struct vendorhid_interface vhid[NUM_OF_VENDORHID];
  struct cdc_interface cdc[NUM_OF_CDC_UARTS];
};

/* fully initialize the bespoke struct as a const */
__ALIGN_BEGIN static const struct configuration_1 USBD_Composite_CfgFSDesc __ALIGN_END =
{
  {
    /*Configuration Descriptor*/
    sizeof(struct configuration_descriptor),         /* bLength */
    USB_DESC_TYPE_CONFIGURATION,                     /* bDescriptorType */
    USB_UINT16(sizeof(USBD_Composite_CfgFSDesc)),    /* wTotalLength */
    USBD_MAX_NUM_INTERFACES,                         /* bNumInterfaces */
    0x01,                                            /* bConfigurationValue */
    0x00,                                            /* iConfiguration */
    0x80,                                            /* bmAttributes */
    50,                                              /* MaxPower */
  },

  {
#if (NUM_OF_VENDORHID > 0)
    VENDORHID_DESCRIPTOR(/* ITF */ (2 * NUM_OF_CDC_UARTS), /* DataOut EP */ 0x07, /* DataIn EP */ 0x87, /* HID report size */ 35)
#endif
  },

  {
#if (NUM_OF_CDC_UARTS > 0)
    /* CDC1 */
    CDC_DESCRIPTOR(/* Command ITF */ 0x00, /* Data ITF */ 0x01, /* Command EP */ 0x82, /* DataOut EP */ 0x01, /* DataIn EP */ 0x81)
#endif
#if (NUM_OF_CDC_UARTS > 1)
    /* CDC2 */
    CDC_DESCRIPTOR(/* Command ITF */ 0x02, /* Data ITF */ 0x03, /* Command EP */ 0x84, /* DataOut EP */ 0x03, /* DataIn EP */ 0x83)
#endif
#if (NUM_OF_CDC_UARTS > 2)
    /* CDC3 */
    CDC_DESCRIPTOR(/* Command ITF */ 0x04, /* Data ITF */ 0x05, /* Command EP */ 0x86, /* DataOut EP */ 0x05, /* DataIn EP */ 0x85)
#endif
  },
};

/* pointer and length of configuration descriptor for main USB driver */
const uint8_t *const USBD_CfgFSDesc_pnt = (const uint8_t *)&USBD_Composite_CfgFSDesc;
const uint16_t USBD_CfgFSDesc_len = sizeof(USBD_Composite_CfgFSDesc);

/*
A peculiarity of the HID protocol is that the driver needs to be able to provide its report descriptor on request.
Given this, we must manually create some data structures to point to the report descriptor within the above data.
*/

const struct USBD_CfgFSHIDDesc_struct USBD_CfgFSHIDDesc_array[NUM_OF_VENDORHID] =
{
#if (NUM_OF_VENDORHID > 0)
  { (const uint8_t *)&USBD_Composite_CfgFSDesc.vhid[0].hid_func, sizeof(USBD_Composite_CfgFSDesc.vhid[0].hid_func) },
#endif
};

const struct USBD_CfgFSHIDDesc_struct *USBD_CfgFSHIDDesc = USBD_CfgFSHIDDesc_array;

/* USB Standard Device Descriptor */
static const uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC]= 
{
  USB_LEN_LANGID_STR_DESC,         
  USB_DESC_TYPE_STRING,       
  LOBYTE(USBD_LANGID_STRING),
  HIBYTE(USBD_LANGID_STRING), 
};

static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ];

/**  * @brief  Returns the device descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(hUSBDDeviceDesc);
  return (uint8_t*)&hUSBDDeviceDesc;
}

/**
  * @brief  Returns the LangID string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(USBD_LangIDDesc);  
  return (uint8_t*)USBD_LangIDDesc;
}

/**
  * @brief  Returns the product string descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING, USBD_StrDesc, length);    
  return USBD_StrDesc;
}

/**
  * @brief  Returns the manufacturer string descriptor. 
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
  return USBD_StrDesc;
}

/**
  * @brief  Returns the serial number string descriptor.        
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
static uint8_t *USBD_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  uint32_t deviceserial0, deviceserial1, deviceserial2;
  
  /*
  for some peculiar reason, ST doesn't define the unique ID registers in the HAL include files
  these registers are documented in Chapter 33 of the RM0091 Reference Manual
  */
  deviceserial0 = *(uint32_t*)(0x1FFFF7AC); /*DEVICE_ID1*/
  deviceserial1 = *(uint32_t*)(0x1FFFF7B0); /*DEVICE_ID2*/
  deviceserial2 = *(uint32_t*)(0x1FFFF7B4); /*DEVICE_ID3*/
  
  deviceserial0 += deviceserial2;
  
  USBD_StrDesc[0] = *length = 0x1A;
  USBD_StrDesc[1] = USB_DESC_TYPE_STRING;
  IntToUnicode (deviceserial0, &USBD_StrDesc[2] ,8);
  IntToUnicode (deviceserial1, &USBD_StrDesc[18] ,4);
  return USBD_StrDesc;  
}

/**
  * @brief  Convert Hex 32Bits value into char 
  * @param  value: value to convert
  * @param  pbuf: pointer to the buffer 
  * @param  len: buffer length
  * @retval None
  */
static void IntToUnicode (uint32_t value, uint8_t *pbuf, uint8_t len)
{
  uint8_t idx = 0;
  
  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
    }
    
    value = value << 4;
    
    pbuf[ 2* idx + 1] = 0;
  }
}
