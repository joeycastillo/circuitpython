#define MICROPY_HW_BOARD_NAME "Arduino Zero"
#define MICROPY_HW_MCU_NAME "samd21g18"

#define MICROPY_HW_LED_STATUS   (&pin_PA17)

#define MICROPY_HW_LED_TX   &pin_PA27
#define MICROPY_HW_LED_RX   &pin_PB03

#define MICROPY_PORT_A        (PORT_PA27)
#define MICROPY_PORT_B        (PORT_PB03)
#define MICROPY_PORT_C        (0)

#define DEFAULT_I2C_BUS_SCL (&pin_PA23)
#define DEFAULT_I2C_BUS_SDA (&pin_PA22)

#define DEFAULT_SPI_BUS_SCK (&pin_PB11)
#define DEFAULT_SPI_BUS_MOSI (&pin_PB10)
#define DEFAULT_SPI_BUS_MISO (&pin_PA12)

#define DEFAULT_UART_BUS_RX (&pin_PA11)
#define DEFAULT_UART_BUS_TX (&pin_PA10)

// USB is always used internally so skip the pin objects for it.
#define IGNORE_PIN_PA24     1
#define IGNORE_PIN_PA25     1
