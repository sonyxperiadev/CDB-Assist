#ifndef LCD_H
#define LCD_H
#include <stdint.h>
    
#define FONT6X6 (0)
#define FONT12X16 (1)
#define INVERT (0x80)

#define DISPTYPE_LCD (0)
#define DISPTYPE_OLED (1)

#define FB_WIDTH (128)
#define FB_HEIGHT (64)

void lcd_setrawcontrast(uint8_t rawvalue);
uint8_t lcd_getcontrast(void);
uint8_t lcd_setcontrast(uint8_t thecontrast);
void lcd_reinit(void);

void xmit_spi(uint8_t* buf, uint8_t len);
void FB_init(void);
void FB_clear(void);
void FB_update(void);
void FB_offset(uint8_t yoffset);
void charoutbig(uint8_t theChar, uint8_t X, uint8_t Y);
void charoutsmall(uint8_t theChar, uint8_t X, uint8_t Y);
void disp_str(uint8_t* theStr, uint8_t theLen, uint8_t startx, uint8_t y, uint8_t theFormat);
void LCD_MultiLineH(uint8_t startx, uint8_t endx, uint64_t ymask);
uint8_t LCD_BMPDisplay(uint8_t* thebmp,uint8_t xoffset,uint8_t yoffset);
#endif
