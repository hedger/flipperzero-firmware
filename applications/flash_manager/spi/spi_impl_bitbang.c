#include "spi_impl.h"

#ifdef FLASMMGR_SPI_BITBANG

#include <furi.h>
#include <furi-hal.h>

#include <furi-hal-gpio.h>


static GpioPin SPI_MOSI = {GPIOA, GPIO_PIN_7};
static GpioPin SPI_MISO = {GPIOA, GPIO_PIN_6};
static GpioPin SPI_SCK = {GPIOB, GPIO_PIN_3};
static GpioPin SPI_CS = {GPIOA, GPIO_PIN_4};

static inline void _spi_delay() {
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static bool CS_LOW() {
    hal_gpio_write(&SPI_CS, false);
    _spi_delay();
    return (hal_gpio_read(&SPI_CS) == false);
}

static bool CS_HIGH() {
    hal_gpio_write(&SPI_CS, true);
    _spi_delay();
    return hal_gpio_read(&SPI_CS);
}

static bool SCK_LOW() {
    hal_gpio_write(&SPI_SCK, false);
    _spi_delay();
    return (hal_gpio_read(&SPI_SCK) == false);
}

static bool SCK_HIGH() {
    hal_gpio_write(&SPI_SCK, true);
    _spi_delay();
    return hal_gpio_read(&SPI_SCK);
}
static bool MOSI_LOW() {
    hal_gpio_write(&SPI_MOSI, false);
    _spi_delay();
    return (hal_gpio_read(&SPI_MOSI) == false);
}
static bool MOSI_HIGH() {
    hal_gpio_write(&SPI_MOSI, true);
    _spi_delay();
    return hal_gpio_read(&SPI_MOSI);
}
static inline bool MISO_READ() {
    return hal_gpio_read(&SPI_MISO);
}

#define SPI_PIN_MOSI 0x01
#define SPI_PIN_MISO 0x02
#define SPI_PIN_SCK 0x04
#define SPI_PIN_CS 0x08

static uint32_t read_pins(void) {
    uint32_t pins = 0;
    if(hal_gpio_read(&SPI_MOSI)) pins |= SPI_PIN_MOSI;
    if(hal_gpio_read(&SPI_MISO)) pins |= SPI_PIN_MISO;
    if(hal_gpio_read(&SPI_SCK)) pins |= SPI_PIN_SCK;
    if(hal_gpio_read(&SPI_CS)) pins |= SPI_PIN_CS;
    return pins;
}

static bool writeByte(uint8_t data) {
    uint8_t mask = 0x80;
    bool result = true;
    while(mask != 0) {
        if((data & mask) == 0)
            result &= MOSI_LOW();
        else
            result &= MOSI_HIGH();
        result &= SCK_HIGH();
        mask >>= 1;
        result &= SCK_LOW();
    }
    return result;
}

static bool readByte(uint8_t* dst) {
    uint8_t mask = 0x80;
    uint8_t data = 0;

    bool result = true;
    while(mask != 0) {
        result &= SCK_HIGH();
        if(MISO_READ()) data |= mask;
        mask >>= 1;
        result &= SCK_LOW();
    }
    if(result) *dst = data;
    return result;
}

bool write_read(
    uint8_t opCode,
    uint8_t* write_data,
    int write_len,
    uint8_t* read_data,
    int read_len) {
    bool result = CS_LOW();
    if(result) {
        result &= writeByte(opCode);
        if(result) {
            while(write_len != 0) {
                result &= writeByte(*write_data++);
                if(!result) break;
                --write_len;
            }
        }
        if(result) {
            while(read_len != 0) {
                result &= readByte(read_data++);
                if(!result) break;
                --read_len;
            }
        }
    }
    result &= CS_HIGH();
    return result;
}

//static bool write(uint8_t opCode) {
//    bool result = CS_LOW();
//    if(result) {
//        result &= writeByte(opCode);
//    }
//    result &= CS_HIGH();
//    return result;
//}

//static bool write(uint8_t opCode, uint8_t* data, int len) {
//    bool result = CS_LOW();
//    if(result) {
//        result &= writeByte(opCode);
//        if(result) {
//            while(len != 0) {
//                result &= writeByte(*data++);
//                if(!result) break;
//                --len;
//            }
//        }
//        result &= CS_HIGH();
//    }
//    return result;
//}

bool spi_wrapper_init() {
    // Check pins
    hal_gpio_init(&SPI_CS, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_SCK, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MOSI, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
    if(read_pins() != 0) return false;

    hal_gpio_init(&SPI_CS, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
    if(read_pins() != SPI_PIN_CS) return false;

    hal_gpio_init(&SPI_SCK, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
    if(read_pins() != (SPI_PIN_CS | SPI_PIN_SCK)) return false;
    hal_gpio_init(&SPI_SCK, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);

    hal_gpio_init(&SPI_MOSI, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
    if(read_pins() != (SPI_PIN_CS | SPI_PIN_MOSI)) return false;
    hal_gpio_init(&SPI_MOSI, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);

    hal_gpio_init(&SPI_MISO, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
    if(read_pins() != (SPI_PIN_CS | SPI_PIN_MISO)) return false;
    hal_gpio_init(&SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);

    // Prepare pins state
    hal_gpio_write(&SPI_CS, true);
    hal_gpio_write(&SPI_SCK, false);
    hal_gpio_write(&SPI_MOSI, false);
    hal_gpio_write(&SPI_MISO, false);
    // Initialize pins
    hal_gpio_init(&SPI_CS, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_SCK, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MOSI, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);

    return true;
}

void spi_wrapper_deinit() {
    hal_gpio_init(&SPI_CS, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_SCK, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MOSI, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MISO, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
}

#endif // FLASMMGR_SPI_BITBANG