/*
MODIFIED:
- handler names replaced with more industry-standard ones
- linker memory map locations replaced with Rowley-convention ones
- added support for CMSIS SystemInit
- changed from SAMC21 to NUC121/NUC125
*/

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

#include <NUC121.h>

//-----------------------------------------------------------------------------
#define DUMMY __attribute__ ((weak, alias ("irq_handler_dummy")))

//-----------------------------------------------------------------------------
void Reset_Handler(void);
#ifndef STARTUP_FROM_RESET
void Reset_Wait(void) {while (1);}
#endif
DUMMY void NMI_Handler(void);
DUMMY void HardFault_Handler(void);
DUMMY void SVC_Handler(void);
DUMMY void PendSV_Handler(void);
DUMMY void SysTick_Handler(void);

DUMMY void BOD_IRQHandler(void);
DUMMY void WDT_IRQHandler(void);
DUMMY void EINT024_IRQHandler(void);
DUMMY void EINT135_IRQHandler(void);
DUMMY void GPAB_IRQHandler(void);
DUMMY void GPCDEF_IRQHandler(void);
DUMMY void PWM0_IRQHandler(void);
DUMMY void PWM1_IRQHandler(void);
DUMMY void TMR0_IRQHandler(void);
DUMMY void TMR1_IRQHandler(void);
DUMMY void TMR2_IRQHandler(void);
DUMMY void TMR3_IRQHandler(void);
DUMMY void UART0_IRQHandler(void);
DUMMY void SPI0_IRQHandler(void);
DUMMY void I2C0_IRQHandler(void);
DUMMY void I2C1_IRQHandler(void);
DUMMY void BPWM0_IRQHandler(void);
DUMMY void BPWM1_IRQHandler(void);
DUMMY void USCI_IRQHandler(void);
DUMMY void USBD_IRQHandler(void);
DUMMY void PWM_BRAKE_IRQHandler(void);
DUMMY void PDMA_IRQHandler(void);
DUMMY void PWRWU_IRQHandler(void);
DUMMY void ADC_IRQHandler(void);
DUMMY void CLKDIRC_IRQHandler(void);
DUMMY void Default_Handler(void);

extern int main(void);

extern void __stack_end__(void);
extern unsigned int __data_load_start__;
extern unsigned int __data_start__;
extern unsigned int __data_end__;
extern unsigned int __bss_start__;
extern unsigned int __bss_end__;

//-----------------------------------------------------------------------------
__attribute__ ((used, section(".vectors")))
void (* const vectors[])(void) =
{
  &__stack_end__,            // 0 - Initial Stack Pointer Value

  // Cortex-M0+ handlers
#ifdef STARTUP_FROM_RESET
  Reset_Handler,             // 1 - Reset
#else
  Reset_Wait,
#endif
  NMI_Handler,               // 2 - NMI
  HardFault_Handler,         // 3 - Hard Fault
  0,                         // 4 - Reserved
  0,                         // 5 - Reserved
  0,                         // 6 - Reserved
  0,                         // 7 - Reserved
  0,                         // 8 - Reserved
  0,                         // 9 - Reserved
  0,                         // 10 - Reserved
  SVC_Handler,               // 11 - SVCall
  0,                         // 12 - Reserved
  0,                         // 13 - Reserved
  PendSV_Handler,            // 14 - PendSV
  SysTick_Handler,           // 15 - SysTick

  // Peripheral handlers
  BOD_IRQHandler,
  WDT_IRQHandler,
  EINT024_IRQHandler,
  EINT135_IRQHandler,
  GPAB_IRQHandler,
  GPCDEF_IRQHandler,
  PWM0_IRQHandler,
  PWM1_IRQHandler,
  TMR0_IRQHandler,
  TMR1_IRQHandler,
  TMR2_IRQHandler,
  TMR3_IRQHandler,
  UART0_IRQHandler,
  Default_Handler,
  SPI0_IRQHandler,
  Default_Handler,
  Default_Handler,
  Default_Handler,
  I2C0_IRQHandler,
  I2C1_IRQHandler,
  BPWM0_IRQHandler,
  BPWM1_IRQHandler,
  USCI_IRQHandler,
  USBD_IRQHandler,
  Default_Handler,
  PWM_BRAKE_IRQHandler,
  PDMA_IRQHandler,
  Default_Handler,
  PWRWU_IRQHandler,
  ADC_IRQHandler,
  CLKDIRC_IRQHandler,
  Default_Handler,
};

//-----------------------------------------------------------------------------
void Reset_Handler(void)
{
  unsigned int *src, *dst;

#ifndef DONT_USE_CMSIS_INIT
  SystemInit();
#endif

  src = &__data_load_start__;
  dst = &__data_start__;
  while (dst < &__data_end__)
    *dst++ = *src++;

  dst = &__bss_start__;
  while (dst < &__bss_end__)
    *dst++ = 0;

  main();
  while (1);
}

//-----------------------------------------------------------------------------
void irq_handler_dummy(void)
{
  while (1);
}

//-----------------------------------------------------------------------------
void _exit(int status)
{
  (void)status;
  while (1);
}
