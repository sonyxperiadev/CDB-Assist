/*
 * dummyload.c - Dummy load handling
 *
 * Copyright (C) 2015-2016 Sony Mobile Communications Inc.
 * Author: Werner Johansson <werner.johansson@sonymobile.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <project.h>
#include <stdint.h>
#include <stdio.h>
#include "dummyload.h"
#include "i2c.h"
#include "adc.h"
#include "nv.h"

float dummyload_minvolt,dummyload_cur,dummyload_temp;
uint8_t dummyload_maxdac,dummyload_curdac;
float dummyload_dacgain;
float dummyload_adcgain;

void DummyLoad_Init( void ) {
    IDAC8_1_Start();
    IDAC8_1_SetRange(IDAC8_1_RANGE_32uA);
    IDAC8_1_SetValue(0);
    DummyLoadDriver_Start();
    PGA_1_Start();
    
    dummyload_minvolt = 0.0f;
    dummyload_maxdac = dummyload_curdac = 0;
    
    // With a 68R resistor to ground from the CurSet pin the gain should have
    // been divided by 2785(mA) full-scale, but due to the internal resistance
    // in the ANAIF_RT_DAC3_SW3 switch connecting the pin to the bus the value
    // is much higher (resistance in the switch is around 50R, so 68R+50R
    // gives 2.048mA * (68R+50R) / 50mR sense resistor ~ 4833)
    dummyload_dacgain = (256.0f*64.0f/4833.28f);
    dummyload_dacgain *= (1 + ((float)(NVREAD->dummyload_dacgainadj) * 0.001f)); // Adjust +/- 10% with a +/-100 adjust value
    
    // ADC readout * 5000mV / 4096 / (50mOhm * 32 PGA gain), result in mA * gain adj
    // Main sources of error here are the PGA gain and sense resistor accuracy
    dummyload_adcgain = ((5000.0f/4096.0f)/(0.050f * 32));
    dummyload_adcgain *= (1 + ((float)(NVREAD->dummyload_adcgainadj) * 0.001f)); // Adjust +/- 10% with a +/-100 adjust value    
}

void DummyLoad_Setpoints(uint16_t minvolt, uint16_t maxcur) {
    dummyload_minvolt = (float)minvolt;
    dummyload_minvolt*=0.001f;
    dummyload_minvolt+=0.010f;
    
    float tmp = (float)maxcur;
    tmp *= dummyload_dacgain; // Apply cal gain here
    tmp += 0.5f; // Rounding
    int16_t dacval = (uint16_t)tmp;
    dacval += (int16_t)(NVREAD->dummyload_dacoffsetadj);
    if( dacval  < 0 ) dacval = 0;
    
    IDAC8_1_SetValue(0);

    if(dacval > 2047) {
        dacval += 32; // Rounding
        dacval >>= 6; // Divide by 64 and choose 2mA range
        IDAC8_1_SetRange(IDAC8_1_RANGE_2mA);
    } else if(dacval > 255) {
        dacval += 4; // Rounding
        dacval >>= 3; // Divide by 8 and choose 255uA range
        IDAC8_1_SetRange(IDAC8_1_RANGE_255uA);
    } else {
        IDAC8_1_SetRange(IDAC8_1_RANGE_32uA); // Lowest range
    }

    dummyload_curdac = dummyload_maxdac = (uint8_t)dacval;
    IDAC8_1_SetValue(dummyload_curdac);
}

void DummyLoad_Work(float actualvolt) {
    if(actualvolt<dummyload_minvolt) {
        if(dummyload_curdac>0) {
            dummyload_curdac--;
            IDAC8_1_SetValue(dummyload_curdac);
        }
    } else {
        if(dummyload_curdac<dummyload_maxdac) {
            dummyload_curdac++;
            IDAC8_1_SetValue(dummyload_curdac);
        }
    }
}

void DummyLoad_ADCWork(void) {
    int16_t tmp = ADC_GetRaw(DUMMYLOAD);
    tmp += (int16_t)(NVREAD->dummyload_adcoffsetadj);
    dummyload_cur = (float)tmp;
    dummyload_cur *= dummyload_adcgain;

    dummyload_temp = I2C_Get_Temperature();

    if(dummyload_temp > 60.0f) {
        CyPins_SetPin(FanEna_0);
    } else {
        CyPins_ClearPin(FanEna_0);
    }
}

float DummyLoad_GetCur(void) {
    return dummyload_cur;
}

float DummyLoad_GetTemp(void) {
    return dummyload_temp;
}
