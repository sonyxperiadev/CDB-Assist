/*
 * AppCallbacks.c - Virtual ACM port "UI" handling
 *
 * Copyright (C) 2013-2015 Werner Johansson, wj@unifiedengineering.se
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
#include "usb.h"
#include "gpio.h"
#include "pwm.h"
#include "dummyload.h"
#include "usbmux.h"
#include "adc.h"
#include "nv.h"
#include "AppCallbacks.h"
#include "i2c.h"

#ifdef ENABLE_TARGETUART
#include "targetuart.h"
#endif

uint8_t result[640];
uint16_t setvoltage=4000; // mV
uint16_t setcurrent=2500; // mA
uint16_t setdummyloadcurrent=500; // mA

uint8_t mode=0;

#define STATUS_BTN1 (1<<0)
#define STATUS_BTN2 (1<<1)
#define STATUS_BTN3 (1<<2)
#define STATUS_BTN4 (1<<3)
#define STATUS_VBUS (1<<4)
#define STATUS_VBAT (1<<5)
#define STATUS_LOAD (1<<6)

static uint8_t PWM_GetVersion(void) {
    return NVREAD->cdbversion;
}

static char* GetSerialPtr(void) {
    char* retval = NVREAD->USBserial;
    if(retval[8] != 0) retval = "Not Set!";
    return retval;
}

// To access software version numbers
extern const uint8 cy_meta_loadable[];
#define BTLDR_VERSION_OFFSET (18)
#define BTLDB_VERSION_OFFSET (22)

typedef enum {
    VER_BOOTLOADER = BTLDR_VERSION_OFFSET,
    VER_FIRMWARE = BTLDB_VERSION_OFFSET
} version_type;

static uint8_t GetVersion( version_type vertype, char* dest, uint8_t maxlen ) {
    uint16_t swver = *(uint16_t*)(&cy_meta_loadable[ vertype ]);
    return snprintf(dest, maxlen, "%u.%x.%x%s", (swver & 0x7f00) >> 8, (swver & 0xf0) >> 4, swver & 0xf, (swver & 0x8000)?"-dirty":"");
}

void ActionHandler(uint8_t* buf, uint8_t len) {
	uint16_t outlen=0;
	uint8_t *rptr=result;
	uint8_t btnnum=0;
	uint8_t on=0;
    uint16_t res;
	static uint16_t accumulator=0;
	static uint8_t digitsleft=0;
	static uint8_t maxdigits=0;
	static uint8_t status=0;
    static uint8_t usbport=2;
	for(uint8_t looper=0;looper<len;looper++) { // Deal with all input sequentially
		uint16_t spaceleft=sizeof(result)-outlen;
		if(mode<=1) {
			switch(buf[looper]) {
				case '?':
					if(outlen==0) { // Only do this if no other output has been generated for this request
                        char swversion[16];
                        GetVersion(VER_FIRMWARE, swversion, sizeof(swversion));
						uint8_t ver=PWM_GetVersion();
						uint16_t cvolt=I2C_Get_VBAT_VoltAvg();
						float ccur=I2C_Get_VBAT_CurAvg();
						uint8_t tmpmeas[35];
						if(mode==1) {
							res=snprintf(rptr,spaceleft,"\r\n\033[2A");
							rptr+=res;
							spaceleft-=res;
							outlen+=res;
						}
						snprintf(tmpmeas,sizeof(tmpmeas)," %5umV/%5umV  %4umA/%6.1fmA",setvoltage,cvolt,setcurrent,ccur);
						res=snprintf(rptr,spaceleft,"\033[?25l  WJ CDB Assist HW v%x.%x / SW %s / SN %s\033[K\r\n"\
														"\033[K\r\n"\
														"  Voltage Setpoint/Actual           Current Setpoint/Actual\033[K\r\n"\
														"\033#3%s\033[K\r\n"\
														"\033#4%s\033[K\r\n"\
														"\033#5\033[K\r\n"\
														"  [P/p] VBAT:%s  [u] Set VBAT voltage  [i] Set VBAT current\033[K\r\n"\
														"  [A/a] Btn1:%s  [B/b] Btn2:%s  [C/c] Btn3:%s\033[K\r\n"\
														"  [V/v] VBus:%s  [1234] Sel USB Port(%x) VBUS=%5.2fV %6.1fmA  UART Vref=%04umV\033[K\r\n"\
														"  [D/d] Load:%s  [I] Set load current(%umA)  Actual=%6.1fmA  Temp=%5.1fC\033[K\r\n"\
                                                        /*"  Dbg: [r] RawPWMOvr: %d,"*/ "  VBATSAR:%5umV USB2VBUS:%5umV USB3VBUS:%5umV\033[K\r\n"\
														"\033[11A\033[?25h",
														ver>>4,ver&0xf,swversion,GetSerialPtr(), // hw version+sw version+USB serial
														tmpmeas,tmpmeas,
														(status&STATUS_VBAT)?"ON ":"off",
														(status&STATUS_BTN1)?"ON ":"off",
														(status&STATUS_BTN2)?"ON ":"off",
														(status&STATUS_BTN3)?"ON ":"off",
														(status&STATUS_VBUS)?"ON ":"off",
														usbport,
                                                        USBMux_GetVBUSVoltage(),
                                                        USBMux_GetVBUSCurrent(),
														ADC_GetMillivolt(VTARGETSENSE),
														(status&STATUS_LOAD)?"ON ":"off",
                                                        setdummyloadcurrent,
                                                        DummyLoad_GetCur(),
                                                        DummyLoad_GetTemp(),
