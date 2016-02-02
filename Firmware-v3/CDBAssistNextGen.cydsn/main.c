/*
 * main.c - CDB Assist v3
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
#include <lcd.h>
#include "i2c.h"
#include "usb.h"
#include "usbmux.h"
#include "dummyload.h"
#include "pwm.h"
#include "gpio.h"
#include "AppCallbacks.h"
#include "debuguart.h"
#include "targetuart.h"
#include "adc.h"
#include "nv.h"
#include <stdio.h>

// Special calibration edition
//#define SPECIAL_EDITION
uint8_t mySerial[]= {0x12,USBFS_1_DESCR_STRING,'x',0,'x',0,'x',0,'x',0,'x',0,'x',0,'x',0,'x',0}; // Dummy serial number until populated from EE

int main()
{
// This must be more dynamic in the future, but right now this IOREF is only used for the analog switches in the 10-pin connector
//  (UART pins are handled by SIORefGen, and that voltage has to be lower than this IORef voltage)
//#define DYN_IOREF
#ifdef DYN_IOREF
    // ~1.0V to 4.1V vref
    IOVoltGen_Start();
    IOVoltGen_SetValue(0x80);
    IOVoltBuffer_Start();
#else
    // Results in 5V vref
    VDDIO2ref_SetDriveMode(VDDIO2ref_DM_STRONG);
    VDDIO2ref_Write(1);
#endif
    NV_Init();
    
    SIORefGen_Start();
    SIORefGen_SetValue(62); // 1V
    GPIO_SetTXDrive(0);
    debuguart1_init();
    
    CyGlobalIntEnable; /* Uncomment this line to enable global interrupts. */
    
    CyPins_ClearPin(LED_Status_0);
    CyPins_SetPin(FanEna_0);
  
    if(NVREAD->USBserial[0] != 0 && NVREAD->USBserial[0] != 0xff) {
		for(uint8_t loopah=0; loopah<8; loopah++) {
			mySerial[2 + (loopah<<1)] = NVREAD->USBserial[loopah]; // Update USB descriptor Unicode values with our ASCII
		}
        USBFS_1_SerialNumString(mySerial);
	}

    USB_Init();
    CyDelay(500);
    CyPins_ClearPin(FanEna_0);
    targetuart_init();
#ifdef SPECIAL_EDITION
    // Note that VtargetRef is in the 5V quadrant so that pin will always
    // output 5V when set high, no matter what the IOREF is set to!
    GPIO_SetPinState( TREF_PIN7, PIN_OUT_HIGH ); // Output on Vref
    GPIO_SetPinState( DFMS_PIN8, PIN_UART_TX ); // Send on DFMS pin, recv on DTMS!
    GPIO_SetPinState( DTMS_PIN9, PIN_UART_RX );
#else
    GPIO_SetPinState( DTMS_PIN9, PIN_UART_TX ); // Send on DTMS pin, recv on DFMS!
    GPIO_SetPinState( DFMS_PIN8, PIN_UART_RX );
#endif
    printf("\n\nWJ CDB Assist v3 controller init\n");
    
    FB_init();
    FB_clear();
    disp_str("CDB ASSIST V3",13,(64-39),0,FONT6X6 | INVERT);
    disp_str("Sony Mobile",11,(64-33),64-12,FONT6X6);
#ifdef SPECIAL_EDITION
    disp_str("Calibration Edition",19,(64-57),64-6,FONT6X6);
#else
    disp_str("Open Devices",12,(64-36),64-6,FONT6X6);
