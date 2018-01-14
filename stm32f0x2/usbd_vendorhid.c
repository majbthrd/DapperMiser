/* Includes ------------------------------------------------------------------*/
#include "usbd_vendorhid.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "vendorhid.h"

static uint8_t  USBD_VendorHID_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_VendorHID_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_VendorHID_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  USBD_VendorHID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_VendorHID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);
static void     USBD_VendorHID_PMAConfig(PCD_HandleTypeDef *hpcd, uint32_t *pma_address);

const USBD_CompClassTypeDef USBD_VendorHID = 
{
  .Init                  = USBD_VendorHID_Init,
  .DeInit                = USBD_VendorHID_DeInit,
  .Setup                 = USBD_VendorHID_Setup,
  .EP0_TxSent            = NULL,
  .EP0_RxReady           = NULL,
  .DataIn                = USBD_VendorHID_DataIn,
  .DataOut               = USBD_VendorHID_DataOut,
  .SOF                   = NULL,
  .PMAConfig             = USBD_VendorHID_PMAConfig,
};

/* NOTE: manually ensure that the size of this report equals the value given in the descriptor in usbd_desc.c */

__ALIGN_BEGIN static const uint8_t VendorHID_ReportDesc[35]  __ALIGN_END =
{
  0x06, 0x00, 0xFF, // Usage Page = 0xFF00 (Vendor Defined Page 1)
  0x09, 0x01,       // Usage (Vendor Usage 1)
  0xA1, 0x01,       // Collection (Application)
  0x15, 0x00,       // Logical Minimum
  0x26, 0xFF, 0x00, // Logical Maximum
  0x75, 0x08,       // Report Size: 8-bit
  0x96, (HID_EP_SIZE >> 0), (HID_EP_SIZE >> 8), // Report Count
  0x09, 0x01,       // Usage (Vendor Usage 1)
  0x81, 0x02,       // Input: variable
  0x96, (HID_EP_SIZE >> 0), (HID_EP_SIZE >> 8), // Report Count
  0x09, 0x01,       // Usage (Vendor Usage 1)
  0x91, 0x02,       // Output: variable
  0x95, 0x01,       // Report Count
  0x09, 0x01,       // Usage (Vendor Usage 1)
  0xB1, 0x02,       // Feature: Variable
  0xC0,             // End Collection
}; 

/* HID report, endpoint numbers, and interface number for each VendorHID instance */
static const struct
{
  const uint8_t *ReportDesc;
  unsigned ReportDesc_Length;
  uint8_t data_in_ep, data_out_ep, itf_num;
} parameters[NUM_OF_VENDORHID] = 
{
#if (NUM_OF_VENDORHID > 0)
  {
    .ReportDesc = VendorHID_ReportDesc,
    .ReportDesc_Length = sizeof(VendorHID_ReportDesc),
    .data_in_ep = 0x87,
    .data_out_ep = 0x07,
    .itf_num = (2 * NUM_OF_CDC_UARTS),
  },
#endif
};

static USBD_VendorHID_HandleTypeDef context[NUM_OF_VENDORHID];

static uint8_t  USBD_VendorHID_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_VendorHID_HandleTypeDef *hhid = context;
  unsigned index;

  for (index = 0; index < NUM_OF_VENDORHID; index++,hhid++)
  {
    /* Open HID EPs */
    USBD_LL_OpenEP(pdev, parameters[index].data_in_ep, USBD_EP_TYPE_INTR, HID_EP_SIZE);  
    USBD_LL_OpenEP(pdev, parameters[index].data_out_ep, USBD_EP_TYPE_INTR, HID_EP_SIZE);  

    USBD_LL_PrepareReceive(pdev, parameters[index].data_out_ep, hhid->buffer, HID_EP_SIZE);
  }

  return USBD_OK;
}

static uint8_t  USBD_VendorHID_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  unsigned index;

  for (index = 0; index < NUM_OF_VENDORHID; index++)
  {
    /* Close HID EPs */
    USBD_LL_CloseEP(pdev, parameters[index].data_in_ep);
    USBD_LL_CloseEP(pdev, parameters[index].data_out_ep);
  }

  return USBD_OK;
}

