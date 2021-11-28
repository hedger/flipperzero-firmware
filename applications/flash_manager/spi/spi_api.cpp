#include "spi_api.h"
#include "spi_chips.h"

#include <furi.h>
#include <furi-hal.h>

#include <furi-hal-gpio.h>

#define TAG "SPIAPI"

SpiToolkit::SpiToolkit() {
}

SpiToolkit::~SpiToolkit() {
    disconnect();
}

static GpioPin SPI_MOSI = {GPIOA, GPIO_PIN_7};
static GpioPin SPI_MISO = {GPIOA, GPIO_PIN_6};
static GpioPin SPI_SCK = {GPIOB, GPIO_PIN_3};
static GpioPin SPI_CS = {GPIOA, GPIO_PIN_4};

static inline void delay() {
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static bool CS_LOW() {
    hal_gpio_write(&SPI_CS, false);
    delay();
    return (hal_gpio_read(&SPI_CS) == false);
}

static bool CS_HIGH() {
    hal_gpio_write(&SPI_CS, true);
    delay();
    return hal_gpio_read(&SPI_CS);
}

static bool SCK_LOW() {
    hal_gpio_write(&SPI_SCK, false);
    delay();
    return (hal_gpio_read(&SPI_SCK) == false);
}

static bool SCK_HIGH() {
    hal_gpio_write(&SPI_SCK, true);
    delay();
    return hal_gpio_read(&SPI_SCK);
}
static bool MOSI_LOW() {
    hal_gpio_write(&SPI_MOSI, false);
    delay();
    return (hal_gpio_read(&SPI_MOSI) == false);
}
static bool MOSI_HIGH() {
    hal_gpio_write(&SPI_MOSI, true);
    delay();
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

static bool write(uint8_t opCode) {
    bool result = CS_LOW();
    if(result) {
        result &= writeByte(opCode);
    }
    result &= CS_HIGH();
    return result;
}

static bool write(uint8_t opCode, uint8_t* data, int len) {
    bool result = CS_LOW();
    if(result) {
        result &= writeByte(opCode);
        if(result) {
            while(len != 0) {
                result &= writeByte(*data++);
                if(!result) break;
                --len;
            }
        }
        result &= CS_HIGH();
    }
    return result;
}

static bool write_read(
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

/**
  * @brief Disconnect pins (set to Analog state)
  */
void SpiToolkit::disconnect() {
    hal_gpio_init(&SPI_CS, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_SCK, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MOSI, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    hal_gpio_init(&SPI_MISO, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
}

static bool read_sfdp() {
    return false;
}

bool SpiToolkit::detect_flash() {
    last_info.valid = false;

    for(;;) {
        // Check pins
        hal_gpio_init(&SPI_CS, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
        hal_gpio_init(&SPI_SCK, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
        hal_gpio_init(&SPI_MOSI, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
        hal_gpio_init(&SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);
        if(read_pins() != 0) break;

        hal_gpio_init(&SPI_CS, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
        if(read_pins() != SPI_PIN_CS) break;

        hal_gpio_init(&SPI_SCK, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
        if(read_pins() != (SPI_PIN_CS | SPI_PIN_SCK)) break;
        hal_gpio_init(&SPI_SCK, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);

        hal_gpio_init(&SPI_MOSI, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
        if(read_pins() != (SPI_PIN_CS | SPI_PIN_MOSI)) break;
        hal_gpio_init(&SPI_MOSI, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh);

        hal_gpio_init(&SPI_MISO, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh);
        if(read_pins() != (SPI_PIN_CS | SPI_PIN_MISO)) break;
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

        uint8_t id[3];
        if(write_read(SpiChipCommand_JEDEC_ID, NULL, 0, id, 3)) {
            last_info.vendor_id = id[0];
            last_info.type_id = id[1];
            last_info.capacity_id = id[2];
            if(read_sfdp()) {
                // Use SFDP
            } else {
                const ChipInfo_t* chip = spi_chip_get_details(id[0], id[1], id[2]);
                if(chip != nullptr) {
                    last_info.name = chip->name;
                    last_info.size = chip->size;
                    last_info.write_mode = chip->write_mode;
                    last_info.erase_gran = chip->erase_gran;
                    last_info.erase_gran_cmd = chip->erase_gran_cmd;
                    last_info.valid = true;
                }
            }
            break;
        }
    }
    if(!last_info.valid) {
        // Chip access error or unknown type
        hal_gpio_init(&SPI_CS, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
        hal_gpio_init(&SPI_SCK, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
        hal_gpio_init(&SPI_MOSI, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
        hal_gpio_init(&SPI_MISO, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh);
    }
#ifdef FLASHMGR_MOCK
    {
        last_info.vendor_id = SpiChipVendor_WINBOND;
        last_info.name = "W25QMOCK";
        last_info.size = 256 * 1024L;
        last_info.write_mode = CHIP_WM_PAGE_256B;
        last_info.erase_gran = 4096;
        last_info.erase_gran_cmd = 0x20;
        last_info.valid = true;
    }
#endif // FLASHMGR_MOCK
    return last_info.valid;
}

bool SpiToolkit::chip_erase() {
    FURI_LOG_I(TAG, "Erasing chip");

#ifdef FLASHMGR_MOCK
    osDelay(4000);
    // TODO: chip_erase
    return true;
#else // FLASHMGR_MOCK
    return false;
#endif // FLASHMGR_MOCK
}

bool SpiToolkit::sector_erase(uint16_t n_sector) {
    FURI_LOG_I(TAG, "Erasing sector %d", n_sector);

#ifdef FLASHMGR_MOCK
    osDelay(1000);
    // TODO: sector_erase
    return true;
#else // FLASHMGR_MOCK
    return false;
#endif // FLASHMGR_MOCK
}

bool SpiToolkit::write_block(
    const size_t offset,
    const uint8_t* const p_data,
    const size_t data_len) {
    furi_assert(p_data);
    furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

    FURI_LOG_I(TAG, "Writing %d bytes @ %x", data_len, offset);

#ifdef FLASHMGR_MOCK
    osDelay(100);
    // TODO: write_block
    return true;
#else // FLASHMGR_MOCK
    return false;
#endif // FLASHMGR_MOCK
}

bool SpiToolkit::read_block(const size_t offset, uint8_t* const p_data, const size_t data_len) {
    furi_assert(p_data);
    furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

    FURI_LOG_I(TAG, "Reading %d bytes @ %x", data_len, offset);

#ifdef FLASHMGR_MOCK
    osDelay(10);
    // TODO: read_block
    //FURI_LOG_I(TAG, "memset @ %x len %x", p_data, data_len);
    memset(p_data, 0xCD, data_len);

    return true;
#else // FLASHMGR_MOCK
    return false;
#endif // FLASHMGR_MOCK
}

SpiFlashInfo_t const* SpiToolkit::get_info() const {
    if(last_info.valid) {
        return &last_info;
    }

    return nullptr;
}