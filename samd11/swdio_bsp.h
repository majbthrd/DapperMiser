#ifndef __SWDIO_BSP_H
#define __SWDIO_BSP_H

#include <samd11.h>

/*
this must be customized to suit the end application
*/

#define CLK_PIN   8
#define DATA_PIN  5
#define RESET_PIN 9
#define PORTGROUP 0

/* FIXME: free-dap always sets INEN, but is this right? */

#define CLK_LOW      { PORT->Group[PORTGROUP].OUTCLR.reg = (1 << CLK_PIN); }
#define CLK_HIGH     { PORT->Group[PORTGROUP].OUTSET.reg = (1 << CLK_PIN); }
#define CLK_ENABLE   { PORT->Group[PORTGROUP].DIRSET.reg = (1 << CLK_PIN); PORT->Group[PORTGROUP].PINCFG[CLK_PIN].reg |= PORT_PINCFG_INEN; }
#define CLK_HIZ      { PORT->Group[PORTGROUP].DIRCLR.reg = (1 << CLK_PIN); PORT->Group[PORTGROUP].PINCFG[CLK_PIN].reg |= PORT_PINCFG_INEN; }

#define DATA_LOW     { PORT->Group[PORTGROUP].OUTCLR.reg = (1 << DATA_PIN); }
#define DATA_HIGH    { PORT->Group[PORTGROUP].OUTSET.reg = (1 << DATA_PIN); }
#define DATA_ENABLE  { PORT->Group[PORTGROUP].DIRSET.reg = (1 << DATA_PIN); PORT->Group[PORTGROUP].PINCFG[DATA_PIN].reg |= PORT_PINCFG_INEN; }
#define DATA_HIZ     { PORT->Group[PORTGROUP].DIRCLR.reg = (1 << DATA_PIN); PORT->Group[PORTGROUP].PINCFG[DATA_PIN].reg |= PORT_PINCFG_INEN; }

#define RESET_LOW    { PORT->Group[PORTGROUP].OUTCLR.reg = (1 << RESET_PIN); }
#define RESET_HIGH   { PORT->Group[PORTGROUP].OUTSET.reg = (1 << RESET_PIN); }
#define RESET_ENABLE { PORT->Group[PORTGROUP].DIRSET.reg = (1 << RESET_PIN); PORT->Group[PORTGROUP].PINCFG[RESET_PIN].reg |= PORT_PINCFG_INEN; }
#define RESET_HIZ    { PORT->Group[PORTGROUP].DIRCLR.reg = (1 << RESET_PIN); PORT->Group[PORTGROUP].PINCFG[RESET_PIN].reg |= PORT_PINCFG_INEN; }

#define SWDIO_INIT  { }

#define DATA_READ   (PORT->Group[PORTGROUP].IN.reg & (1UL << DATA_PIN))
#define CLK_READ    (PORT->Group[PORTGROUP].IN.reg & (1UL << CLK_PIN))
#define RESET_READ  (PORT->Group[PORTGROUP].IN.reg & (1UL << RESET_PIN))

#endif /* __SWDIO_BSP_H */
