USB_VID = 0x239A
USB_PID = 0x807E
USB_PRODUCT = "The Sensor Cap"
USB_MANUFACTURER = "Oddly Specific Objects"

CHIP_VARIANT = SAMD51G19A
CHIP_FAMILY = samd51

QSPI_FLASH_FILESYSTEM = 1
EXTERNAL_FLASH_DEVICE_COUNT = 1
EXTERNAL_FLASH_DEVICES = GD25Q32C
LONGINT_IMPL = MPZ

CIRCUITPY_AUDIOBUSIO = 0

FROZEN_MPY_DIRS += $(TOP)/frozen/Adafruit_CircuitPython_BusDevice
FROZEN_MPY_DIRS += $(TOP)/frozen/Adafruit_CircuitPython_Register
FROZEN_MPY_DIRS += $(TOP)/frozen/Adafruit_CircuitPython_DS3231
