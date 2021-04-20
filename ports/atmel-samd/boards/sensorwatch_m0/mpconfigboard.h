// LEDs
#define MICROPY_HW_LED_STATUS   (&pin_PA03)

#define MICROPY_HW_BOARD_NAME "The Sensor Watch"
#define MICROPY_HW_MCU_NAME "saml22g18"

#define MICROPY_PORT_A        (0)
#define MICROPY_PORT_B        (0)
#define MICROPY_PORT_C        (0)

#define DEFAULT_I2C_BUS_SDA (&pin_PB30)
#define DEFAULT_I2C_BUS_SCL (&pin_PB31)

#define DEFAULT_SPI_BUS_SCK (&pin_PB01)
#define DEFAULT_SPI_BUS_MOSI (&pin_PB02)
#define DEFAULT_SPI_BUS_MISO (&pin_PB00)

#define DEFAULT_UART_BUS_TX (&pin_PB00)
#define DEFAULT_UART_BUS_RX (&pin_PB01)

// USB is always used internally so skip the pin objects for it.
#define IGNORE_PIN_PA24     1
#define IGNORE_PIN_PA25     1
