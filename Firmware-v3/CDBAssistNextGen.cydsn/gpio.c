/*
 * gpio.c - GPIO/UART muxing for CDB Assist v3
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
#include "gpio.h"

Pins_t curtx = NO_PIN;
uint8_t curtxdrive = 0;
reg8* portpcs[NUM_PINS] = {NULL, (reg8*)BtnA_Pin1b_0, (reg8*)BtnA_Pin2b_0, (reg8*)BtnB_Pin3b_0, (reg8*)BtnB_Pin4b_0, (reg8*)BtnC_Pin5b_0, (reg8*)BtnC_Pin6b_0,
                                (reg8*)VtargetSense_0, (reg8*)DFMS_0, (reg8*)DTMS_0, (reg8*)DUTUSBDPmux_0, (reg8*)DUTUSBDNmux_0, (reg8*)DUTUSBID_0, (reg8*)SpareSIO_0};

void GPIO_Init() {
    // Disconnect analog switches (BtnA, B and C)
    // BtnA is controlled by bits 7+3 in both regs
    // BtnB is controlled by bits 6+2 in both regs
    // BtnC is controlled by bits 7+1 in both regs
    CY_SET_REG8(CYREG_PRT2_AG, 0b00000000);
    CY_SET_REG8(CYREG_PRT6_AG, 0b00000000);
}

static uint8_t BtnBitmask(uint8_t idx) {
    uint16_t bitmask = 0;
    if( idx>=1 && idx <=3 ) {
        bitmask = 0b100010000;
        bitmask >>= idx;
    }
    return (uint8_t)bitmask;
}

void Buttons_Enable(uint8_t idx) {
    uint8_t tmp = CY_GET_REG8(CYREG_PRT2_AG);
    tmp |= BtnBitmask( idx );
    CY_SET_REG8(CYREG_PRT2_AG, tmp);
    CY_SET_REG8(CYREG_PRT6_AG, tmp);    
}

void Buttons_Disable(uint8_t idx) {
    uint8_t tmp = CY_GET_REG8(CYREG_PRT2_AG);
    tmp &= ~BtnBitmask( idx );
    CY_SET_REG8(CYREG_PRT2_AG, tmp);
    CY_SET_REG8(CYREG_PRT6_AG, tmp);    
}

// Pins are mapped like this:
// 1 = P6 bit 3 (first pin of BtnA)
// 2 = P6 bit 7 (second pin of BtnA)
// 3 = P6 bit 2 (first pin of BtnB)
// 4 = P6 bit 6 (second part of BtnB)
// 5 = P6 bit 1 (first part of BtnC)
// 6 = P6 bit 5 (second part of BtnC)
// 7 = P15 bit 3 (tgt sense) (only rx at this point due to psoc creator bug)
// 8 = P12 bit 4 (dfms)
// 9 = P12 bit 5 (dtms)
// 10 = P12 bit 1 (usb dp)
// 11 = P12 bit 0 (usb dm)
// 12 = P12 bit 3 (usb id)
// 13 = P12 bit 2 (spare SIO, testpoint)
// Pins 1-9 are mapped 1:1 to physical pins 1-9 in the 10-pin header (pin 10 is fixed GND)

void GPIO_SetPinState( Pins_t id, PinStates_t state ) {
    if( id >= NUM_PINS || state >= PIN_NUM_STATES ) return;

    uint8_t conf;

    switch( state ) {
        case PIN_UART_RX:
            // Select the desired rx pin using the mux
            UART_Control_Reg_Write(id & 0x0f);
            // Intentional fall-through here
        case PIN_HIZ_INPUT:
            conf = CY_PINS_DM_DIG_HIZ;
            break;
        case PIN_OUT_HIGH:
            conf = (CY_PINS_PC_DATAOUT | CY_PINS_DM_STRONG);
            break;
        case PIN_OUT_LOW:
            conf = CY_PINS_DM_STRONG;
            break;
        case PIN_UART_TX:
            // First disconnect old pin
            GPIO_SetPinState( curtx, PIN_HIZ_INPUT );
            conf = curtxdrive;
            curtx = id;
            break;
        default:
            id = NO_PIN;
    }
    if( portpcs[id] ) *portpcs[id] = conf;
}

void GPIO_SetTXDrive(uint8_t onoff) {
    uint8_t drive;
    if( onoff ) {
        drive = (CY_PINS_PC_BYPASS | CY_PINS_DM_STRONG);
    } else {
        drive = (CY_PINS_PC_BYPASS | CY_PINS_DM_OD_LO);
    }
    if( drive != curtxdrive ) {
        curtxdrive = drive;
        GPIO_SetPinState( curtx, PIN_UART_TX ); // Force reconfig
    }        
}