static uint8_t  USBD_VendorHID_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  uint16_t len = 0;
  const uint8_t  *pbuf = NULL;
  USBD_VendorHID_HandleTypeDef *hhid = context;
  unsigned index;

  for (index = 0; index < NUM_OF_VENDORHID; index++,hhid++)
  {
    if (parameters[index].itf_num != req->wIndex)
      continue;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_CLASS :  
      switch (req->bRequest)
      {
      case HID_REQ_SET_PROTOCOL:
        hhid->Protocol = (uint8_t)(req->wValue);
        break;
      
      case HID_REQ_GET_PROTOCOL:
        USBD_CtlSendData (pdev, (uint8_t *)&hhid->Protocol, 1);    
        break;
      
      case HID_REQ_SET_IDLE:
        hhid->IdleState = (uint8_t)(req->wValue >> 8);
        break;
      
      case HID_REQ_GET_IDLE:
        USBD_CtlSendData (pdev, (uint8_t *)&hhid->IdleState, 1);        
        break;      
      
      default:
        USBD_CtlError (pdev, req);
        return USBD_FAIL; 
      }
      break;
    
    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
      case USB_REQ_GET_DESCRIPTOR: 
        if( req->wValue >> 8 == HID_REPORT_DESC)
        {
          len = MIN(parameters[index].ReportDesc_Length, req->wLength);
          pbuf = parameters[index].ReportDesc;
        }
        else if( req->wValue >> 8 == HID_DESCRIPTOR_TYPE)
        {
          len = MIN(USBD_CfgFSHIDDesc[index].len, req->wLength);
          pbuf = USBD_CfgFSHIDDesc[index].pnt;
        }
      
        USBD_CtlSendData (pdev, (uint8_t *)pbuf, len);
      
        break;
      
      case USB_REQ_GET_INTERFACE :
        USBD_CtlSendData (pdev, (uint8_t *)&hhid->AltSetting, 1);
        break;
      
      case USB_REQ_SET_INTERFACE :
        hhid->AltSetting = (uint8_t)(req->wValue);
        break;
      }
    }
  }

  return USBD_OK;
}

static uint8_t  USBD_VendorHID_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_VendorHID_HandleTypeDef *hhid = context;
  unsigned index;

  for (index = 0; index < NUM_OF_VENDORHID; index++,hhid++)
  {
    if (parameters[index].data_in_ep != (epnum | 0x80))
      continue;
  }

  return USBD_OK;
}

static uint8_t  USBD_VendorHID_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_VendorHID_HandleTypeDef *hhid = context;
  unsigned index;
  uint32_t RxLength;
  static uint8_t scratchpad[HID_EP_SIZE];

  for (index = 0; index < NUM_OF_VENDORHID; index++,hhid++)
  {
    if (parameters[index].data_out_ep != epnum)
      continue;

    /* Get the received data length */
    RxLength = USBD_LL_GetRxDataSize (pdev, epnum);

    /* we have to make a temporary copy of the data, as the following PrepareReceive may fail */
    memcpy(scratchpad, hhid->buffer, HID_EP_SIZE);

    if (USBD_OK != USBD_LL_PrepareReceive(pdev, parameters[index].data_out_ep, hhid->buffer, HID_EP_SIZE))
      return USBD_BUSY;

    VendorHID_Callback(pdev, index, scratchpad, RxLength, parameters[index].data_in_ep);
  }

  return USBD_OK;
}

static void USBD_VendorHID_PMAConfig(PCD_HandleTypeDef *hpcd, uint32_t *pma_address)
{
  unsigned index;

  for (index = 0; index < NUM_OF_VENDORHID; index++)
  {
    HAL_PCDEx_PMAConfig(hpcd, parameters[index].data_in_ep, PCD_SNG_BUF, *pma_address);
    *pma_address += HID_EP_SIZE;
    HAL_PCDEx_PMAConfig(hpcd, parameters[index].data_out_ep, PCD_SNG_BUF, *pma_address);
    *pma_address += HID_EP_SIZE;
  }
}
