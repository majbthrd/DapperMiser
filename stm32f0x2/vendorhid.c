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

#include "vendorhid.h"
#include "swdio_bsp.h"
#include "dm.h"

/*
since parsing and responding to VendorHID is expected to take time, 
these routines are implemented to run primarily in the main loop rather than in the ISR context
*/

static struct
{
  uint32_t length;
  uint8_t rxbuffer[HID_EP_SIZE];
  uint8_t txbuffer[HID_EP_SIZE];
  uint8_t data_in_ep;
  USBD_HandleTypeDef *pdev;
} message[NUM_OF_VENDORHID];

void VendorHID_Callback(USBD_HandleTypeDef *pdev, unsigned index, uint8_t *buffer, uint32_t length, uint8_t data_in_ep)
{
  /* DO NOT BLOCK; it is imperative that this function returns quickly, as it is called by the ISR */

  memcpy(message[index].rxbuffer, buffer, length);
  message[index].length = length;
  message[index].data_in_ep = data_in_ep;
  message[index].pdev = pdev;
}

extern void vendor_extension(const uint8_t *RxDataBuffer, uint8_t *TxDataBuffer);
extern void vendor_extension_init(void);

void VendorHID_Service(void)
{
  unsigned index;
  uint8_t *TxDataBuffer, *RxDataBuffer;
  uint8_t *pnt, val;
  unsigned count;

  for (index = 0; index < NUM_OF_VENDORHID; index++)
  {
    if (message[index].length)
    {
      TxDataBuffer = message[index].txbuffer;
      RxDataBuffer = message[index].rxbuffer;

      if ( (RxDataBuffer[0] >= 0x80) && (RxDataBuffer[0] < 0xA0) )
      {
        /* ID_DAP_Vendor0 through ID_DAP_Vendor31 */
        vendor_extension(RxDataBuffer, TxDataBuffer);
      }
      else
      {
        dap_handler(RxDataBuffer);
        memcpy(TxDataBuffer, RxDataBuffer, HID_EP_SIZE);
      }

      /* send back response */
      USBD_LL_Transmit(message[index].pdev, message[index].data_in_ep, TxDataBuffer, HID_EP_SIZE);

      /* mark that we've handled the message */
      message[index].length = 0;

      break;
    }
  }
}

void VendorHID_Init(void)
{
  unsigned index;

  SWDIO_INIT;
  DATA_HIZ;
  CLK_HIZ;
  RESET_HIZ;

  vendor_extension_init();

  for (index = 0; index < NUM_OF_VENDORHID; index++)
  {
    message[index].length = 0;
  }
}

__weak void vendor_extension(const uint8_t *TxDataBuffer, uint8_t *RxDataBuffer) {}
__weak void vendor_extension_init(void) {}
