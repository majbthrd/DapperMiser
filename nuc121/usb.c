/*
 * Copyright (c) 2018, 2019, Peter Lawrence
 * All rights reserved.
 *
 * This code originated from:
 * https://github.com/majbthrd/NUC121usb
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

/*
  Theory of operation:

  This code is intended to have an API similar to https://github.com/ataradov/vcp
  However, some changes had to be made to better suit the Nuvoton NUC121/NUC125.

  The NUC121/NUC125 USBD peripheral has eight "EP"s, but each is simplex, so two 
  collectively (peripheral nomenclature of "EP0" and "EP1") are needed to 
  implement USB EP0.  This leaves six for user usage.

  The presumption of this code is that:
  - the same endpoint number is not used as both an IN and an OUT
  - endpoint numbers are between 1 and 6 (inclusive)
*/

/*- Includes ----------------------------------------------------------------*/
#include <string.h>
#include <stdbool.h>
#include "NUC121.h"
#include "utils.h"
#include "usb.h"
#include "usb_std.h"
#include "usb_descriptors.h"

/*- Definitions -------------------------------------------------------------*/
#define PERIPH_SETUP_BUF_BASE  0
#define PERIPH_SETUP_BUF_LEN   8
#define PERIPH_EP0_BUF_BASE    (PERIPH_SETUP_BUF_BASE + PERIPH_SETUP_BUF_LEN)
#define PERIPH_EP0_BUF_LEN     64
#define PERIPH_EP1_BUF_BASE    (PERIPH_EP0_BUF_BASE + PERIPH_EP0_BUF_LEN)
#define PERIPH_EP1_BUF_LEN     64
#define PERIPH_EP2_BUF_BASE    (PERIPH_EP1_BUF_BASE + PERIPH_EP1_BUF_LEN)

#define USBD_BUF_BASE   (USBD_BASE + 0x100)
#define USBD_MAX_EP     8

enum ep_enum
{
  PERIPH_EP0 = 0,
  PERIPH_EP1 = 1,
  PERIPH_EP2 = 2,
  PERIPH_EP3 = 3,
  PERIPH_EP4 = 4,
  PERIPH_EP5 = 5,
  PERIPH_EP6 = 6,
  PERIPH_EP7 = 7,
};

enum
{
  USBD_CFG_EPMODE_DISABLE = (0ul << USBD_CFG_STATE_Pos),
  USBD_CFG_EPMODE_OUT     = (1ul << USBD_CFG_STATE_Pos),
  USBD_CFG_EPMODE_IN      = (2ul << USBD_CFG_STATE_Pos),
  USBD_CFG_TYPE_ISO       = (1ul << USBD_CFG_ISOCH_Pos),
};

/*- Types -------------------------------------------------------------------*/
typedef void (*usb_ep_callback_t)(uint8_t *data, int size);

/*- Variables ---------------------------------------------------------------*/

/* copy of SETUP package retrieved from USBD buffer */
static usb_request_t request;

/* set by usb_set_address(), it is subsequently used by ep0_callback() */
static volatile uint32_t assigned_address;

/* used to track transfers (multi-packet or not) over control IN endpoint */
static volatile uint8_t *usb_ctrl_in_ptr;
static volatile uint32_t usb_ctrl_in_len;

/* buffer utilized for control OUT transfers */
static uint8_t usb_ctrl_out_buf[PERIPH_EP0_BUF_LEN];

/* optional callback for control OUT endpoint */
static void (*usb_control_recv_callback)(uint8_t *data, int size);

/* reset by usb_configure_rewind(), this is used by usb_configure_endpoint() to assign USBD peripheral buffer addresses */
static uint32_t bufseg_addr;

/* callbacks for interrupts USBD_INTSTS_EPEVT0_Msk through USBD_INTSTS_EP7_Msk */
static usb_ep_callback_t usb_ep_callbacks[USBD_MAX_EP];

/*- Prototypes --------------------------------------------------------------*/
static void periph_ep0_callback(uint8_t *data, int size);
static void periph_ep1_callback(uint8_t *data, int size);
static void usb_callback(enum ep_enum ep_index, uint8_t *data, int size);

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void usb_set_callback(int ep, void (*callback)(uint8_t *data, int size))
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  usb_ep_callbacks[ep_index] = callback;
}