#endif
    FB_update();
    
    GPIO_Init();
    DummyLoad_Init();
    USBMux_Init();
    I2C_Init();
    PWM_Init();
    ADC_Init();
    
    uint16_t ctr=0;
    
    for(;;)
    {
        char buffah[22];
        uint8_t num;
        float vbatvolt,vbatcur,vbusvolt,vbuscur;
        
        I2C_Work();
        vbatvolt=I2C_Get_VBAT_Volt();
        vbatcur=I2C_Get_VBAT_Cur();

        PWM_Work(vbatvolt,vbatcur);
        DummyLoad_Work(vbatvolt);
        
        ADC_Work();
        uint8_t vrefok = ADC_VtargetValid();
        static uint8_t oldvrefok = 2;
        
        if( vrefok != oldvrefok ) {
            if( vrefok ) {
                CyPins_ClearPin(LED_Vref_0);
                GPIO_SetTXDrive( 1 );
            } else {
                CyPins_SetPin(LED_Vref_0);
                GPIO_SetTXDrive( 0 );
                SIORefGen_SetValue(62); // Default 1V ref
            }
            oldvrefok = vrefok;
        }

        if(ctr == 0) {
            vbusvolt=I2C_Get_VBUS_Volt();
            vbuscur=I2C_Get_VBUS_Cur();
            USBMux_UpdateMeasurements(vbusvolt,vbuscur);
            
            num = snprintf(buffah, sizeof(buffah), "VBAT %5.2fV %6.1fmA", vbatvolt,vbatcur);
            disp_str(buffah, num, 0, 8, FONT6X6);

            num = snprintf(buffah, sizeof(buffah), "VBUS %5.2fV %6.1fmA", vbusvolt,vbuscur);
            disp_str(buffah, num, 0, 8+6, FONT6X6);

            DummyLoad_ADCWork();
            float loadcur = DummyLoad_GetCur();
            float loadtemp = DummyLoad_GetTemp();
            num = snprintf(buffah, sizeof(buffah), "Load %5.1f` %6.1fmA", loadtemp,loadcur);
            disp_str(buffah, num, 0, 8+12, FONT6X6);

            float tmp = ADC_GetVoltage(VBATSENSE);
            num = snprintf(buffah, sizeof(buffah), "VBAT %5.2fV", tmp);
            disp_str(buffah, num, 0, 8+18, FONT6X6);

            tmp = ADC_GetVoltage(USB2SENSE);
            num = snprintf(buffah, sizeof(buffah), "USB2 %5.2fV", tmp);
            disp_str(buffah, num, 0, 8+24, FONT6X6);

            tmp = ADC_GetVoltage(USB3SENSE);
            num = snprintf(buffah, sizeof(buffah), "USB3 %5.2fV", tmp);
            disp_str(buffah, num, 0, 8+30, FONT6X6);

            tmp = ADC_GetVoltage(VTARGETSENSE);
            if( tmp > 4.5f ) { // Assume 5V, disable regulated output
                CY_SET_REG8(DTMS__SIO_CFG, (CY_GET_REG8(DTMS__SIO_CFG) & 0xcf) | 0x20);
                CY_SET_REG8(DTMS__SIO_DIFF, (CY_GET_REG8(DTMS__SIO_DIFF) & 0xcf) | 0x00);
                CY_SET_REG8(DTMS__SIO_HYST_EN, (CY_GET_REG8(DTMS__SIO_HYST_EN) & 0xcf) | 0x00);
            } else if( tmp > 0.89f ) {
                float val = tmp * 255.0f / 4.096f;
                if(val > 255.0f) val = 255.0f;
                SIORefGen_SetValue((uint8_t)val);
                CY_SET_REG8(DTMS__SIO_CFG, (CY_GET_REG8(DTMS__SIO_CFG) & 0xcf) | 0x30); // Regulated output buffer
                CY_SET_REG8(DTMS__SIO_DIFF, (CY_GET_REG8(DTMS__SIO_DIFF) & 0xcf) | 0x20);
                CY_SET_REG8(DTMS__SIO_HYST_EN, (CY_GET_REG8(DTMS__SIO_HYST_EN) & 0xcf) | 0x20);
            }
            num = snprintf(buffah, sizeof(buffah), "VTGT %5.2fV", tmp);
            disp_str(buffah, num, 0, 8+36, FONT6X6);

            FB_update();
            UpdateCtrl();
            ctr=100;
        } else {
            ctr--;
        }

        USB_Work();
    }
}
