#ifndef __SWDIO_BSP_H
#define __SWDIO_BSP_H

/*
As implemented in the sample schematic:
C0: SWCLK
C1: SWCLK_DIR
C2: SWDIO
C3: SWDIO_DIR
A4: RESET
*/

#define CLK_LOW      LATCbits.LATC0 = 0;
#define CLK_HIGH     LATCbits.LATC0 = 1;
#define CLK_ENABLE   {TRISCbits.TRISC1 = 0; LATCbits.LATC1 = 1; TRISCbits.TRISC0 = 0;}
#define CLK_HIZ      {TRISCbits.TRISC0 = 1; LATCbits.LATC1 = 0; TRISCbits.TRISC1 = 1;}

#define DATA_LOW      LATCbits.LATC2 = 0;
#define DATA_HIGH     LATCbits.LATC2 = 1;
#define DATA_ENABLE   {TRISCbits.TRISC3 = 0; LATCbits.LATC3 = 1; TRISCbits.TRISC2 = 0;}
#define DATA_HIZ      {TRISCbits.TRISC2 = 1; LATCbits.LATC3 = 0; TRISCbits.TRISC3 = 1;}

#define RESET_LOW    LATAbits.LATA4 = 1;
#define RESET_HIGH   LATAbits.LATA4 = 0;
#define RESET_ENABLE TRISAbits.TRISA4 = 0;
#define RESET_HIZ    TRISAbits.TRISA4 = 1;

#define SWDIO_INIT   {ANSELCbits.ANSC0 = 0; ANSELCbits.ANSC1 = 0; ANSELCbits.ANSC2 = 0; ANSELCbits.ANSC3 = 0; ANSELA = 0;}

#define DATA_READ   (PORTCbits.RC2)
#define CLK_READ    (PORTCbits.RC4)
#define RESET_READ  (PORTAbits.RA4)

#endif /* __SWDIO_BSP_H */
