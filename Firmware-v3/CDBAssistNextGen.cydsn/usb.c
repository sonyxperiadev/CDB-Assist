/*
 * usb.c - Low-level USB handling for Cypress PSoC5LP
 *
 * Copyright (C) 2013-2015 Werner Johansson, wj@unifiedengineering.se
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
#include "usb.h"
#include "lcd.h"
#include "debuguart.h"
#include "targetuart.h"
#include "AppCallbacks.h"

// Additional define missing from Cypress framework
#define USBFS_1_CDC_SEND_BREAK (0x23u)

volatile uint8_t ControlLineCoding[USBFS_1_LINE_CODING_SIZE];
volatile T_USBFS_1_XFER_STATUS_BLOCK ControlStatus;
volatile uint8_t TargetLineCoding[USBFS_1_LINE_CODING_SIZE];
volatile T_USBFS_1_XFER_STATUS_BLOCK TargetStatus;

static uint8_t Local_DispatchCDCClassRqst(uint8_t ifnum) {
    uint8_t requestHandled = USBFS_1_FALSE;

    if ((CY_GET_REG8(USBFS_1_bmRequestType) & USBFS_1_RQST_DIR_MASK) == USBFS_1_RQST_DIR_D2H) { // Control Read
        switch (CY_GET_REG8(USBFS_1_bRequest)) {
            case USBFS_1_CDC_GET_LINE_CODING:
                if(ifnum==CONTROLMGMT_IF) {
                    USBFS_1_currentTD.count = USBFS_1_LINE_CODING_SIZE;
                    USBFS_1_currentTD.pData = ControlLineCoding;
                    requestHandled  = USBFS_1_InitControlRead();
                } else if(ifnum==TARGETMGMT_IF) {
                    USBFS_1_currentTD.count = USBFS_1_LINE_CODING_SIZE;
                    USBFS_1_currentTD.pData = TargetLineCoding;
                    requestHandled  = USBFS_1_InitControlRead();
                }
                break;
            default:
                break;
        }
    }
    else if ((CY_GET_REG8(USBFS_1_bmRequestType) & USBFS_1_RQST_DIR_MASK) == USBFS_1_RQST_DIR_H2D) { // Control Write
        switch (CY_GET_REG8(USBFS_1_bRequest)) {
            case USBFS_1_CDC_SET_LINE_CODING:
                // Note that the data isn't actually available yet, it will be populated by the next OUT xfer!
                // The status block will be updated when new data has been received
                if(ifnum==CONTROLMGMT_IF) {
                    USBFS_1_currentTD.count = USBFS_1_LINE_CODING_SIZE;
                    USBFS_1_currentTD.pData = ControlLineCoding;
                    USBFS_1_currentTD.pStatusBlock = &ControlStatus;
                    requestHandled = USBFS_1_InitControlWrite();
                } else if(ifnum==TARGETMGMT_IF) {
                    USBFS_1_currentTD.count = USBFS_1_LINE_CODING_SIZE;
                    USBFS_1_currentTD.pData = TargetLineCoding;
                    USBFS_1_currentTD.pStatusBlock = &TargetStatus;
                    requestHandled = USBFS_1_InitControlWrite();
                }
                break;

            case USBFS_1_CDC_SET_CONTROL_LINE_STATE:
                USBFS_1_lineControlBitmap = CY_GET_REG8(USBFS_1_wValueLo);
                USBFS_1_lineChanged |= USBFS_1_LINE_CONTROL_CHANGED;
                requestHandled = USBFS_1_InitNoDataControlTransfer();
                break;

            case USBFS_1_CDC_SEND_BREAK:
                {
				uint16_t blength = CY_GET_REG16(USBFS_1_wValue);
				uint8_t* ifstring = (uint8_t*)"Control";
				if(ifnum == TARGETMGMT_IF) {
					ifstring = (uint8_t*)"Target";
					// 0 value means stop an ongoing break
					// 0xffff means pull line low until 0 is sent
					// Anything else means pull line low for that many milliseconds
					if(blength == 0) {
						targetuart_breakctl(0); // Disable break
					} else if(blength == 0xffff) {
    					targetuart_breakctl(1); // Enable break, no timeout
					} else {
//FIXME						uint32_t numticks = blength * TICKS_PER_MS;
//FIXME						if(numticks > 0x7fff) numticks = 0x7fff; // Truncate timeouts longer than we can handle (nobody uses this anyway)
//FIXME						Sched_SetState(BREAK_WORK, 1, (int16_t)numticks);
//FIXME						targetuart_breakctl(1); // Enable break
						//printf("\nScheduled break end in 0x%x ticks", (int16_t)numticks);
					}
				}
				printf("\nSendBreak for %s UART: 0x%xms", ifstring, blength);
                }
                requestHandled = USBFS_1_InitNoDataControlTransfer();
                break;

            default:
                break;
        }
    }

    return(requestHandled);
}

// Overridden from USBFS_1_cls.c to be able to deal with multiple CDC-ACM interfaces
uint8 USBFS_1_DispatchClassRqst(void) {
    uint8 requestHandled = USBFS_1_FALSE;
    uint8 interfaceNumber = 0u;

    switch(CY_GET_REG8(USBFS_1_bmRequestType) & USBFS_1_RQST_RCPT_MASK) {
        case USBFS_1_RQST_RCPT_IFC:        /* Class-specific request directed to an interface */
            interfaceNumber = CY_GET_REG8(USBFS_1_wIndexLo); /* wIndexLo contain Interface number */
            break;
        case USBFS_1_RQST_RCPT_EP:         /* Class-specific request directed to the endpoint */
            /* Find related interface to the endpoint, wIndexLo contain EP number */
            interfaceNumber = USBFS_1_EP[CY_GET_REG8(USBFS_1_wIndexLo) &
                              USBFS_1_DIR_UNUSED].interface;
            break;
        default:    /* RequestHandled is initialized as FALSE by default */
            break;
    }

    // Avoid printing from in here (irq context), useful for debugging specific issues though
    //printf("\nIF %x class req", interfaceNumber);

    // Handle Class request depend on interface type
    switch(USBFS_1_interfaceClass[interfaceNumber]) {
        case USBFS_1_CLASS_HID:
            requestHandled = USBFS_1_DispatchHIDClassRqst();
            break;
        case USBFS_1_CLASS_CDC:
            requestHandled = Local_DispatchCDCClassRqst(interfaceNumber);
            break;
        default:
            break;
    }

    return(requestHandled);
}

