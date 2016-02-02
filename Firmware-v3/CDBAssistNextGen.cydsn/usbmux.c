/*
 * usbmux.c - USB data/ID and VBUS muxing support for CDB Assist v3
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
#include "usbmux.h"
#include "gpio.h"

float usbmux_vbusvolt, usbmux_vbuscur;

void USBMux_Init( void ) {
    IDSwitcher_Start(); // ch0=idpullup, 1=chg/otg3, ch2=otg2, 3=dut, 4=idcap
    IDSwitcher_Connect(3);
}

void USBMux_UpdateMeasurements(float vbusvolt, float vbuscur) {
    usbmux_vbusvolt = vbusvolt;
    usbmux_vbuscur = vbuscur;
}

float USBMux_GetVBUSVoltage(void) {
    return usbmux_vbusvolt;
}

float USBMux_GetVBUSCurrent(void) {
    return usbmux_vbuscur;
}

void USBMux_SelPort( uint8_t port ) {
    
    CyPins_SetPin(MuxOE_0); // Start by tristating the mux
    CyPins_ClearPin(SSR_USB1_0);
    CyPins_ClearPin(SSR_USB2_0);
    CyPins_ClearPin(SSR_USB3_0);
    CyPins_ClearPin(SSR4_CTRL_0);
    CyPins_ClearPin(MuxSel0_0);
    CyPins_ClearPin(MuxSel1_0);
    IDSwitcher_DisconnectAll();
    IDSwitcher_Connect(3); // Always connect DUT right now
    
    switch( port ) {
        case 1:
            CyPins_SetPin(SSR_USB1_0);
            GPIO_SetPinState( USBID, PIN_OUT_LOW ); // ID pin grounded internally
            CyPins_SetPin(MuxSel0_0); // 0=mcu, 1=USB3, 2=USB2, 3=USB1
            CyPins_SetPin(MuxSel1_0);
            CyPins_ClearPin(MuxOE_0);
            break;

        case 2:
            CyPins_SetPin(SSR_USB2_0);
            GPIO_SetPinState( USBID, PIN_HIZ_INPUT ); // Don't pull ID low
            CyPins_SetPin(MuxSel1_0);
            CyPins_ClearPin(MuxOE_0);
            IDSwitcher_Connect(2);
            break;
            
        case 3:
            CyPins_SetPin(SSR_USB3_0);
            GPIO_SetPinState( USBID, PIN_HIZ_INPUT ); // Don't pull ID low
            CyPins_SetPin(MuxSel0_0);
            CyPins_ClearPin(MuxOE_0);
            IDSwitcher_Connect(1);
            break;
            
        case 4:
            CyPins_SetPin(SSR4_CTRL_0);
            CyPins_ClearPin(MuxOE_0);
            // This should also configure the MCU DP/DM pins and route ID pin somewhere useful
            break;
    }
}
