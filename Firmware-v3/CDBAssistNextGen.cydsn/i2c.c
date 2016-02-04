/*
 * i2c.c - I2C handling for CDB Assist v3
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
#include "i2c.h"
#include "nv.h"

float islvbus_curgain;
float islvbat_curgain;

float I2C_GetCurGain(int8_t gainadj) {
    // 25 mOhm sense resistor gives 3200mA full-scale readout with the ISL chips
    float retval = (-3200.0f/32768.0f);
    retval *= (1 + ((float)(gainadj) * 0.001f)); // Adjust +/- 10% with a +/-100 adjust value
    return retval;
}

void I2C_CalcGain(void) {
    islvbus_curgain = I2C_GetCurGain(NVREAD->vbuscur_islgainadj);
    islvbat_curgain = I2C_GetCurGain(NVREAD->vbatcur_islgainadj);
}

void I2C_Init( void ) {
    IntI2C_Start();
#if 0
    uint8_t idx=8;
    for(uint8_t addr=0;addr<128;addr++) {
        uint8_t status=IntI2C_MasterSendStart(addr,1);
        IntI2C_MasterSendStop();
        if(status==IntI2C_MSTR_NO_ERROR) {
            char buffah[22];
            uint8_t num = snprintf(buffah, sizeof(buffah), "%02x Responding", addr);
            disp_str(buffah, num, 0, idx, FONT6X6);
            idx+=6;
        }
    }
    FB_update();
#endif

    uint8_t setup[3];

    setup[0] = ISL28023_CONFIG_ICHANNEL;
    setup[1] = 0;
//    setup[2] = 0b00111111; // 128 avg 2.048ms conversion time
//    setup[2] = 0b00111010; // 128 avg 256us conversion time
//    setup[2] = 0b00010000; // 4 avg * 64us conversion time
//    setup[2] = 0b00001001; // 2 avg * 128us conversion time
    setup[2] = 0b00000010; // 1 avg * 256us conversion time
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    setup[2] = 0b00111111; // 128 avg 2.048ms conversion time
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    setup[0] = ISL28023_CONFIG_VCHANNEL;
    setup[2] = 0b00000010; // 1 avg * 256us conversion time
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    setup[2] = 0b00111111; // 128 avg 2.048ms conversion time
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    
    I2C_CalcGain();
}

uint16_t i2c_vbus_volt,i2c_vbat_volt,i2c_vbat_volt_avg;
int16_t i2c_vbus_cur,i2c_vbat_cur,i2c_temp,i2c_vbat_cur_avg;
uint32_t i2c_vbat_volt_accum = 0;
int32_t i2c_vbat_cur_accum = 0;
uint16_t i2c_accumcnt = 0;

int16_t I2C_Get_VBUS_Volt( void ) {
    return i2c_vbus_volt;
}

float I2C_Get_VBUS_Cur( void ) {
    float temp = (float)(i2c_vbus_cur + (int16_t)(NVREAD->vbuscur_isloffsetadj));
    return temp * islvbus_curgain;
}

int16_t I2C_Get_VBAT_Volt( void ) {
    return i2c_vbat_volt;
}

int16_t I2C_Get_VBAT_VoltAvg( void ) {
    return i2c_vbat_volt_avg;
}

float I2C_Get_VBAT_CurAvg( void ) {
    float temp = (float)(i2c_vbat_cur_avg + (int16_t)(NVREAD->vbatcur_isloffsetadj));
    temp *= islvbat_curgain;

    //float curcomp = i2c_vbat_volt * (0.001f/110.0f); // Calculate current in to or out of the vsense resistors (110k, result in mA)
    //if( temp<0.0f) temp+=curcomp; else temp-=curcomp;

    return temp;
}

int16_t I2C_Get_VBAT_CurRaw( void ) {
    return i2c_vbat_cur;
}

int16_t I2C_Get_VBAT_CurRawAvg( void ) {
    return i2c_vbat_cur_avg;
}

float I2C_Get_Temperature( void ) {
    return (float)i2c_temp * (1.0f/256.0f); // Divide by 256 to get temp in degrees C
}

void I2C_Work( void ) {
    uint8_t tmpbuf[2];
    
    tmpbuf[0] = ISL28023_READ_VOUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    i2c_vbus_volt = (tmpbuf[0] << 8) | tmpbuf[1];

    tmpbuf[0] = ISL28023_READ_VSHUNT_OUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    i2c_vbus_cur = (tmpbuf[0] << 8) | tmpbuf[1];
    
    IntI2C_MasterReadBuf(I2CADDR_LM75, tmpbuf, 2, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    i2c_temp = (tmpbuf[0] << 8) | tmpbuf[1];

    tmpbuf[0] = ISL28023_READ_VOUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    i2c_vbat_volt = (tmpbuf[0] << 8) | tmpbuf[1];

    tmpbuf[0] = ISL28023_READ_VSHUNT_OUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 1, IntI2C_MODE_NO_STOP);

    i2c_vbat_volt_accum += i2c_vbat_volt;

    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    i2c_vbat_cur = (tmpbuf[0] << 8) | tmpbuf[1];

    i2c_vbat_cur_accum += i2c_vbat_cur;
    i2c_accumcnt++;
    if(i2c_accumcnt >= 256) {
        i2c_vbat_cur_avg = i2c_vbat_cur_accum >> 8;
        i2c_vbat_volt_avg = i2c_vbat_volt_accum >> 8;
        i2c_vbat_cur_accum = 0;
        i2c_vbat_volt_accum = 0;
        i2c_accumcnt = 0;
    }
#if 0
    tmpbuf[0] = ISL28023_READ_IOUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    temp = (tmpbuf[0] << 8) | tmpbuf[1];
    float realiout = (float)temp * (1.0f/1.0f);
#endif
}