uint8_t lastconfigured = 0;

void USB_Init(void) {
    ControlStatus.length = 0;
    TargetStatus.length = 0;
    USBFS_1_Start(0, USBFS_1_DWR_VDDD_OPERATION);
}

uint8_t* ep7fifoptr[16];
uint8_t ep7fifolen[16];
uint8_t ep7fifopos=0;

static void USBPopFrame(uint8_t epnum) {
	static uint8_t lastsendlength=0;
	uint8_t writepos=ep7fifopos&0x0f;
	uint8_t readpos=ep7fifopos>>4;
	if(writepos != readpos || lastsendlength == 64) {
        if(USBFS_1_GetEPState(epnum) == USBFS_1_IN_BUFFER_EMPTY) { // Device-to-host endpoint idle
            uint8_t* dataptr = ep7fifoptr[readpos];
    
			if(writepos != readpos) {
				lastsendlength=ep7fifolen[readpos];
				readpos++;
				if(readpos>15) {
					readpos = 0;
				}
				ep7fifopos = (ep7fifopos & 0x0f) | (readpos<<4);
			} else {
				lastsendlength=0; // NULL frame
			}					

            USBFS_1_LoadInEP(epnum, dataptr, lastsendlength);
		}
	}
}

void USBQueueFrame(uint8_t ep,uint8* bufptr,uint8_t len) {
    uint8_t writepos=ep7fifopos&0x0f;
	ep7fifoptr[writepos] = bufptr;
	ep7fifolen[writepos] = len;
	writepos++;
	if(writepos>15) {
		writepos = 0;
	}
	ep7fifopos = (ep7fifopos & 0xf0) | writepos;
    USBPopFrame(ep);
}

