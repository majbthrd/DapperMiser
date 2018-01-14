#ifndef __VENDORHID_H
#define __VENDORHID_H

#include "usbd_vendorhid.h"

extern void VendorHID_Callback(USBD_HandleTypeDef *pdev, unsigned index, uint8_t *buffer, uint32_t length, uint8_t data_in_ep);
extern void VendorHID_Service(void);
extern void VendorHID_Init(void);

#endif  /* __VENDORHID_H */