static void rewind_ctrlin(void)
{
  usb_ctrl_in_ptr = 0;
  usb_ctrl_in_len = 0;
}

//-----------------------------------------------------------------------------
void usb_hw_init(void)
{
#ifdef SUPPORT_LPM
  USBD->ATTR = 0x7D0 | USBD_LPMACK;
#else
  USBD->ATTR = 0x7D0;
#endif

  usb_detach();

  USBD->STBUFSEG = PERIPH_SETUP_BUF_BASE;

  USBD->EP[PERIPH_EP0].CFG = USBD_CFG_CSTALL_Msk | USBD_CFG_EPMODE_IN;
  USBD->EP[PERIPH_EP0].BUFSEG = PERIPH_EP0_BUF_BASE;

  USBD->EP[PERIPH_EP1].CFG = USBD_CFG_CSTALL_Msk | USBD_CFG_EPMODE_OUT;
  USBD->EP[PERIPH_EP1].BUFSEG = PERIPH_EP1_BUF_BASE;

  for (enum ep_enum ep_index = PERIPH_EP2; ep_index < USBD_MAX_EP; ep_index++)
  {
    usb_reset_endpoint(ep_index);
  }
  usb_ep_callbacks[PERIPH_EP0] = periph_ep0_callback;
  usb_ep_callbacks[PERIPH_EP1] = periph_ep1_callback;

  assigned_address = 0;

  usb_attach();

  USBD->INTSTS = USBD_INTEN_BUSIEN_Msk | USBD_INTEN_USBIEN_Msk | USBD_INTEN_VBDETIEN_Msk | USBD_INTEN_WKEN_Msk | USBD_INTEN_SOFIEN_Msk;
  USBD->INTEN  = USBD_INTEN_BUSIEN_Msk | USBD_INTEN_USBIEN_Msk | USBD_INTEN_VBDETIEN_Msk | USBD_INTEN_WKEN_Msk | USBD_INTEN_SOFIEN_Msk;

//  NVIC_EnableIRQ(USBD_IRQn);
}

//-----------------------------------------------------------------------------
void usb_attach(void)
{
  USBD->SE0 &= ~USBD_SE0_SE0_Msk;
}

//-----------------------------------------------------------------------------
void usb_detach(void)
{
  USBD->SE0 |= USBD_SE0_SE0_Msk;
}

//-----------------------------------------------------------------------------
void usb_reset_endpoint(int ep)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  USBD->EP[ep_index].CFGP &= ~USBD_CFG_STATE_Msk;
}

//-----------------------------------------------------------------------------
void usb_configure_rewind(void)
{
  bufseg_addr = PERIPH_EP2_BUF_BASE;
}

//-----------------------------------------------------------------------------
void usb_configure_endpoint(usb_endpoint_descriptor_t *desc)
{
  int ep, dir, type, size;
  uint32_t cfg;

  ep = desc->bEndpointAddress & USB_INDEX_MASK;
  dir = desc->bEndpointAddress & USB_DIRECTION_MASK;
  type = desc->bmAttributes & 0x03;
  size = desc->wMaxPacketSize & 0x3ff;

  enum ep_enum ep_index = PERIPH_EP1 + ep;

  cfg = ep;
  cfg |= (USB_IN_ENDPOINT == dir) ? USBD_CFG_EPMODE_IN : USBD_CFG_EPMODE_OUT;
  if (USB_ISOCHRONOUS_ENDPOINT == type)
    cfg |= USBD_CFG_TYPE_ISO;
  
  USBD->EP[ep_index].CFG = cfg;
  if (USB_OUT_ENDPOINT == dir)
    USBD->EP[ep_index].MXPLD = size;
  USBD->EP[ep_index].BUFSEG = bufseg_addr;
  bufseg_addr += size;
}

//-----------------------------------------------------------------------------
bool usb_endpoint_configured(int ep, int dir)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  uint32_t searchfor = ep << USBD_CFG_EPNUM_Pos;

  if (USB_IN_ENDPOINT == dir)
    searchfor |= USBD_CFG_EPMODE_IN;
  else
    searchfor |= USBD_CFG_EPMODE_OUT;

  return (searchfor == (USBD->EP[ep_index].CFG & (USBD_CFG_STATE_Pos | USBD_CFG_EPNUM_Msk)));
}