//                                                        PWM_GetOverride(),
                                                        ADC_GetMillivolt(VBATSENSE),
                                                        ADC_GetMillivolt(USB2SENSE),
                                                        ADC_GetMillivolt(USB3SENSE));
                                                        
						rptr+=res;
                        spaceleft-=res;
                        outlen+=res;
					}
					mode=0;	// Make sure we abort any current or voltage input
					break;
				case 'A':
				case 'a':
				case 'B':
				case 'b':
				case 'C':
				case 'c':
					if(buf[looper]<'a') on=1;
					btnnum=buf[looper]&0x7;
					switch(btnnum) {
						case 1:
							if(on) {
								status |= STATUS_BTN1;
								Buttons_Enable(1);
							} else {
								status &= ~STATUS_BTN1;
								Buttons_Disable(1);
							}
							break;
						case 2:
							if(on) {
								status |= STATUS_BTN2;
								Buttons_Enable(2);
							} else {
								status &= ~STATUS_BTN2;
								Buttons_Disable(2);
							}
							break;
						case 3:
							if(on) {
								status |= STATUS_BTN3;
								Buttons_Enable(3);
							} else {
								status &= ~STATUS_BTN3;
								Buttons_Disable(3);
							}
							break;
					}
					if(mode!=0) mode=1;	// Make sure we abort any current or voltage input
					break;
				case 'P':
				case 'p':
					if(buf[looper]<'a') {
						on=1;
						PWM_Setpoints(setvoltage,setcurrent);
						status |= STATUS_VBAT;
					} else {
						PWM_Setpoints(0,0);
						status &= ~STATUS_VBAT;
					}
					if(mode!=0) mode=1;	// Make sure we abort any current or voltage input
					break;
				case 'u':
					mode='u';
					accumulator=0;
					maxdigits=5;
					digitsleft=5;
					res=snprintf(rptr,spaceleft,"\r\nVoltage in mV:");
					rptr+=res;
					spaceleft-=res;
                    outlen+=res;
					printf("\nVoltage input mode");
					break;
				case 'i':
					mode='i';
					accumulator=0;
					maxdigits=4;
					digitsleft=4;
					res=snprintf(rptr,spaceleft,"\r\nCurrent in mA:");
					rptr+=res;
                    spaceleft-=res;
					outlen+=res;
					printf("\nCurrent input mode");
					break;
				case 'V':
				case 'v':
					if(buf[looper]<'a') {
						on=1;
						status |= STATUS_VBUS;
                        USBMux_SelPort(usbport);
					} else {
						status &= ~STATUS_VBUS;
                        USBMux_SelPort(0);
					}
					if(mode!=0) mode=1;	// Make sure we abort any current or voltage input
					break;
                case '1':
                case '2':
                case '3':
                case '4':
                    usbport = buf[looper]-'0';
                    if(status & STATUS_VBUS) {
                        USBMux_SelPort(usbport);
                    }
                    break;
                case 'D':
                case 'd':
                    if(buf[looper]<'a') {
                        status |= STATUS_LOAD;
                        DummyLoad_Setpoints(setvoltage, setdummyloadcurrent);
                    } else {
                        status &= ~STATUS_LOAD;
                        DummyLoad_Setpoints(0, 0);
                    }
                    break;
				case 'I':
					mode='I';
					accumulator=0;
					maxdigits=4;
					digitsleft=4;
					res=snprintf(rptr,spaceleft,"\r\nMax dummy load current in mA:");
					rptr+=res;
                    spaceleft-=res;
					outlen+=res;
					printf("\nDummy load current input mode");
					break;
