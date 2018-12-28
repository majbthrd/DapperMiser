/*
 * Copyright (C) 2013-2018 Peter Lawrence
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
#include <stdbool.h>
#include <string.h>
#include "utils.h"
#include "usb.h"
#include "dm.h"

/*- Prototypes --------------------------------------------------------------*/
static void usb_vendorhid_epin_callback(uint8_t *data, int size);
static void usb_vendorhid_epout_callback(uint8_t *data, int size);

/*- Variables ---------------------------------------------------------------*/
static uint8_t dap_buffer[DAP_PACKET_SIZE];
static volatile bool epout_pending, epin_pending;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void usb_vendorhid_init(void)
{
  usb_set_callback(USB_VENDORHID_IN, usb_vendorhid_epin_callback);
  usb_set_callback(USB_VENDORHID_OUT, usb_vendorhid_epout_callback);
}

void usb_vendorhid_task(void)
{
  if (!epout_pending)
    return;

  if (epin_pending)
    return;

  dap_handler(dap_buffer);
  usb_send(USB_VENDORHID_IN, dap_buffer, DAP_PACKET_SIZE);
  usb_recv(USB_VENDORHID_OUT, DAP_PACKET_SIZE);
  epin_pending = true;
  epout_pending = false;
}

static void usb_vendorhid_epin_callback(uint8_t *data, int size)
{
  (void)data;
  (void)size;
  epin_pending = false;
}

static void usb_vendorhid_epout_callback(uint8_t *data, int size)
{
  memcpy(dap_buffer, data, size);
  epout_pending = true;
}

void usb_configuration_callback(int config)
{
  epin_pending = epout_pending = false;
  usb_recv(USB_VENDORHID_OUT, DAP_PACKET_SIZE);
  (void)config;
}
