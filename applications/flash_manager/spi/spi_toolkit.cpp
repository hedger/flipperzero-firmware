#include "spi_toolkit.h"
#include "spi_chips.h"

#include "spi_impl.h"

#include <furi.h>

#define TAG "SPIAPI"

class SpiLock {
public:
    SpiLock() {
        spi_wrapper_acquire_bus();
    }

    ~SpiLock() {
        spi_wrapper_release_bus();
    }
};

SpiToolkit::SpiToolkit() {
    spi_wrapper_init();
}

SpiToolkit::~SpiToolkit() {
    disconnect();
}

/**
  * @brief Disconnect pins (set to Analog state)
  */
void SpiToolkit::disconnect() {
    spi_wrapper_deinit();
}

#define SUPPORT_MAX_SFDP_MAJOR_REV 1
/* the JEDEC basic flash parameter table length is 9 DWORDs (288-bit) on JESD216 (V1.0) initial release standard */
#define BASIC_TABLE_LEN 9

static bool read_sfdp(struct SpiFlashInfo_t* info) {
    /* The SFDP header is located at address 000000h of the SFDP data structure.
     * It identifies the SFDP Signature, the number of parameter headers, and the SFDP revision numbers. */
    /* sfdp parameter header address */
    /* each parameter header being 2 DWORDs (64-bit) */
    uint8_t header[2 * 4] = {0};
    /* read SFDP header */
    const uint8_t cmd1[] = {0, 0, 0, 0xFF};

    if(!spi_wrapper_write_read(
           SpiChipCommand_READ_SFDP_REGISTER,
           (uint8_t*)&cmd1[0],
           sizeof(cmd1),
           header,
           sizeof(header)))
        return false;

    /* check SFDP header */
    if(!(header[0] == 'S' && header[1] == 'F' && header[2] == 'D' && header[3] == 'P'))
        // Error: Check SFDP signature error. It's must be 50444653h('S' 'F' 'D' 'P').
        return false;

    info->minor_rev = header[4];
    info->major_rev = header[5];
    if(info->major_rev > SUPPORT_MAX_SFDP_MAJOR_REV) {
        // Error: This reversion SFDP is not supported.
        return false;
    }

    const uint8_t cmd2[] = {0, 0, 0x08, 0xFF};
    if(!spi_wrapper_write_read(
           SpiChipCommand_READ_SFDP_REGISTER,
           (uint8_t*)&cmd2[0],
           sizeof(cmd2),
           header,
           sizeof(header)))
        return false;

    // uint8_t id = header[0];
    info->minor_rev = header[1];
    info->major_rev = header[2];
    uint8_t len = header[3];
    uint32_t table_addr = (long)header[4] | (long)header[5] << 8 | (long)header[6] << 16;
    /* check JEDEC basic flash parameter header */
    if(info->major_rev > SUPPORT_MAX_SFDP_MAJOR_REV)
        // "Error: This reversion JEDEC basic flash parameter header is not supported.",
        return false;

    if(len < BASIC_TABLE_LEN)
        // "Error: The JEDEC basic flash parameter table length error.",
        return false;

    /* parameter table */
    uint8_t table[BASIC_TABLE_LEN * 4] = {0}, i, j;
    uint8_t cmd3[] = {
        (uint8_t)((table_addr >> 16) & 0xFF),
        (uint8_t)((table_addr >> 8) & 0xFF),
        (uint8_t)((table_addr >> 0) & 0xFF),
        0xFF};
    if(!spi_wrapper_write_read(
           SpiChipCommand_READ_SFDP_REGISTER, &cmd3[0], sizeof(cmd3), table, sizeof(table)))
        return false;

    info->erase_4k_cmd = table[1];
    switch(table[0] & 0x03) {
    case 1:
        // 4 KB Erase is supported throughout the device.
        info->erase_4k = true;
        break;
    case 3:
        // Uniform 4 KB erase is unavailable for this device.
        info->erase_4k = false;
        break;
    default:
        // Error: Uniform 4 KB erase supported information error.
        return false;
    }

    switch((table[0] & (0x01 << 2)) >> 2) {
    case 0:
        // Write granularity is 1 byte.
        info->write_gran = 1;
        break;
    case 1:
        // Write granularity is 64 bytes or larger.
        info->write_gran = 256;
        break;
    }

    /* volatile status register block protect bits */
    switch((table[0] & (0x01 << 3)) >> 3) {
    case 0:
        /* Block Protect bits in device's status register are solely non-volatile or may be
         * programmed either as volatile using the 50h instruction for write enable or non-volatile
         * using the 06h instruction for write enable.
         */
        // Target flash status register is non-volatile.
        info->sr_is_non_vola = true;
        break;
    case 1:
        /* block protect bits in device's status register are solely volatile. */
        // Block Protect bits in device's status register are solely volatile.
        info->sr_is_non_vola = false;
        /* write enable instruction select for writing to volatile status register */
        switch((table[0] & (0x01 << 4)) >> 4) {
        case 0:
            // Flash device requires instruction 50h as the write enable prior
            // to performing a volatile write to the status register.
            info->vola_sr_we_cmd = SpiChipCommand_VOLATILE_SR_WRITE_ENABLE;
            break;
        case 1:
            // Flash device requires instruction 06h as the write enable prior
            // to performing a volatile write to the status register.
            info->vola_sr_we_cmd = SpiChipCommand_WRITE_ENABLE;
            break;
        }
        break;
    }

    /* get address bytes, number of bytes used in addressing flash array read, write and erase. */
    switch((table[2] & (0x03 << 1)) >> 1) {
    case 0:
        // 3-Byte only addressing.
        info->addr_3_byte = true;
        info->addr_4_byte = false;
        break;
    case 1:
        // 3- or 4-Byte addressing.
        info->addr_3_byte = true;
        info->addr_4_byte = true;
        break;
    case 2:
        // 4-Byte only addressing.
        info->addr_3_byte = false;
        info->addr_4_byte = true;
        break;
    default:
        // Error: Read address bytes error!
        info->addr_3_byte = false;
        info->addr_4_byte = false;
        return false;
    }
    /* get flash memory capacity */
    uint32_t table2_temp = ((long)table[7] << 24) | ((long)table[6] << 16) |
                           ((long)table[5] << 8) | (long)table[4];
    switch((table[7] & (0x01 << 7)) >> 7) {
    case 0:
        info->size = 1 + (table2_temp >> 3);
        break;
    case 1:
        table2_temp &= 0x7FFFFFFF;
        if(table2_temp > sizeof(info->size) * 8 + 3) {
            // Error: The flash capacity is grater than 32 Gb/ 4 GB! Not Supported.
            info->size = 0;
            return false;
        }
        info->size = 1L << (table2_temp - 3);
        break;
    }

    /* get erase size and erase command  */
    for(i = 0, j = 0; i < SFDP_ERASE_TYPE_MAX_NUM; i++) {
        if(table[28 + 2 * i] != 0x00) {
            info->eraser[j].size = 1L << table[28 + 2 * i];
            info->eraser[j].cmd = table[28 + 2 * i + 1];
            j++;
        }
    }
    /* sort the eraser size from small to large */
    for(i = 0, j = 0; i < SFDP_ERASE_TYPE_MAX_NUM; i++) {
        if(info->eraser[i].size) {
            for(j = i + 1; j < SFDP_ERASE_TYPE_MAX_NUM; j++) {
                if(info->eraser[j].size != 0 && info->eraser[i].size > info->eraser[j].size) {
                    /* swap the small eraser */
                    uint32_t temp_size = info->eraser[i].size;
                    uint8_t temp_cmd = info->eraser[i].cmd;
                    info->eraser[i].size = info->eraser[j].size;
                    info->eraser[i].cmd = info->eraser[j].cmd;
                    info->eraser[j].size = temp_size;
                    info->eraser[j].cmd = temp_cmd;
                }
            }
        }
    }
    return true;
}

bool SpiToolkit::detect_flash() {
    SpiLock lock;
#ifdef FLASHMGR_MOCK
    osDelay(1500);
    last_info.vendor_id = SpiChipVendor_WINBOND;
    last_info.name = "W25QMOCK";
    last_info.size = 64 * 1024L;
    last_info.write_mode = CHIP_WM_PAGE_256B;
    last_info.erase_gran = 4096;
    last_info.erase_gran_cmd = 0x20;
    last_info.valid = true;

    return true;
#endif // FLASHMGR_MOCK
    last_info.valid = false;

    for(;;) {
        uint8_t id[3];
        if(spi_wrapper_write_read(SpiChipCommand_JEDEC_ID, NULL, 0, id, 3)) {
            last_info.vendor_id = id[0];
            last_info.type_id = id[1];
            last_info.capacity_id = id[2];
            if(read_sfdp(&last_info)) {
                last_info.valid = true;
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
        spi_wrapper_deinit();
    }
    return last_info.valid;
}

bool SpiToolkit::chip_erase() {
    SpiLock lock;

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
    SpiLock lock;

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

    SpiLock lock;

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
    SpiLock lock;

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