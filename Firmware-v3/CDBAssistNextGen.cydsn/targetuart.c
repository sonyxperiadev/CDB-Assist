/*
 * targetuart.c - Target UART handling
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
#include <string.h>
#include <stdint.h>
#include "targetuart.h"

// Very hard-coded for a total of 256 bytes buffer and a power-of-two buffer size!
#define NUM_RX_BUFS (4)
#define RX_BUF_SIZE (64) // Per buffer
uint8_t rxbufs[NUM_RX_BUFS][RX_BUF_SIZE];
volatile uint8_t bufstatus=0;

void targetuart_putc(char c) {

}

#define CDC2INEP (11) // See USB driver, this is the EP to queue received target UART data on

uint8_t flushtimer=0;

void targetuart_flushbuffer(void) { // Called periodically to make sure data doesn't become stale while trying to fill the buffers

}

void targetuart_init(void) {
    TargetUART_1_Start();
    targetuart_config( 115200, 0, 8, 1);
}

void targetuart_breakctl(uint8_t enable) {
    if(enable==1) {
        TargetUART_1_SendBreak(TargetUART_1_SEND_WAIT_REINIT); // Hacky solution at the moment, only a single length available
    } else {
        // Re-enable UART
    }
}

void targetuart_config(uint32_t baudrate, uint8_t parity, uint8_t databits, uint8_t stopbits) {
    // UART clock fractional divider setup based on requested bitrate
    float multiplier = 16777216.0f / 64000000.0f;
    multiplier *= ((baudrate) * 8);
    CY_SET_REG24(fracdiv_1_WJ_FracDiv_A1_PTR, (uint32_t)multiplier);

    // TODO: Stop bits, databits and parity settings
}
