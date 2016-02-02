/*
 * main.c - Simple bootloader customization
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
#include <../CDBAssistNextGen.cydsn/lcd.h>
extern uint8_t logobmp[];

void main()
{
    PWM_1_Start(); // Status flash during bootloader run

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    FB_init();
    FB_clear();
    //disp_str("IN DAS BOOTLOADER!",18,0,0,FONT6X6);
    LCD_BMPDisplay(logobmp, 0, 0);
    FB_update();

    CyBtldr_Start();

    /* CyGlobalIntEnable; */ /* Uncomment this line to enable global interrupts. */
    for(;;)
    {
        /* Place your application code here. */
    }
}

/* [] END OF FILE */
