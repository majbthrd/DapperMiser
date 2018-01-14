#ifndef __SWDIO_BSP_H
#define __SWDIO_BSP_H

/*
this must be customized to suit the end application
*/

#define CLK_PIN   6
#define DATA_PIN  7
#define RESET_PIN 8

#define CLK_LOW      { GPIOC->BSRR = (1UL << CLK_PIN) << 16; }
#define CLK_HIGH     { GPIOC->BSRR = (1UL << CLK_PIN) << 0; }
#define CLK_ENABLE   { GPIOC->MODER = ( (GPIOC->MODER & ~(0x3 << (CLK_PIN * 2))) | (0x1 << (CLK_PIN * 2)) ); }
#define CLK_HIZ      { GPIOC->MODER = ( (GPIOC->MODER & ~(0x3 << (CLK_PIN * 2))) ); }

#define DATA_LOW     { GPIOC->BSRR = (1UL << DATA_PIN) << 16; }
#define DATA_HIGH    { GPIOC->BSRR = (1UL << DATA_PIN) << 0; }
#define DATA_ENABLE  { GPIOC->MODER = ( (GPIOC->MODER & ~(0x3 << (DATA_PIN * 2))) | (0x1 << (DATA_PIN * 2)) ); }
#define DATA_HIZ     { GPIOC->MODER = ( (GPIOC->MODER & ~(0x3 << (DATA_PIN * 2))) ); }

#define RESET_LOW    { GPIOC->BSRR = (1UL << RESET_PIN) << 16; }
#define RESET_HIGH   { GPIOC->BSRR = (1UL << RESET_PIN) << 0; }
#define RESET_ENABLE { GPIOC->MODER = ( (GPIOC->MODER & ~(0x3 << (RESET_PIN * 2))) | (0x1 << (RESET_PIN * 2)) ); }
#define RESET_HIZ    { GPIOC->MODER = ( (GPIOC->MODER & ~(0x3 << (RESET_PIN * 2))) ); }

#define SWDIO_INIT  { __GPIOC_CLK_ENABLE(); }

#define DATA_READ   (GPIOC->IDR & (1UL << DATA_PIN))
#define CLK_READ    (GPIOC->IDR & (1UL << CLK_PIN))
#define RESET_READ  (GPIOC->IDR & (1UL << RESET_PIN))

#endif /* __SWDIO_BSP_H */
