/*
 * pwm.c - PWM Buck/Boost power supply support
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
#include "pwm.h"
#include "PID_v1.h"

float pwmsetvolt,pwmsetcur,pwmactvolt,pwmactcur;
int16_t curpwm;
// 1ms between each run
#define PID_TIMEBASE (1)
uint8_t supplykill;

PidType PID;

void PWM_Init( void ) {
    CY_SET_REG8(DitherPWM_1_pwmdp_u0__DP_AUX_CTL_REG, 1); // FIFO clear for BuckBoost DitherPWM
    CY_SET_REG8(DitherPWM_1_pwmdp_u0__F0_REG, 0xff); // Period set to 0xff+1 (full 8-bit)
    PWM_SYNC_Start();
    
    pwmsetvolt = 0.0f;
    supplykill = 1;
    
    PID_init(&PID, 0, 0, 0, PID_Direction_Direct); // Can't supply tuning to PID_Init when not using the default timebase
	PID_SetSampleTime(&PID, PID_TIMEBASE);

    // Tuning:
    // P 1700 starts to oscillate, so set P = 1700/4 = 425
    
    PID_SetTunings(&PID, 425.0, 200.0, 0.0); // Adjusted values to compensate for the incorrect timebase earlier

	PID.mySetpoint = pwmsetvolt;
//	PID_SetOutputLimits(&PID, 0, 1023); // Buck only
	PID_SetOutputLimits(&PID, 0, 1800); // For Buck/Boost
	PID_SetMode(&PID, PID_Mode_Manual);
	PID.myOutput = 0; // No output
	PID_SetMode(&PID, PID_Mode_Automatic);

}

uint16_t PWM_GetVoltage(void) {
    uint16_t retval = 0;
    if(pwmactvolt > 0.0f) retval = (uint16_t)(pwmactvolt*1000.0f);
    return retval;
}

float PWM_GetCurrent(void) {
    return pwmactcur;
}

void PWM_Setpoints(uint16_t volt,uint16_t cur, uint8_t range) {
    // range is unused in v3 design
    pwmsetvolt = (float)volt;
    pwmsetvolt*=0.001f;
    pwmsetcur = (float)cur;
	PID.mySetpoint = pwmsetvolt;
    if(volt) {
        supplykill = 0;
    } else {
        supplykill = 1;
    }
}

int16_t pwmoverride = -1;
void PWM_Override( int16_t rawpwm ) {
    pwmoverride = rawpwm;
    supplykill = (pwmoverride > 0) ? 0 : 1;
}

int16_t PWM_GetOverride( void ) {
    return pwmoverride;
}

void PWM_Work(float actualvolt, float actualcur) {
    static uint8_t isboost = 0;
    pwmactvolt = actualvolt;
    pwmactcur = actualcur;

//#define PWM_PID
#ifdef PWM_PID
    PID.myInput = pwmactvolt;
	PID_Compute(&PID);
	curpwm = PID.myOutput;
#else
    // Simple non-PID regulation
    if(pwmactvolt<pwmsetvolt && pwmactcur<pwmsetcur) {
        curpwm++;
        if(curpwm>1800) curpwm=1800;
    } else if(pwmactvolt>pwmsetvolt || pwmactcur>pwmsetcur) {
        curpwm--;
        if(curpwm<0) curpwm=0;
    }
#endif

    if(pwmoverride >= 0) curpwm = pwmoverride;
    
    uint8_t actualpwm;
    if(curpwm >= 1024) { // Boost
        isboost = 1;
        actualpwm = ((curpwm - 1024) >> 2);
    } else {
        isboost = 0;
        actualpwm = (curpwm >> 2);
    }
    
    // Must not clash with main switch! (watch the deadtime!)
    // Rudimentary protection exists in form of an AND gate but don't push it...
    // It's important not to set the sync time too long either at light loads (especially for boost-mode)
    // as reverse current flow will occur otherwise. No-load value seems to be around 24 cycles for boost
    #define DEADTIME (16)
    int16_t actualsync = 255 - actualpwm;
    actualsync -= DEADTIME;
    if(isboost) actualsync = 0; // No sync rectifier for boost at the moment, need dynamic leading edge delay

    // Enable in bit 7 and lowest 2 bits of PWM (dither) in bit 0 & 1
    CY_SET_REG8(DitherPWM_1_ctrlreg__CONTROL_REG, 0x80 | (curpwm & 0x03));
    // Remaining 8 bits
    CY_SET_REG8(DitherPWM_1_pwmdp_u0__D0_REG, actualpwm);

    // Sanity check before handing it to the PWM
    if( actualsync < 0 || actualsync > 255 ) actualsync = 0;
    // Compare 255 means no sync rectification, 0 means 255 cycles sync
    // The reason for the inverted behavior is because of the triggered oneshot PWM used 
    PWM_SYNC_WriteCompare(255 - actualsync);
    
    if(supplykill) {
        CY_SET_REG8( BUCK_LS_0, (CY_PINS_DM_STRONG) ); // Buck LS forced off
        CY_SET_REG8( BOOST_HS_0, (CY_PINS_DM_STRONG) ); // Boost HS forced off
        CY_SET_REG8( BUCK_HS_0, (CY_PINS_DM_STRONG) ); // Buck HS forced off
        CY_SET_REG8( BOOST_LS_0, (CY_PINS_DM_STRONG) ); // Boost LS forced off
    } else if(isboost) {
        CY_SET_REG8( BUCK_LS_0, (CY_PINS_DM_STRONG) ); // Buck LS forced off
        CY_SET_REG8( BOOST_HS_0, (CY_PINS_PC_BYPASS | CY_PINS_DM_STRONG) ); // Boost HS as sync rectifier
        CY_SET_REG8( BUCK_HS_0, (CY_PINS_PC_DATAOUT | CY_PINS_DM_STRONG) ); // Buck HS forced on
        CY_SET_REG8( BOOST_LS_0, (CY_PINS_PC_BYPASS | CY_PINS_DM_STRONG) ); // Boost LS as main PWM switch
    } else {
        CY_SET_REG8( BUCK_HS_0, (CY_PINS_PC_BYPASS | CY_PINS_DM_STRONG) ); // Buck HS as main PWM switch
        CY_SET_REG8( BUCK_LS_0, (CY_PINS_PC_BYPASS | CY_PINS_DM_STRONG) ); // Buck LS as sync rectifier
        CY_SET_REG8( BOOST_HS_0, (CY_PINS_PC_DATAOUT | CY_PINS_DM_STRONG) ); // Boost HS forced on
        CY_SET_REG8( BOOST_LS_0, (CY_PINS_DM_STRONG) ); // Boost LS forced off
    }    
}
