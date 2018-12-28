#ifndef __DM_BSP_H
#define __DM_BSP_H

#include <xc.h>
#include "usb_config.h" /* device-specific: for EP_1_OUT_LEN */

#define DAP_PACKET_COUNT  1
#define DAP_PACKET_SIZE   EP_1_OUT_LEN

#define DAP_SUPPORT_JTAG_SEQUENCE

#endif /* __DM_BSP_H */
