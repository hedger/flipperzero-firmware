#include "spi_api.h"

#include <furi.h>
#include <furi-hal.h>

#include "furi-hal-gpio.h"

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

#define SPI_CMD_JEDEC_ID 0x9F

#define VENDOR_ID_CYPRESS 0x01
#define VENDOR_ID_FUJITSU 0x04
#define VENDOR_ID_EON 0x1C
#define VENDOR_ID_ATMEL 0x1F
#define VENDOR_ID_MICRON 0x20
#define VENDOR_ID_AMIC 0x37
#define VENDOR_ID_NOR_MEM 0x52
#define VENDOR_ID_SANYO 0x62
#define VENDOR_ID_INTEL 0x89
#define VENDOR_ID_ESMT 0x8C
#define VENDOR_ID_FUDAN 0xA1
#define VENDOR_ID_HYUNDAI 0xAD
#define VENDOR_ID_SST 0xBF
#define VENDOR_ID_MICRONIX 0xC2
#define VENDOR_ID_GIGADEVICE 0xC8
#define VENDOR_ID_ISSI 0xD5
#define VENDOR_ID_WINBOND 0xEF

typedef struct {
    const char* name;
    uint8_t id;
} VendorName_t;

static const VendorName_t VendorNames[] = {
    {"Cypress", VENDOR_ID_CYPRESS},
    {"Fujitsu", VENDOR_ID_FUJITSU},
    {"EON", VENDOR_ID_EON},
    {"Atmel", VENDOR_ID_ATMEL},
    {"Micron", VENDOR_ID_MICRON},
    {"AMIC", VENDOR_ID_AMIC},
    {"Sanyo", VENDOR_ID_SANYO},
    {"Intel", VENDOR_ID_INTEL},
    {"ESMT", VENDOR_ID_ESMT},
    {"Fudan", VENDOR_ID_FUDAN},
    {"Hyundai", VENDOR_ID_HYUNDAI},
    {"SST", VENDOR_ID_SST},
    {"GigaDevice", VENDOR_ID_GIGADEVICE},
    {"ISSI", VENDOR_ID_ISSI},
    {"Winbond", VENDOR_ID_WINBOND},
    {"Micronix", VENDOR_ID_MICRONIX},
    {"Nor-Mem", VENDOR_ID_NOR_MEM},
    {NULL, 0}};

typedef enum {
    CHIP_WM_PAGE_256B = 0x01, /**< write 1 to 256 bytes per page */
    CHIP_WM_BYTE = 0x02, /**< byte write */
    CHIP_WM_AAI = 0x04, /**< auto address increment */
    CHIP_WM_DUAL_BUFFER = 0x08, /**< dual-buffer write, like AT45DB series */
} SpiWriteMode_t;

typedef struct {
    const char* name; /**< flash chip name */
    uint8_t vendor_id; /**< vendor ID */
    uint8_t type_id; /**< memory type ID */
    uint8_t capacity_id; /**< capacity ID */
    uint32_t size; /**< flash capacity (bytes) */
    SpiWriteMode_t write_mode; /**< write mode */
    uint32_t erase_gran; /**< erase granularity (bytes) */
    uint8_t erase_gran_cmd; /**< erase granularity size block command */
} ChipInfo_t;

