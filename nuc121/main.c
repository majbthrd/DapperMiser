/*
 * Copyright (c) 2018, Peter Lawrence
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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "NUC121.h"
#include "usb.h"
#include "clk.h"
#include "sys.h"

/*- Definitions -------------------------------------------------------------*/

/*- Variables ---------------------------------------------------------------*/

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static uint32_t fmc_uid(void)
{
  uint32_t uid = 0;

  /* unlock sequence to allow access to FMC->ISPCTL */
  SYS->REGLCTL = 0x59;
  SYS->REGLCTL = 0x16;
  SYS->REGLCTL = 0x88;

  CLK->AHBCLK |= CLK_AHBCLK_ISPCKEN_Msk;
  FMC->ISPCTL = FMC_ISPCTL_ISPEN_Msk;

  /* clear any existing fail flag */
  FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;

  /* perform ID command */
  for (int addr = 0; addr < 12; addr+=4)
  {
    FMC->ISPCMD = 0x04 /* read unique ID */;
    FMC->ISPADDR = addr;
    FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
    while (FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk);

    if (FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk)
      break;

    uid ^= FMC->ISPDAT;
  }

  FMC->ISPCTL = 0;

  return uid;
}

//-----------------------------------------------------------------------------
static void sys_init(void)
{
  /* Unlock protected registers */
  SYS_UnlockReg();

  /*---------------------------------------------------------------------------------------------------------*/
  /* Init System Clock                                                                                       */
  /*---------------------------------------------------------------------------------------------------------*/

  /* Enable Internal HIRC 48 MHz clock */
  CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN);

  /* Waiting for Internal RC clock ready */
  CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

  /* Switch HCLK clock source to Internal HIRC and HCLK source divide 1 */
  CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

  /* Enable module clock */
  CLK_EnableModuleClock(USBD_MODULE);

  /* Select module clock source */
  CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL3_USBDSEL_HIRC, CLK_CLKDIV0_USB(1));

  /* Enable module clock */
  CLK_EnableModuleClock(USBD_MODULE);

  uint32_t sn = fmc_uid();

  for (int i = 0; i < 8; i++)
    usb_serial_number[i] = "0123456789ABCDEF"[(sn >> (i * 4)) & 0xf];

  usb_serial_number[9] = 0;
}

//-----------------------------------------------------------------------------
int main(void)
{
  sys_init();
  usb_hw_init();
  usb_vendorhid_init();

  while (1)
  {
    usb_task();
    usb_vendorhid_task();
  }

  return 0;
}