void USB_Work(void) {
    uint8_t usbconfigured = USBFS_1_GetConfiguration();
    if(usbconfigured != lastconfigured) {
        USBFS_1_EnableOutEP(CONTROLDATA_OUT_EP); // Control UART commands
        USBFS_1_EnableOutEP(TARGETDATA_OUT_EP); // Target UART TX data
        lastconfigured = usbconfigured;
        printf("\nUSB reconfigured");
    }

#if 0
    // Dump endpoint RAM pointers
    uint8_t ptr=0;
    printf("\nEP");
    for(uint8_t eps=0;eps<7;eps++) {
        uint16_t waddr = CY_GET_REG8((reg8 *)(USBFS_1_ARB_RW1_WA_MSB_IND + ptr)) << 8;
        waddr += CY_GET_REG8((reg8 *)(USBFS_1_ARB_RW1_WA_IND + ptr));
        uint16_t raddr = CY_GET_REG8((reg8 *)(USBFS_1_ARB_RW1_RA_MSB_IND + ptr)) << 8;
        raddr += CY_GET_REG8((reg8 *)(USBFS_1_ARB_RW1_RA_IND + ptr));
        ptr += USBFS_1_EPX_CNTX_ADDR_OFFSET;
        printf(" %x:%03x/%03x", eps+1, waddr, raddr);
    }
#endif

    if(ControlStatus.length) {
        printf("\nControl IF line config xfer completion, %x bytes, status 0x%02x", ControlStatus.length, ControlStatus.status);
        // Ignoring any changes here as Control IF is virtual
        ControlStatus.length = 0;
    }
    
    if(TargetStatus.length) {
        printf("\nTarget IF line config xfer completion, %x bytes, status 0x%02x", TargetStatus.length, TargetStatus.status);
        // Re-configure baudrate, stopbits and parity of physical UART here!

        uint32_t requestedbitrate = TargetLineCoding[0] | (uint32_t)TargetLineCoding[1]<<8 | (uint32_t)TargetLineCoding[2]<<16 | (uint32_t)TargetLineCoding[3]<<24;
		uint8_t requestedparity = TargetLineCoding[5];
		uint8_t requesteddatabits = TargetLineCoding[6];
		uint8_t requestedstopbits = TargetLineCoding[4];

        targetuart_config(requestedbitrate, requestedparity, requesteddatabits, requestedstopbits==0?1:2);
		printf("\nSetLineCoding for Target UART: %ubps, %c%x%s requested", requestedbitrate, requestedparity==0?'n':requestedparity==1?'o':'e',
													requesteddatabits, requestedstopbits==0?"1":requestedstopbits==1?"1.5(bork)":"2");
        TargetStatus.length = 0;
    }
    
    uint16_t chunklen = debugring_get_rx_bytes_avail();
    if(chunklen) {
        if(USBFS_1_GetEPState(HID_IN_EP) == USBFS_1_IN_BUFFER_EMPTY) {
            uint8_t USBTXBuf1[8];
            chunklen=(chunklen>8)?8:chunklen;
	        uint8_t* copyptr=debugring_get_rx_ptr();
		    uint8_t* endptr=debugring_get_buf_end_ptr();
		    uint16_t steplen=endptr-copyptr+1;
		    if(steplen>chunklen) steplen=chunklen;
		    memcpy(USBTXBuf1, copyptr, steplen);
		    if(steplen!=chunklen) {
    			memcpy(USBTXBuf1+steplen, debugring_get_buf_start_ptr(), chunklen-steplen);
		    }
		    debugring_advance_rx_ptr(chunklen);

    		// Must pad report for now
		    while(chunklen < 8)
    			USBTXBuf1[chunklen++]=0x00;

            USBFS_1_LoadInEP(HID_IN_EP, USBTXBuf1, sizeof(USBTXBuf1));
        }
    }

    if(USBFS_1_GetEPState(CONTROLDATA_OUT_EP) == USBFS_1_OUT_BUFFER_FULL) {
        uint8_t tmpbuf[64];
        uint8_t len;
        memset(tmpbuf,0,64);

#if 0        
        // Mega hack to make sure the endpoint RAM pointers isn't getting out of sync, something's broken in the Cypress code it seems
        CY_SET_REG8(USBFS_1_ARB_RW4_WA_MSB_PTR, 0x00);
        CY_SET_REG8(USBFS_1_ARB_RW4_WA_PTR, 0x88);
        CY_SET_REG8(USBFS_1_ARB_RW4_RA_MSB_PTR, 0x00);
        CY_SET_REG8(USBFS_1_ARB_RW4_RA_PTR, 0x88);

        if (!(CY_GET_REG8(USBFS_1_ARB_EP4_CFG_PTR) & USBFS_1_ARB_EPX_CFG_RESET)) {
            printf("\nReset bit not set!");
        }
#endif
        len = USBFS_1_GetEPCount(CONTROLDATA_OUT_EP);
        if(len>64) len=64;
        uint8_t actual = USBFS_1_ReadOutEP(CONTROLDATA_OUT_EP, tmpbuf, len);
        //while (USBFS_1_GetEPState(CONTROLDATA_OUT_EP) == USBFS_1_OUT_BUFFER_FULL); // Wait for DMA to complete
        //USBFS_1_EnableOutEP(CONTROLDATA_OUT_EP); // Already taken care of in Cypress code
        uint16_t waddr = CY_GET_REG8(USBFS_1_ARB_RW8_WA_MSB_PTR) << 8;
        waddr += CY_GET_REG8(USBFS_1_ARB_RW8_WA_PTR);
        uint16_t raddr = CY_GET_REG8(USBFS_1_ARB_RW8_RA_MSB_PTR) << 8;
        raddr += CY_GET_REG8(USBFS_1_ARB_RW8_RA_PTR);

        printf("\nControl IF received %u/%u bytes epram w%03x r%03x from host: '%s'", len, actual, waddr, raddr, tmpbuf);
        
        ActionHandler(tmpbuf,len);
    }

    USBPopFrame(CONTROLDATA_IN_EP);

    if(USBFS_1_GetEPState(TARGETDATA_IN_EP) == USBFS_1_IN_BUFFER_EMPTY) {
        uint16_t numavail = TargetUART_1_GetRxBufferSize();
        uint8_t tmpbuf[64];
        static int8_t lastlength=-1;

        if(numavail) {
            if(numavail>64) numavail=64;
            for(uint8_t lp=0;lp<numavail;lp++) {
                tmpbuf[lp]=TargetUART_1_GetByte();
            }
        } 
        if(numavail || lastlength) {
            USBFS_1_LoadInEP(TARGETDATA_IN_EP, tmpbuf, numavail); // Always queue up one NULL xfer when having nothing to send
        }
        lastlength=numavail;
    }

    if(USBFS_1_GetEPState(TARGETDATA_OUT_EP) == USBFS_1_OUT_BUFFER_FULL) {
        uint8_t tmpbuf[64];
        uint8_t len;
        memset(tmpbuf,0,64);
        len = USBFS_1_GetEPCount(TARGETDATA_OUT_EP);
        if(len>64) len=64;

        // Don't read the OutEP if the TargetUART TX FIFO is full!!
        uint16_t spaceavail = TargetUART_1_TXBUFFERSIZE - TargetUART_1_GetTxBufferSize();
        if(spaceavail >= len) {
            USBFS_1_ReadOutEP(TARGETDATA_OUT_EP, tmpbuf, len);
            //while (USBFS_1_GetEPState(TARGETDATA_OUT_EP) == USBFS_1_OUT_BUFFER_FULL); // Wait for DMA to complete
            //USBFS_1_EnableOutEP(TARGETDATA_OUT_EP); // Already taken care of in Cypress code
            
            //printf("\nTarget IF received %u bytes from host: '%s'", len, tmpbuf);
            TargetUART_1_PutArray(tmpbuf, len);
        }
    }
}
