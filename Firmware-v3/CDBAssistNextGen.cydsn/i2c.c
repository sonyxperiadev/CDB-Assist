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
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    setup[0] = ISL28023_CONFIG_VCHANNEL;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, setup, 3, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    
    // 25 mOhm sense resistor gives 3200mA full-scale readout with the ISL chips
    islvbus_curgain = (-3200.0f/32768.0f);
    islvbus_curgain *= (1 + ((float)(NVREAD->vbuscur_islgainadj) * 0.001f)); // Adjust +/- 10% with a +/-100 adjust value
    
    // 25 mOhm sense resistor gives 3200mA full-scale readout with the ISL chips
    islvbat_curgain = (-3200.0f/32768.0f);
    islvbat_curgain *= (1 + ((float)(NVREAD->vbatcur_islgainadj) * 0.001f)); // Adjust +/- 10% with a +/-100 adjust value
}

float i2c_vbus_volt,i2c_vbus_cur,i2c_vbat_volt,i2c_vbat_cur,i2c_temp;

float I2C_Get_VBUS_Volt( void ) {
    return i2c_vbus_volt;
}

float I2C_Get_VBUS_Cur( void ) {
    return i2c_vbus_cur;
}

float I2C_Get_VBAT_Volt( void ) {
    return i2c_vbat_volt;
}

float I2C_Get_VBAT_Cur( void ) {
    return i2c_vbat_cur;
}

float I2C_Get_Temperature( void ) {
    return i2c_temp;
}

void I2C_Work( void ) {
    uint8_t tmpbuf[2];
    int voltage;
    int16_t temp;
    
    tmpbuf[0] = ISL28023_READ_VOUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    voltage = (tmpbuf[0] << 8) | tmpbuf[1];
    i2c_vbus_volt = (float)voltage * 0.001f;

    tmpbuf[0] = ISL28023_READ_VSHUNT_OUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBUS, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    temp = (tmpbuf[0] << 8) | tmpbuf[1];
    temp += (int16_t)(NVREAD->vbuscur_isloffsetadj);
    i2c_vbus_cur = (float)temp * islvbus_curgain;

    IntI2C_MasterReadBuf(I2CADDR_LM75, tmpbuf, 2, IntI2C_MODE_COMPLETE_XFER);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    temp = (tmpbuf[0] << 8) | tmpbuf[1];
    i2c_temp = (float)temp * (1.0f/256.0f); // Divide by 256 to get temp in degrees C

    tmpbuf[0] = ISL28023_READ_VOUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    voltage = (tmpbuf[0] << 8) | tmpbuf[1];
    i2c_vbat_volt = (float)voltage * 0.001f;

    tmpbuf[0] = ISL28023_READ_VSHUNT_OUT;
    IntI2C_MasterWriteBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 1, IntI2C_MODE_NO_STOP);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_WR_CMPLT));
    IntI2C_MasterReadBuf(I2CADDR_ISL28023_VBAT, tmpbuf, 2, IntI2C_MODE_REPEAT_START);
    while(!(IntI2C_MasterStatus() & IntI2C_MSTAT_RD_CMPLT));
    temp = (tmpbuf[0] << 8) | tmpbuf[1];
    temp += (int16_t)(NVREAD->vbatcur_isloffsetadj);
    i2c_vbat_cur = (float)temp * islvbat_curgain;

    float curcomp = i2c_vbat_volt * (1.0f/110.0f); // Calculate current in to or out of the vsense resistors (110k, result in mA)
    if( i2c_vbat_cur<0.0f) i2c_vbat_cur+=curcomp; else i2c_vbat_cur-=curcomp;
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
