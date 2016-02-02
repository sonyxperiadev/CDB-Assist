/*
 * nv.c - CDB Assist v3 persistant storage handling
 *
 * Copyright (C) 2016 Sony Mobile Communications Inc.
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

#include "nv.h"

void NV_Init( void ) {
    EEPROM_1_Start();

    if(NVREAD->cdbversion == 0 || NVREAD->cdbversion == 0xff) { // No valid cal record found, populate a default
        uint8_t tmpbuf[16];
        EE_t* ee = (EE_t*)tmpbuf;
        ee->cdbversion = 0x31;
        ee->vbuscur_islgainadj = 0;
        ee->vbuscur_isloffsetadj = 0;
        ee->vbatcur_islgainadj = 0;
        ee->vbatcur_isloffsetadj = 0;
        ee->vbatvolt_adcgainadj = 0;
        ee->vbatvolt_adcoffsetadj = 0;
        ee->dummyload_dacgainadj = 0;
        ee->dummyload_dacoffsetadj = 0;
        ee->dummyload_adcgainadj = 0;
        ee->dummyload_adcoffsetadj = 0;
        ee->calpadding[0] = 0;
        ee->calpadding[1] = 0;
        ee->calpadding[2] = 0;
        ee->calpadding[3] = 0;
        ee->calpadding[4] = 0;
        EEPROM_1_UpdateTemperature();
        EEPROM_1_Write(tmpbuf, 0); // Row 0
    }
//#define WRITECAL
#ifdef WRITECAL
    uint8_t tmpbuf[16];
    memcpy(tmpbuf, NVREAD, 16);
    EE_t* ee = (EE_t*)tmpbuf;
//    ee->vbuscur_islgainadj = 0;
//    ee->vbuscur_isloffsetadj = 0;
//    ee->vbatcur_islgainadj = 0;
//    ee->vbatcur_isloffsetadj = 0;
//    ee->vbatvolt_adcgainadj = 0;
//    ee->vbatvolt_adcoffsetadj = 0;
//    ee->dummyload_dacgainadj = 0;
//    ee->dummyload_dacoffsetadj = 0;
//    ee->dummyload_adcgainadj = 0;
//    ee->dummyload_adcoffsetadj = 0;
    EEPROM_1_UpdateTemperature();
    EEPROM_1_Write(tmpbuf, 0); // Row 0
#endif

//#define WRITESERIAL
#ifdef WRITESERIAL
    char* serial = "00000322";
    EEPROM_1_UpdateTemperature();
    EEPROM_1_Write((uint8_t*)serial, 1); // Row 1
#endif
}
/*
Calibration board:
        ee->vbatcur_islgainadj = 20;
        ee->vbatcur_isloffsetadj = 0;
        ee->dummyload_dacgainadj = 9;
        ee->dummyload_dacoffsetadj = 24;
        ee->dummyload_adcgainadj = -45;
        ee->dummyload_adcoffsetadj = 10;
*/