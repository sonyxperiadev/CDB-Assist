#ifndef USBMUX_H_
#define USBMUX_H_

#include <stdint.h>

void USBMux_Init( void );
void USBMux_UpdateMeasurements(float vbusvolt, float vbuscur);
float USBMux_GetVBUSVoltage(void);
float USBMux_GetVBUSCurrent(void);
void USBMux_SelPort( uint8_t port );

#endif // USBMUX_H_