/*				case 'r':
					mode='r';
					accumulator=0;
					maxdigits=4;
					digitsleft=4;
					res=snprintf(rptr,spaceleft,"\r\nRaw PWM override (0-2047, >= 2048 aborts):");
					rptr+=res;
                    spaceleft-=res;
					outlen+=res;
					printf("\nRaw PWM input mode");
					break;
*/                case '[':
                    Bootloadable_1_Load();
                    break;
				default:
					res=snprintf(rptr,spaceleft,"[%02x]",buf[looper]);
					rptr+=res;
                    spaceleft-=res;
					outlen+=res;
					if(mode!=0) mode=1;	// Make sure we abort any current or voltage input
			}
		} else {
			switch(buf[looper]) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if((mode!=0) && digitsleft) {
						uint16_t temp=accumulator<<1;
						accumulator<<=3; // Multiply by 8
						accumulator+=temp; // Now it's multiplied by 10
						accumulator+=(buf[looper]-'0'); // And add the new digit at the end
						digitsleft--;
						//wjprintf_P(PSTR("\nAcc:%u digitsleft=%x"),accumulator,digitsleft);
						*rptr++=buf[looper];
                        spaceleft--;
						outlen++;
					} // Fall-through intentional
				case 13:
				case 8:
				case 0x7f:
					if(mode>1) {
						if(buf[looper]==8 || buf[looper]==0x7f) { // Backspace
							if(digitsleft<maxdigits) {
								*rptr++=0x7f;
                                spaceleft--;
								outlen++;
								accumulator/=10; // SLOOOOOOOOOW
								digitsleft++;
							}							
						} else if(buf[looper]==13) { // Input done
							if(mode=='u') {
								if(accumulator<=15000) setvoltage=accumulator;
							} else if(mode=='i') {
								if(accumulator<=2500) setcurrent=accumulator;
							} else if(mode=='I') {
								if(accumulator<=2500) setdummyloadcurrent=accumulator;
							}
							if((mode=='u'||mode=='i') && (status & STATUS_VBAT)) {
								PWM_Setpoints(setvoltage,setcurrent);
							}
							if((mode=='I') && (status & STATUS_LOAD)) {
								DummyLoad_Setpoints(setvoltage,setdummyloadcurrent);
							}
							if(mode=='r') {
                                if(accumulator < 2048) {
                                    PWM_Override( accumulator );
                                } else {
                                    PWM_Override( -1 );
                                }
                            }
							mode=1;
						}
					}					
					break;
				default:
					res=snprintf(rptr,spaceleft,"[%02x]",buf[looper]);
					rptr+=res;
                    spaceleft-=res;
					outlen+=res;
					if(mode!=0) mode=1;	// Make sure we abort any current or voltage input
			}
		}
	}
	rptr=result;
	while(outlen>0) {
		uint8_t queuesize = (outlen>64)?64:outlen;
		USBQueueFrame(CONTROLDATA_IN_EP,rptr,queuesize);
		rptr+=queuesize;
		outlen-=queuesize;
	}
}

void UpdateCtrl(void) {
    if(mode<=1) ActionHandler((uint8_t*)"?",1);
}