static const ChipInfo_t ChipInfos[] = {
    {"AT45DB161E",
     VENDOR_ID_ATMEL,
     0x26,
     0x00,
     2L * 1024L * 1024L,
     (SpiWriteMode_t)(CHIP_WM_BYTE | CHIP_WM_DUAL_BUFFER),
     512,
     0x81},
    {"W25Q40BV", VENDOR_ID_WINBOND, 0x40, 0x13, 512L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"W25Q16BV", VENDOR_ID_WINBOND, 0x40, 0x15, 2L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"W25Q32BV", VENDOR_ID_WINBOND, 0x40, 0x16, 4L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"W25Q64CV", VENDOR_ID_WINBOND, 0x40, 0x17, 8L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"W25Q64DW", VENDOR_ID_WINBOND, 0x60, 0x17, 8L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"W25Q128BV", VENDOR_ID_WINBOND, 0x40, 0x18, 16L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"W25Q256FV", VENDOR_ID_WINBOND, 0x40, 0x19, 32L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"SST25VF080B",
     VENDOR_ID_SST,
     0x25,
     0x8E,
     1L * 1024L * 1024L,
     (SpiWriteMode_t)(CHIP_WM_BYTE | CHIP_WM_AAI),
     4096,
     0x20},
    {"SST25VF016B",
     VENDOR_ID_SST,
     0x25,
     0x41,
     2L * 1024L * 1024L,
     (SpiWriteMode_t)(CHIP_WM_BYTE | CHIP_WM_AAI),
     4096,
     0x20},
    {"M25P32",
     VENDOR_ID_MICRON,
     0x20,
     0x16,
     4L * 1024L * 1024L,
     CHIP_WM_PAGE_256B,
     64L * 1024L,
     0xD8},
    {"M25P80",
     VENDOR_ID_MICRON,
     0x20,
     0x14,
     1L * 1024L * 1024L,
     CHIP_WM_PAGE_256B,
     64L * 1024L,
     0xD8},
    {"M25P40", VENDOR_ID_MICRON, 0x20, 0x13, 512L * 1024L, CHIP_WM_PAGE_256B, 64L * 1024L, 0xD8},
    {"EN25Q32B", VENDOR_ID_EON, 0x30, 0x16, 4L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"GD25Q64B",
     VENDOR_ID_GIGADEVICE,
     0x40,
     0x17,
     8L * 1024L * 1024L,
     CHIP_WM_PAGE_256B,
     4096,
     0x20},
    {"GD25Q16B",
     VENDOR_ID_GIGADEVICE,
     0x40,
     0x15,
     2L * 1024L * 1024L,
     CHIP_WM_PAGE_256B,
     4096,
     0x20},
    {"GD25Q32C",
     VENDOR_ID_GIGADEVICE,
     0x40,
     0x16,
     4L * 1024L * 1024L,
     CHIP_WM_PAGE_256B,
     4096,
     0x20},
    {"S25FL216K", VENDOR_ID_CYPRESS, 0x40, 0x15, 2L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"S25FL032P", VENDOR_ID_CYPRESS, 0x02, 0x15, 4L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"A25L080", VENDOR_ID_AMIC, 0x30, 0x14, 1L * 1024L * 1024L, CHIP_WM_PAGE_256B, 4096, 0x20},
    {"F25L004",
     VENDOR_ID_ESMT,
     0x20,
     0x13,
     512L * 1024L,
     (SpiWriteMode_t)(CHIP_WM_BYTE | CHIP_WM_AAI),
     4096,
     0x20},
    {"PCT25VF016B",
     VENDOR_ID_SST,
     0x25,
     0x41,
     2L * 1024L * 1024L,
     (SpiWriteMode_t)(CHIP_WM_BYTE | CHIP_WM_AAI),
     4096,
     0x20},
    {"NM25Q128EV",
     VENDOR_ID_NOR_MEM,
     0x21,
     0x18,
     16L * 1024L * 1024L,
     CHIP_WM_PAGE_256B,
     4096,
     0x20},
    {NULL, 0, 0, 0, 0}};

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
        if(write_read(SPI_CMD_JEDEC_ID, NULL, 0, id, 3)) {
            last_info.vendor_id = id[0];
            last_info.type_id = id[1];
            last_info.capacity_id = id[2];
            if(read_sfdp()) {
                // Use SFDP
            } else {
                const ChipInfo_t* chip = ChipInfos;
                while(chip->name != NULL) {
                    if(chip->vendor_id == id[0] && chip->type_id == id[1] &&
                       chip->capacity_id == id[2]) {
                        last_info.name = chip->name;
                        last_info.size = chip->size;
                        last_info.write_mode = chip->write_mode;
                        last_info.erase_gran = chip->erase_gran;
                        last_info.erase_gran_cmd = chip->erase_gran_cmd;
                        last_info.valid = true;
                        break;
                    }
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
    return last_info.valid;
}

bool SpiToolkit::chip_erase() {
    FURI_LOG_I(TAG, "Erasing chip");

    osDelay(4000);
    // TODO: chip_erase

    return true;
}

bool SpiToolkit::sector_erase(uint16_t n_sector) {
    FURI_LOG_I(TAG, "Erasing sector %d", n_sector);

    osDelay(1000);
    // TODO: sector_erase
    return true;
}

bool SpiToolkit::write_block(
    const size_t offset,
    const uint8_t* const p_data,
    const size_t data_len) {
    furi_assert(p_data);
    furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

    FURI_LOG_I(TAG, "Writing %d bytes @ %x", data_len, offset);

    osDelay(100);
    // TODO: write_block
    return true;
}

bool SpiToolkit::read_block(const size_t offset, uint8_t* const p_data, const size_t data_len) {
    furi_assert(p_data);
    furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

    FURI_LOG_I(TAG, "Reading %d bytes @ %x", data_len, offset);

    osDelay(10);
    // TODO: read_block
    FURI_LOG_I(TAG, "memset @ %x len %x", p_data, data_len);
    memset(p_data, 0xCD, data_len);

    return true;
}

SpiFlashInfo_t const* SpiToolkit::get_info() const {
    if(last_info.valid) {
        return &last_info;
    }

    return nullptr;
}