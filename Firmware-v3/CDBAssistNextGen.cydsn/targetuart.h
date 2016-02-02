#ifndef TARGETUART_H
#define TARGETUART_H

void targetuart_putc(char c);
void targetuart_flushbuffer(void); // Called periodically to make sure data doesn't become stale while trying to fill the buffers
void targetuart_init(void);
void targetuart_breakctl(uint8_t enable);
void targetuart_config(uint32_t baudrate, uint8_t parity, uint8_t databits, uint8_t stopbits);

#endif // TARGETUART_H
