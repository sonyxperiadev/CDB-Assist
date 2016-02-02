/*
 * lcd.c - This drives a display based on the SSD1309 (PMOLED) controller
 *
 * Copyright (C) 2010,2012-2016 Werner Johansson, wj@unifiedengineering.se
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
#include "lcd.h"

#include "bigfont.h"
#include "smallfont.h"

// Frame buffer storage (each "page" is 8 pixels high)
uint8_t FB[FB_HEIGHT/8][FB_WIDTH];

#ifdef LCD_PAGEMODE
const uint8_t oledinit[]={ 0xfd,0x12, 0xd5,0xa0, 0xa8,0x3f, 0xa0, 0xc0, 0xda,0x12, 0x81,0x1f, 0xd9,0x25, 0xdb,0x34, /*0xa4, 0xa6,*/ 0xaf }; // SSD1309-based 128x64 OLED module (0xa0,0xc0=flex on top 0xa1,0xc8=flex down)
#else
const uint8_t oledinit[]={ 0xfd,0x12, 0xd5,0xa0, 0xa8,0x3f, 0xa0, 0xc0, 0xda,0x12, 0x81,0x1f, 0xd9,0x25, 0xdb,0x34, 0x20,0x00, /*0xa4, 0xa6,*/ 0xaf, 0x40, 0xb0, 0x10, 0x00 }; // SSD1309-based 128x64 OLED module (0xa0,0xc0=flex on top 0xa1,0xc8=flex down)
#endif

//Go to pageX
uint8_t go[]={ 0x40, 0xb0, 0x10, 0x00 };

typedef struct __attribute__ ((packed)) {
	uint8_t bfType[2]; // 'BM' only in this case
	uint32_t bfSize; // Total size
	uint16_t bfReserved[2];
	uint32_t bfOffBits; // Pixel start byte
	uint32_t biSize; // 40 bytes for BITMAPINFOHEADER
	int32_t biWidth; // Image width in pixels
	int32_t biHeight; // Image height in pixels (if negative image is right-side-up)
	uint16_t biPlanes; // Must be 1
	uint16_t biBitCount; // Must be 1
	uint32_t biCompression; // Only 0 (uncompressed) supported at the moment
	uint32_t biSizeImage; // Pixel data size in bytes
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
	uint32_t aColors[2]; // Palette data, first color is used if pixel bit is 0, second if pixel bit is 1
} BMhdr_t;

void delayms(uint16_t millis) {
	CyDelay( millis );
}

void xmit_spi(uint8_t* buf, uint8_t len) {
    DispSPI_PutArray(buf, len);
	while(!(DispSPI_ReadTxStatus() & DispSPI_STS_SPI_DONE));
}

void FB_init(void) {
    DispSPI_Start();
    CyPins_ClearPin(DispEna_0);
    CyPins_ClearPin(DispRst_0);
    //CyPins_SetPin(DispCS_0);
    CyPins_ClearPin(DispCD_0);
    delayms(2);
    CyPins_SetPin(DispRst_0);
    delayms(2);
    //CyPins_ClearPin(DispCS_0);
	xmit_spi((uint8_t*)oledinit,sizeof(oledinit));
    CyPins_SetPin(DispEna_0);
}

void FB_clear(void) {
	// Init FB storage
	for(uint8_t j=0;j<(FB_HEIGHT/8);j++) {
		memset(FB[j],0,FB_WIDTH);
	}
}

void FB_update(void) {
// Complete FB update
#ifdef LCD_PAGEMODE
        for(uint8_t pagenum=0;pagenum<(FB_HEIGHT/8);pagenum++) {
            CyPins_ClearPin(DispCD_0);
			go[1]=0xb0 | pagenum;
			xmit_spi((uint8_t*)go,sizeof(go));
            CyPins_SetPin(DispCD_0);
			xmit_spi(FB[pagenum],FB_WIDTH);
		}
#else // Horizontal mode, one continuous transmission 770us total transfer time at 10.66MHz
        CyPins_SetPin(DispCD_0);
        uint16_t byteCount  = FB_WIDTH*(FB_HEIGHT/8);
        uint8_t* bufIndex = FB[0];
        while(byteCount-- > 0u) {
            while( !(DispSPI_TX_STATUS_REG & DispSPI_STS_TX_FIFO_NOT_FULL) );
            CY_SET_REG8(DispSPI_TXDATA_PTR, *bufIndex++);
        }
	    while(!(DispSPI_ReadTxStatus() & DispSPI_STS_SPI_DONE));
#endif
}

void FB_offset(uint8_t yoffset) {
	go[0]=0x40 | (yoffset&0x3f); // Set which frame buffer row should be at the top of the display
}

void charoutbig(uint8_t theChar, uint8_t X, uint8_t Y) {
	uint16_t fontoffset=((theChar&0x7f)-0x20) * 12;
	uint8_t yoffset=Y&0x7;
	Y>>=3;
	for(uint8_t x=0; x<12; x++) {
		uint32_t temp=bigfont[fontoffset++];
		if(theChar&0x80) temp^=0xffff;
		temp=temp<<yoffset; // Shift pixel data to the correct lines
		uint32_t old=(FB[Y][X] | ((uint16_t)(FB[Y+1][X])<<8) | ((uint32_t)(FB[Y+2][X])<<16));
		old&=~((uint32_t)(0xffff<<yoffset)); //Clean out old data
		temp|=old; // Merge old data in FB with new char
		if(X>=FB_WIDTH) return;
		if(Y<((FB_HEIGHT/8)-0)) FB[Y][X]=temp;
		if(Y<((FB_HEIGHT/8)-1)) FB[Y+1][X]=temp>>8;
		if(Y<((FB_HEIGHT/8)-2)) FB[Y+2][X]=temp>>16;
		X++;
	}
}