//-----------------------------------------------------------------------------
int usb_endpoint_get_status(int ep, int dir)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  return (USBD->EP[ep_index].CFGP & USBD_CFGP_SSTALL_Msk) ? 1 : 0;
  (void)dir;
}

//-----------------------------------------------------------------------------
void usb_endpoint_set_feature(int ep, int dir)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  USBD->EP[ep_index].CFGP |= USBD_CFGP_SSTALL_Msk;
  (void)dir;
}

//-----------------------------------------------------------------------------
void usb_endpoint_clear_feature(int ep, int dir)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  USBD->EP[ep_index].CFGP &= ~USBD_CFGP_SSTALL_Msk;
  (void)dir;
}

//-----------------------------------------------------------------------------
void usb_set_address(int address)
{
  assigned_address = address;
}

//-----------------------------------------------------------------------------
void usb_send(int ep, uint8_t *data, int size)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  memcpy((uint8_t *)(USBD_BUF_BASE + USBD->EP[ep_index].BUFSEG), data, size);
  USBD->EP[ep_index].MXPLD = size;
}

//-----------------------------------------------------------------------------
void usb_recv(int ep, int size)
{
  enum ep_enum ep_index = PERIPH_EP1 + ep;
  USBD->EP[ep_index].MXPLD = size;
}

//-----------------------------------------------------------------------------
void usb_control_send_zlp(void)
{
  USBD->EP[PERIPH_EP0].CFG |= USBD_CFG_DSQSYNC_Msk;
  USBD->EP[PERIPH_EP0].MXPLD = 0;
}

//-----------------------------------------------------------------------------
void usb_control_stall(void)
{
  USBD->EP[PERIPH_EP0].CFGP |= USBD_CFGP_SSTALL_Msk;
  USBD->EP[PERIPH_EP1].CFGP |= USBD_CFGP_SSTALL_Msk;
}

//-----------------------------------------------------------------------------
void usb_control_send(uint8_t *data, int size)
{
  int transfer_size = size;

  if(transfer_size > usb_device_descriptor.bMaxPacketSize0)
    transfer_size = usb_device_descriptor.bMaxPacketSize0;

  USBD->EP[PERIPH_EP0].CFG |= USBD_CFG_DSQSYNC_Msk;
  memcpy((uint8_t *)USBD_BUF_BASE + USBD->EP[PERIPH_EP0].BUFSEG, data, transfer_size);
  USBD->EP[PERIPH_EP0].MXPLD = transfer_size;

  if(size > usb_device_descriptor.bMaxPacketSize0)
  {
    usb_ctrl_in_ptr = data + usb_device_descriptor.bMaxPacketSize0;
    usb_ctrl_in_len = size - usb_device_descriptor.bMaxPacketSize0;
  }
  else
  {
    usb_ctrl_in_ptr = 0;
    usb_ctrl_in_len = 0;
  }
}

//-----------------------------------------------------------------------------
void usb_control_recv(void (*callback)(uint8_t *data, int size))
{
  usb_control_send_zlp();
  usb_control_recv_callback = callback;
}

static void periph_ep0_callback(uint8_t *data, int size)
{
  static bool zlp_needed = 0;
  (void)size;

  if (usb_ctrl_in_len)
  {
    uint32_t transfer_size = usb_ctrl_in_len;
    if (transfer_size > usb_device_descriptor.bMaxPacketSize0)
      transfer_size = usb_device_descriptor.bMaxPacketSize0;
    else if (transfer_size == usb_device_descriptor.bMaxPacketSize0)
      zlp_needed = true;

    memcpy(data, (uint8_t *)usb_ctrl_in_ptr, transfer_size);
    USBD->EP[PERIPH_EP0].MXPLD = transfer_size;
    usb_ctrl_in_ptr += transfer_size;
    usb_ctrl_in_len -= transfer_size;
  }
  else
  {
    /* given ACK from host has happened, we can now set the address (if not already done) */
    if((USBD->FADDR != assigned_address) && (USBD->FADDR == 0))
    {
        USBD->FADDR = assigned_address;
    }

    if (zlp_needed)
    {
        USBD->EP[PERIPH_EP0].MXPLD = 0;
        zlp_needed = false;
    }
  }
}

