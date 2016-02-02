/*
 * adc.c - CDB Assist v3 voltage monitor handling
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
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "adc.h"

// Vtarget is considered valid if above 0.89V
#define VTARGETVALID (0.89f / 5.0f * 4096.0f)

int16_t adcvalues[ADCMAX] = { 0, 0, 0, 0 };

// ADC_SAR_1 deals with Dummy load current and Target Vref voltages (ADC Vref=2.5V for 5V full-scale measurements)
// ADC_SAR_2 deals with VBAT, USB2VBUS and USB3VBUS voltages (ADC Vref=1.024V for 2.048V full-scale measurements)

CY_ISR( ADC_SAR_1_ISR ) {
    static uint8_t count = 0;
    static uint32_t accum = 0;
    static uint16_t hi = 0, lo = 0;
    int16_t adcval = CY_GET_REG16(ADC_SAR_1_SAR_WRK_PTR);
    count++;
    if((count & 0x7f) > 2) {
        accum += adcval;
        if(adcval > hi) hi = adcval;
        if(adcval < lo) lo = adcval;
    }
    if((count & 0x7f) > 19) {
        accum -= hi;
        accum -= lo;
        adcvalues[(count >> 7) + DUMMYLOAD] = accum >> 4;
        accum = 0;
        hi = 0;
        lo = 0xffff;
        count ^= (1<<7);
        count &= 0x80;
        AMux_1_FastSelect(count >> 7);
    }
}

CY_ISR( ADC_SAR_2_ISR ) {
    static uint8_t count = 0;
    static uint8_t ch = 0;
    static uint32_t accum = 0;
    static uint16_t hi = 0, lo = 0;
    int16_t adcval = CY_GET_REG16(ADC_SAR_2_SAR_WRK_PTR);
    count++;
    if(count > 2) {
        accum += adcval;
        if(adcval > hi) hi = adcval;
        if(adcval < lo) lo = adcval;
    }
    if(count > 19) {
        accum -= hi;
        accum -= lo;
        adcvalues[ch] = accum >> 4;
        accum = 0;
        hi = 0;
        lo = 0xffff;
        ch++;
        if(ch > 2) ch = 0;
        count = 0;
        AMuxSeq_1_Next();
    }
}

void ADC_Init(void) {
    SenseRefGen_Start(); // Make sure we have our 2.5V ref which results in a 0-5V measurement range
    ADC_SAR_1_Start();
    AMux_1_Start();
    AMux_1_FastSelect(0);
    ADC_SAR_2_Start();
    AMuxSeq_1_Start();
    AMuxSeq_1_Next();
    ADC_SAR_1_IRQ_Enable();
    ADC_SAR_1_StartConvert();
    ADC_SAR_2_IRQ_Enable();
    ADC_SAR_2_StartConvert();
}

void ADC_Work( void ) {
//    for( uint8_t i=0; i<ADCMAX; i++ ) {
//        adcvalues[i] = ADC_SAR_Seq_1_GetResult16(i);
//    }
}

float ADC_GetVoltage( ADC_type sel ) {
    float retval = -1;
    if( sel < ADCMAX ) {
        float tmp = (float)adcvalues[sel];
        if( sel == VTARGETSENSE ) {
            tmp *= (5.0f / 4096.0f); // Convert to actual Volts
        } else {
            tmp *= (22.528f / 4096.0f); // Convert to actual Volts
        }
        retval = tmp;
    }
    return retval;
}

uint16_t ADC_GetMillivolt( ADC_type sel ) {
    uint16_t retval = 0;
    if( sel < ADCMAX ) {
        uint32_t tmp = adcvalues[sel];
        if( sel == VTARGETSENSE ) {
            tmp *= 625; // Convert to actual millivolts
            tmp >>= 9; // Net result: Multiply by 5000, divide by 4096 (same as multiply by 625, divide by 512)
        } else { // 22528 mv full-scale when using 2.048V ref and 1:11 dividers
            tmp *= 2816; // Convert to actual millivolts
            tmp >>= 9; // Net result: Multiply by 22528, divide by 4096 (same as multiply by 2816, divide by 512)
        }
        retval = tmp;
    }
    return retval;
}

uint16_t ADC_GetRaw( ADC_type sel ) {
    uint16_t retval = 0;
    if( sel < ADCMAX ) {
        retval = adcvalues[sel];
    }
    return retval;
}

uint8_t ADC_VtargetValid( void ) {
    return adcvalues[VTARGETSENSE] > VTARGETVALID;
}