void charoutsmall(uint8_t theChar, uint8_t X, uint8_t Y) {
	// First of all, make lowercase into uppercase
	// (as there are no lowercase letters in the font)
	if((theChar&0x7f)>=0x61 && (theChar&0x7f)<=0x7a) theChar-=0x20;
	uint16_t fontoffset=((theChar&0x7f)-0x20) * 6;
	uint8_t yoffset=Y&0x7;
	Y>>=3;
	uint8_t width=(theChar&0x80)?7:6;
	for(uint8_t x=0; x<width; x++) {
		uint16_t temp=smallfont[fontoffset++];
		if(theChar&0x80) temp^=0x7f;
		temp=temp<<yoffset; // Shift pixel data to the correct lines
		uint16_t old=(FB[Y][X] | (FB[Y+1][X]<<8));
		old&=~(0x7f<<yoffset); //Clean out old data
		temp|=old; // Merge old data in FB with new char
		if(X>=(FB_WIDTH)) return; // make sure we don't overshoot
		if(Y<((FB_HEIGHT/8)-0)) FB[Y][X]=temp&0xff;
		if(Y<((FB_HEIGHT/8)-1)) FB[Y+1][X]=temp>>8;
		X++;
	}
}

void disp_str(uint8_t* theStr, uint8_t theLen, uint8_t startx, uint8_t y, uint8_t theFormat) {
	uint8_t invmask = theFormat & 0x80;
	switch(theFormat & 0x7f) {
		case FONT6X6:
			for(uint8_t q=0;q<theLen;q++) {
				charoutsmall( theStr[q] | invmask, startx, y );
				startx+=6;
			}
			break;
        case FONT12X16:
			for(uint8_t q=0;q<theLen;q++) {
				charoutbig( theStr[q] | invmask, startx, y );
				startx+=12;
			}
			break;
    }
}

void LCD_MultiLineH(uint8_t startx, uint8_t endx, uint64_t ymask) {
	for(uint8_t x=startx;x<=endx;x++) {
		FB[0][x]|=ymask&0xff;
		FB[1][x]|=ymask>>8;
		FB[2][x]|=ymask>>16;
		FB[3][x]|=ymask>>24;
#if FB_HEIGHT==64
		FB[4][x]|=ymask>>32;
		FB[5][x]|=ymask>>40;
		FB[6][x]|=ymask>>48;
		FB[7][x]|=ymask>>56;
#endif		
	}
}

// At the moment this is a very basic BMP file reader with the following limitations:
// The bitmap must be 1-bit, uncompressed with a BITMAPINFOHEADER.
uint8_t LCD_BMPDisplay(uint8_t* thebmp,uint8_t xoffset,uint8_t yoffset) {
	BMhdr_t* bmhdr;
	uint8_t upsidedown=1;
	uint8_t inverted=0;
	uint16_t pixeloffset;
	uint8_t numpadbytes=0;
	
	bmhdr=(BMhdr_t*)thebmp;
	
//	wjprintf_P(PSTR("\n%s: bfSize=%x biSize=%x"), __FUNCTION__, (uint16_t)bmhdr->bfSize, (uint16_t)bmhdr->biSize);
//	wjprintf_P(PSTR("\n%s: Image size is %d x %d"), __FUNCTION__, (int16_t)bmhdr->biWidth, (int16_t)bmhdr->biHeight);
	if(bmhdr->biPlanes!=1 || bmhdr->biBitCount!=1 || bmhdr->biCompression!=0) {
//		wjprintf_P(PSTR("\n%s: Incompatible bitmap format!"), __FUNCTION__);
		return 1;
	}
	pixeloffset=bmhdr->bfOffBits;
	if(bmhdr->aColors[0]==0) {
		inverted=1;
	}
	if(bmhdr->biHeight<0) {
		bmhdr->biHeight=-bmhdr->biHeight;
		upsidedown=0;
	}
	if((bmhdr->biWidth+xoffset > FB_WIDTH) || (bmhdr->biHeight+yoffset > FB_HEIGHT)) {
//		wjprintf_P(PSTR("\n%s: Image won't fit on display!"), __FUNCTION__);
		return 1;
	}

	// Figure out how many dummy bytes that is present at the end of each line
	// If the image is 132 pixels wide then the pixel lines will be 20 bytes (160 pixels)
	// 132&31 is 4 which means that there are 3 bytes of padding
	numpadbytes=(4-((((bmhdr->biWidth)&0x1f)+7)>>3))&0x03;
//	wjprintf_P(PSTR("\n%s: Skipping %d padding bytes after each line"), __FUNCTION__, numpadbytes);

	for(int8_t y=bmhdr->biHeight-1; y>=0; y--) {
		uint8_t realY=upsidedown?(uint8_t)y:(uint8_t)(bmhdr->biHeight)-y;
		realY+=yoffset;
		uint8_t pagenum=realY>>3;
		uint8_t pixelval=1<<(realY&0x07);
		for(uint8_t x=0; x<bmhdr->biWidth; x+=8) {
			uint8_t pixel=thebmp[pixeloffset++];
			if(inverted) pixel^=0xff;
			uint8_t max_b = bmhdr->biWidth - x;
            if(max_b>8) max_b = 8;
			for(uint8_t b=0; b<max_b; b++) {
				if(pixel&0x80) {
					FB[pagenum][x+b+xoffset]|=pixelval;
				}
				pixel=pixel<<1;
			}
		}
		pixeloffset+=numpadbytes;
	}
	return 0;
}