static void periph_ep1_callback(uint8_t *data, int size)
{
  memcpy(usb_ctrl_out_buf, data, size);
  USBD->EP[PERIPH_EP1].MXPLD = usb_device_descriptor.bMaxPacketSize0;

  if (usb_control_recv_callback)
  {
    usb_control_recv_callback(usb_ctrl_out_buf, size);
    usb_control_recv_callback = NULL;
  }
}

//-----------------------------------------------------------------------------
void usb_task(void)
{
  uint32_t status = USBD->INTSTS;
#ifdef SUPPORT_LPM
  uint32_t state = USBD->ATTR & 0x300f;
#else
  uint32_t state = USBD->ATTR & 0xf;
#endif

  if(status & USBD_INTSTS_VBDETIF_Msk)
  {
    USBD->INTSTS = USBD_INTSTS_VBDETIF_Msk;

    if(USBD->VBUSDET & USBD_VBUSDET_VBUSDET_Msk)
    {
      /* USB connect */
      USBD->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;
    }
    else
    {
      /* USB disconnect */
      USBD->ATTR &= ~USBD_ATTR_USBEN_Msk;
    }
  }

  if(status & USBD_INTSTS_BUSIF_Msk)
  {
    USBD->INTSTS = USBD_INTSTS_BUSIF_Msk;

    if(state & USBD_ATTR_USBRST_Msk)
    {
      /* USB bus reset */
      USBD->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;

      rewind_ctrlin();

      /* Reset all endpoints to DATA0 */
      for(enum ep_enum ep_index = 0; ep_index < USBD_MAX_EP; ep_index++)
        USBD->EP[ep_index].CFG &= ~USBD_CFG_DSQSYNC_Msk;

      /* Reset USB device address */
      USBD->FADDR = 0;
    }

    if(state & USBD_ATTR_SUSPEND_Msk)
    {
      /* Enable USB but disable PHY */
      USBD->ATTR &= ~USBD_ATTR_PHYEN_Msk;
    }

    if(state & USBD_ATTR_RESUME_Msk)
    {
      /* Enable USB and enable PHY */
      USBD->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;
    }
  }

  if(status & USBD_INTSTS_USBIF_Msk)
  {
    if(status & USBD_INTSTS_SETUP_Msk)
    {
      USBD->INTSTS = USBD_INTSTS_SETUP_Msk;

      /* clear the data ready flag of control endpoints */
      USBD->EP[PERIPH_EP0].CFGP |= USBD_CFGP_CLRRDY_Msk;
      USBD->EP[PERIPH_EP1].CFGP |= USBD_CFGP_CLRRDY_Msk;

      /* get SETUP packet from USB buffer */
      memcpy((void *)&request, (uint8_t *)USBD_BUF_BASE, 8);

      rewind_ctrlin();

      if (usb_handle_standard_request(&request))
      {
        USBD->EP[PERIPH_EP1].MXPLD = usb_device_descriptor.bMaxPacketSize0;
      }
      else
      {
        usb_control_stall();
      }
    }

    /* service EP0 through EP7 */
    enum ep_enum ep_index;
    uint32_t mask;
    for (ep_index = PERIPH_EP0, mask = USBD_INTSTS_EPEVT0_Msk; ep_index <= PERIPH_EP7; ep_index++, mask <<= 1)
    {
      if(status & mask)
      {
        USBD->INTSTS = mask;
        usb_callback(ep_index, (uint8_t *)(USBD_BUF_BASE + USBD->EP[ep_index].BUFSEG), USBD->EP[ep_index].MXPLD);
      }
    }
  }

  if(status & USBD_INTSTS_SOFIF_Msk)
  {
    USBD->INTSTS = USBD_INTSTS_SOFIF_Msk;

    usb_sof_callback();
  }
}

//-----------------------------------------------------------------------------
static void usb_callback(enum ep_enum ep_index, uint8_t *data, int size)
{
  if (usb_ep_callbacks[ep_index])
    usb_ep_callbacks[ep_index](data, size);
}

//-----------------------------------------------------------------------------
WEAK void usb_sof_callback(void) {}
