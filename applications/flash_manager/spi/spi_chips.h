#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SpiChipCommand_PAGE_PROGRAM = 0x02,
    SpiChipCommand_READ_DATA = 0x03,
    SpiChipCommand_WRITE_DISABLE = 0x04,
    SpiChipCommand_READ_STATUS = 0x05,
    SpiChipCommand_WRITE_STATUS_REG_1 = 0x01,
    SpiChipCommand_WRITE_STATUS_REG_2 = 0x31, // TODO: check compliancy
    SpiChipCommand_WRITE_ENABLE = 0x06,
    SpiChipCommand_VOLATILE_SR_WRITE_ENABLE = 0x50,
    SpiChipCommand_READ_SFDP_REGISTER = 0x5A,
    SpiChipCommand_JEDEC_ID = 0x9F,
    SpiChipCommand_RELEASE_DEEP = 0xAB,
    SpiChipCommand_AAI_WORD_PROGRAM = 0xAD,
    SpiChipCommand_ERASE_CHIP = 0xC7,
} SpiChipCommand;

enum {
    SpiStatusRegister_BUSY = (1 << 0), /**< busy */
    SpiStatusRegister_WEL = (1 << 1), /**< write enable latch */
    SpiStatusRegister_SWP0 = (1 << 2), /**< Software protection status */
    SpiStatusRegister_SWP1 = (1 << 3), /**< Software protection status */
    SpiStatusRegister_WP_STATUS = (1 << 4), /**< WRITE PROTECT Pin status */
    SpiStatusRegister_EPE = (1 << 5), /**< Erase Program Error */
    SpiStatusRegister_RES = (1 << 6), /**< Reserved */
    SpiStatusRegister_SRP = (1 << 7), /**< status register protect */
};

typedef enum {
    SpiChipVendor_CYPRESS = 0x01,
    SpiChipVendor_FUJITSU = 0x04,
    SpiChipVendor_EON = 0x1C,
    SpiChipVendor_ATMEL = 0x1F,
    SpiChipVendor_MICRON = 0x20,
    SpiChipVendor_AMIC = 0x37,
    SpiChipVendor_NOR_MEM = 0x52,
    SpiChipVendor_SANYO = 0x62,
    SpiChipVendor_INTEL = 0x89,
    SpiChipVendor_ESMT = 0x8C,
    SpiChipVendor_FUDAN = 0xA1,
    SpiChipVendor_HYUNDAI = 0xAD,
    SpiChipVendor_SST = 0xBF,
    SpiChipVendor_MICRONIX = 0xC2,
    SpiChipVendor_GIGADEVICE = 0xC8,
    SpiChipVendor_ISSI = 0xD5,
    SpiChipVendor_WINBOND = 0xEF,
    SpiChipVendor_BOYA = 0x68,
} SpiChipVendor;

typedef struct {
    const char* name;
    const uint8_t id;
} VendorName_t;

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

extern const VendorName_t VendorNames[];
extern const ChipInfo_t ChipInfos[];

const char* spi_vendor_get_name(const uint8_t code);
const ChipInfo_t* spi_chip_get_details(const uint8_t vid, const uint8_t tid, const uint8_t capid);

#ifdef __cplusplus
}
#endif