#ifndef __SWDIO_BSP_H
#define __SWDIO_BSP_H

#include <NUC121.h>

/*
this must be customized to suit the end application
*/

#define MODE_INPUT  0UL
#define MODE_OUTPUT 1UL

#define CLK_LOW      { PB->DOUT &= ~GPIO_DOUT_DOUT14_Msk; }
#define CLK_HIGH     { PB->DOUT |=  GPIO_DOUT_DOUT14_Msk; }
#define CLK_ENABLE   { PB->MODE = (PB->MODE & ~GPIO_MODE_MODE14_Msk) | (MODE_OUTPUT << GPIO_MODE_MODE14_Pos); }
#define CLK_HIZ      { PB->MODE = (PB->MODE & ~GPIO_MODE_MODE14_Msk) | (MODE_INPUT  << GPIO_MODE_MODE14_Pos); }

#define DATA_LOW     { PB->DOUT &= ~GPIO_DOUT_DOUT13_Msk; }
#define DATA_HIGH    { PB->DOUT |=  GPIO_DOUT_DOUT13_Msk; }
#define DATA_ENABLE  { PB->MODE = (PB->MODE & ~GPIO_MODE_MODE13_Msk) | (MODE_OUTPUT << GPIO_MODE_MODE13_Pos); }
#define DATA_HIZ     { PB->MODE = (PB->MODE & ~GPIO_MODE_MODE13_Msk) | (MODE_INPUT  << GPIO_MODE_MODE13_Pos); }

#define RESET_LOW    { PB->DOUT &= ~GPIO_DOUT_DOUT12_Msk; }
#define RESET_HIGH   { PB->DOUT |=  GPIO_DOUT_DOUT12_Msk; }
#define RESET_ENABLE { PB->MODE = (PB->MODE & ~GPIO_MODE_MODE12_Msk) | (MODE_OUTPUT << GPIO_MODE_MODE12_Pos); }
#define RESET_HIZ    { PB->MODE = (PB->MODE & ~GPIO_MODE_MODE12_Msk) | (MODE_INPUT  << GPIO_MODE_MODE12_Pos); }

#define SWDIO_INIT  { }

#define DATA_READ   ( PB->PIN & GPIO_PIN_PIN13_Msk )
#define CLK_READ    ( PB->PIN & GPIO_PIN_PIN14_Msk )
#define RESET_READ  ( PB->PIN & GPIO_PIN_PIN12_Msk )

#endif /* __SWDIO_BSP_H */
