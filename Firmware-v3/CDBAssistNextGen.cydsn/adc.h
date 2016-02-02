#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

typedef enum {//vbat otg chg (adc2) dummyload vtarget (adc1)
    VBATSENSE = 0,
    USB2SENSE, // "OTG"
    USB3SENSE, // "CHG"
    DUMMYLOAD,
    VTARGETSENSE,
    ADCMAX
} ADC_type;

void ADC_Init( void );
void ADC_Work( void );
float ADC_GetVoltage( ADC_type sel );
uint16_t ADC_GetMillivolt( ADC_type sel );
uint16_t ADC_GetRaw( ADC_type sel );
uint8_t ADC_VtargetValid( void );

#endif /* ADC_H_ */
