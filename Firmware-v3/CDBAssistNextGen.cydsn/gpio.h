#ifndef GPIO_H_
#define GPIO_H_

typedef enum ePins {
    NO_PIN = 0,
    BTNA_PIN1,
    BTNA_PIN2,
    BTNB_PIN3,
    BTNB_PIN4,
    BTNC_PIN5,
    BTNC_PIN6,
    TREF_PIN7,
    DFMS_PIN8,
    DTMS_PIN9,
    USBDP,
    USBDN,
    USBID,
    SPARESIO,
    NUM_PINS
} Pins_t;

typedef enum ePinStates {
    PIN_HIZ_INPUT = 0,
    PIN_OUT_HIGH,
    PIN_OUT_LOW,
    PIN_UART_RX,
    PIN_UART_TX,
    PIN_NUM_STATES
} PinStates_t;

void GPIO_Init();
void Buttons_Enable(uint8_t idx);
void Buttons_Disable(uint8_t idx);
void GPIO_SetPinState( Pins_t id, PinStates_t state );
void GPIO_SetTXDrive(uint8_t onoff);

#endif // GPIO_H_
