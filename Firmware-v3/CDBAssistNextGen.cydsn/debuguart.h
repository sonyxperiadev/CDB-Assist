#ifndef DEBUGUART_H
#define DEBUGUART_H

#if 0
#define ENABLE_DEBUG
//#define ENABLE_VERBOSE
//#define ENABLE_REALLYVERBOSE

#if defined(ENABLE_DEBUG)
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#if defined(ENABLE_VERBOSE)
#define VERBOSE(x) x
#else
#define VERBOSE(x)
#endif

#if defined(ENABLE_REALLYVERBOSE)
#define REALLYVERBOSE(x) x
#else
#define REALLYVERBOSE(x)
#endif
#endif

int uart1_putc(char c);
void debuguart1_init(void);
uint16_t debugring_get_rx_bytes_avail(void);
uint8_t* debugring_get_rx_ptr(void);
uint8_t* debugring_get_buf_end_ptr(void);
uint8_t* debugring_get_buf_start_ptr(void);
void debugring_advance_rx_ptr(uint16_t theAmount);

#endif // DEBUGUART_H
