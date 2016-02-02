/*
 * debuguart.c - Debug UART handling
 *
 * Copyright (C) 2011-2015 Werner Johansson, wj@unifiedengineering.se
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
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "debuguart.h"

// Imported defines for Cortex-M3 SWV
#define CYSWV_ENCODING_MANCHESTER	0x01
#define CYSWV_ENCODING_UART			0x02
#define ENCODING_USE				CYSWV_ENCODING_UART

#define TRCENA          0x01000000
#define DTS_ENA			0x02		// TRACE_CTL: Enable differential timestamps
#define ITM_ENA			0x01		// TRACE_CTL: Enable ITM
#define SYNC_ENA		0x04		// TRACE_CTL: Enable sync output from SWV
#define SWV_ENA			0x10		// TRACE_CTL: Enable SWV behavior
#define SWV_CLK_ENA			0x04	// MLOGIC_DEBUG, enable SWV clk
#define SWV_CLK_CPU_DIV_2	0x08	// MLOGIC_DEBUG, set SWV clk to CPU/2
#define SWV_CLK			4000u		// The UART will run at 4Mbps

#define SYNC_COUNT		0x0100		//0x07AF

// PSoC5LP SWV stimulus register defines
#define CYREG_SWV_ITM_SPR_DATA0 CYDEV_ITM_BASE
#define CYREG_SWV_ITM_SPR_DATA1 CYDEV_ITM_BASE+4
#define CYREG_SWV_ITM_SPR_DATA2 CYDEV_ITM_BASE+8
#define CYREG_SWV_ITM_SPR_DATA4 CYDEV_ITM_BASE+16

#ifndef MINIMALISTIC
// If RAM gets tight feel free to wind this down but it's VERY handy to have a big buffer to catch all output during boot (until USB enumerates)
#define LOGRINGSIZE (512)
volatile uint16_t log_ring_write=0;
volatile uint16_t log_ring_read=LOGRINGSIZE-8;
volatile uint8_t log_ring[LOGRINGSIZE];
#endif

int uart1_putc(char c) {
	if (c == '\n')
		uart1_putc('\r');
    while (CY_GET_REG8(CYDEV_ITM_BASE) == 0);
    CY_SET_REG8(CYDEV_ITM_BASE,c);
#ifndef MINIMALISTIC
	// Copy to buffer if there's at least one byte available
	// (write ptr is >= readptr or write ptr is < readptr-1)
	if((log_ring_write>=log_ring_read) || ((log_ring_write<(log_ring_read-1)) && (log_ring_read!=0))) {
		log_ring[log_ring_write]=c;
		log_ring_write++;
		if(log_ring_write==LOGRINGSIZE) log_ring_write=0;
	}
#endif
	return 0;
}

// Override _write_r so we actually can do printf
int _write_r(struct _reent *ptr, int file, char *buf, int len) {
	int foo = len;
	while(foo--) uart1_putc(*buf++);
	return len;
}

void debuguart1_init(void) {
    uint8 clkdiv;
	
	// Enable serial wire viewer (SWV) on PSoC5LP
    // Note that the Cortex-M3 ITM will output a "packet header" before every byte in this setup, basically lowering the throughput to 2Mbps!
	
	/* Assuming SWV_CLK is CPU CLK (bus clk) / 2. Need to configure divider to output the right freq 
	Include a warning if the divider does not produce the exact frequency required */
	#if (BCLK__BUS_CLK__KHZ % (2u*SWV_CLK)) == 0
	clkdiv = (BCLK__BUS_CLK__KHZ / (2u*SWV_CLK))-1;	
	#else
	/* BUS CLK / 2 must be able to be divided down with an integer value to reach SWV_CLK 
	To fix, either change your BUS CLK or change your SWV divider (might require a change
	to your PC application to change the Miniprog3 clock frequency) */
	#warning Accurate SWV divider could not be reached. BUS CLK / 2 must be able to be divided down with an integer value to reach SWV_CLK.
	#endif
	clkdiv = (BCLK__BUS_CLK__KHZ / (2u*SWV_CLK))-1;
	
	//Enable output features
	CY_SET_REG32(CYDEV_CORE_DBG_EXC_MON_CTL,TRCENA);
	
	// Set following bits in the MLOGIC_DEBUG register:
	// swv_clk_en = 1, swv_clk_sel = 1 (CPUCLK/2).
	CY_SET_REG8(CYREG_MLOGIC_DEBUG, (CY_GET_REG8(CYREG_MLOGIC_DEBUG) | SWV_CLK_ENA | SWV_CLK_CPU_DIV_2));
	
	//enable ITM CR write privs
	CY_SET_REG32(CYDEV_ITM_LOCK_ACCESS,0xC5ACCE55);	//enable ITM CR privs
	
	//Enable ITM
	CY_SET_REG32(CYDEV_ITM_TRACE_CTRL, (uint32) (SWV_ENA | ITM_ENA/* | SYNC_ENA*/));

	//Set Output Divisor Register (13 bits)
	CY_SET_REG32(CYDEV_TPIU_ASYNC_CLK_PRESCALER,(uint32)(clkdiv));

	//Setup the output encoding style
	CY_SET_REG8(CYDEV_TPIU_PROTOCOL, ENCODING_USE);
	
	//Set trace enable registers
	CY_SET_REG32(CYDEV_ITM_TRACE_EN, 0x00000001); //enable 3 stimulus register
	
	//Set sync counter
//	CY_SET_REG8(CYDEV_ETM_SYNC_FREQ + 1, (uint8)(SYNC_COUNT >> 8));
//	CY_SET_REG8(CYDEV_ETM_SYNC_FREQ + 0, (uint8)SYNC_COUNT);
	
	CY_SET_REG32(CYDEV_TPIU_FORM_FLUSH_CTRL,0x00000000);   // Formatter and Flush Control Register

#ifndef MINIMALISTIC
	memset((void*)log_ring,0,sizeof(log_ring));
#endif

//#ifdef __NEWLIB__
	setbuf(stdout, NULL); // Needed to get rid of default line-buffering in newlib not present in redlib
//#endif

//	wjprintf_set_charout((charoutfunc_t)uart1_putc);
}

#ifndef MINIMALISTIC
uint16_t debugring_get_rx_bytes_avail(void) {
	uint16_t theLen=0;
	if(log_ring_read!=log_ring_write) {
		if(log_ring_read<log_ring_write) {
			theLen=log_ring_write-log_ring_read;
		} else {
			theLen=LOGRINGSIZE-log_ring_read+log_ring_write;
		}			
	}
	return theLen;
}

uint8_t* debugring_get_rx_ptr(void) {
	return (uint8_t*)log_ring+log_ring_read;
}

uint8_t* debugring_get_buf_end_ptr(void) {
	return (uint8_t*)log_ring+LOGRINGSIZE-1;
}

uint8_t* debugring_get_buf_start_ptr(void) {
	return (uint8_t*)log_ring;
}

void debugring_advance_rx_ptr(uint16_t theAmount) {
	log_ring_read+=theAmount;
	if(log_ring_read>=LOGRINGSIZE) log_ring_read-=LOGRINGSIZE;
}
#endif
