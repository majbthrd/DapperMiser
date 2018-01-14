#ifndef __DM_BSP_H
#define __DM_BSP_H

#include "usbd_vendorhid.h" /* for HID_EP_SIZE */

#define DAP_PACKET_COUNT  1
#define DAP_PACKET_SIZE   HID_EP_SIZE

#define DAP_SUPPORT_JTAG_SEQUENCE

#endif /* __DM_BSP_H */
