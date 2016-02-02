#ifndef USB_H_
#define USB_H_

#include <stdint.h>

#define HID_IN_EP (USBFS_1_EP1) // 0
#define CONTROLMGMT_EP (USBFS_1_EP2) // 0x40
#define CONTROLDATA_IN_EP (USBFS_1_EP3) // 0x80
#define CONTROLDATA_OUT_EP (USBFS_1_EP8) // 0x180
#define TARGETMGMT_EP (USBFS_1_EP5) // 0xc0
#define TARGETDATA_IN_EP (USBFS_1_EP6) // 0x100
#define TARGETDATA_OUT_EP (USBFS_1_EP7) // 0x140

#define CONTROLMGMT_IF (2)
#define CONTROLDATA_IF (3)
#define TARGETMGMT_IF (4)
#define TARGETDATA_IF (5)

uint8_t USBFS_1_DispatchClassRqst(void);
void USB_Init(void);
void USBQueueFrame(uint8_t ep,uint8_t* buf,uint8_t len);
void USB_Work(void);

#endif // USB_H_
