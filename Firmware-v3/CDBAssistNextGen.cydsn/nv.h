#ifndef NV_H_
#define NV_H_

#include <stdint.h>

typedef struct __attribute__ ((__packed__)) eeinfo {
    uint8_t cdbversion;
    int8_t vbuscur_islgainadj;
    int8_t vbuscur_isloffsetadj;
    int8_t vbatcur_islgainadj;
    int8_t vbatcur_isloffsetadj;
    int8_t vbatvolt_adcgainadj;
    int8_t vbatvolt_adcoffsetadj;
    int8_t dummyload_dacgainadj;
    int8_t dummyload_dacoffsetadj;
    int8_t dummyload_adcgainadj;
    int8_t dummyload_adcoffsetadj;    
    uint8_t calpadding[5]; // Space for more calibration data
    char USBserial[9]; // Serial number should start at an even row (byte 16)
    uint8_t serialpadding[7]; // To make nothing but serial number stored in the row
} EE_t;

// For direct read-only access of the EE after it has been started
#define NVREAD ((EE_t*)(CYDEV_EE_BASE))

void NV_Init( void );
    
#endif // NV_H_
