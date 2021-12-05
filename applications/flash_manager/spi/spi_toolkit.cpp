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

static bool read_status(uint8_t* const status) {
    bool success = spi_wrapper_write_read(SpiChipCommand_READ_STATUS, NULL, 0, status, 1);
    FURI_LOG_I(TAG, "Status reg = %02X", *status);
    return success;
}

static bool release_deep_sleep() {
    if(spi_wrapper_write_read(SpiChipCommand_RELEASE_DEEP, NULL, 0, NULL, 0)) {
        osDelay(1);
        return true;
    }
    return false;
}

static bool wait_busy(uint32_t wait_in_10ticks) {
    uint8_t status;
    bool result = false;

    while(wait_in_10ticks != 0) {
        result = read_status(&status);
        if(result && ((status & SpiStatusRegister_BUSY) == 0)) {
            break;
        }
        osDelay(10);
        --wait_in_10ticks;
    }
    if (wait_in_10ticks == 0) {
        FURI_LOG_I(TAG, "wait_busy() elapsed!");
    }
    return (result && ((status & SpiStatusRegister_BUSY) == 0));
}

static bool set_write_enabled(bool enabled) {
    FURI_LOG_I(TAG, "Setting WE to %d", enabled);

    bool result = true;
    uint8_t status, cmd = enabled ? SpiChipCommand_WRITE_ENABLE : SpiChipCommand_WRITE_DISABLE;
    result = spi_wrapper_write_read(cmd, NULL, 0, NULL, 0);
    if(result) {
        result = read_status(&status);
    }

    result = spi_wrapper_write_read(SpiChipCommand_VOLATILE_SR_WRITE_ENABLE, NULL, 0, NULL, 0);
    if(result) {
        result = read_status(&status);
    }

    // FIXME: respect 'enabled' mode
    //status &= ~(SpiStatusRegister_WEL | SpiStatusRegister_SWP0 | SpiStatusRegister_SWP1 | SpiStatusRegister_WP_STATUS);
    //result &= spi_wrapper_write_read(SpiChipCommand_WRITE_STATUS_REG_1, &status, 1, NULL, 0);
    //if (status & (SpiStatusRegister_SWP0 | SpiStatusRegister_SWP1)) {
    //    FURI_LOG_I(TAG, "Resetting SWP01");
    //    uint8_t zero_status = 0;
    //    result &= spi_wrapper_write_read(SpiChipCommand_WRITE_STATUS_REG_1, &zero_status, 1, NULL, 0);
    //}

    if(result) {
        FURI_LOG_I(TAG, "Updated status reg");
        result = read_status(&status);
    }

    bool is_currently_protected = ((status & SpiStatusRegister_WEL) == 0);

    FURI_LOG_I(TAG, "Requested protection: %d, effective protection: %d", enabled, is_currently_protected);

    if(result) {
        if(enabled && is_currently_protected) {
            // Can't enable write status.
            return false;
        } else if(!enabled && !is_currently_protected) {
            // Can't disable write status.
            return false;
        }
    }
    return result;
}

static int make_adress_byte_array(struct SpiFlashInfo_t* info, uint32_t addr, uint8_t* array) {
    int len, i;
    len = (!info->addr_3_byte && info->addr_4_byte) ? 4 : 3;
    for(i = 0; i < len; i++) {
        array[i] = (addr >> ((len - (i + 1)) * 8)) & 0xFF;
    }
    return len;
}

static bool page256_or_1_byte_write(
    struct SpiFlashInfo_t* info,
    uint32_t addr,
    size_t size,
    uint16_t write_gran,
    uint8_t* data) {
    furi_assert(write_gran == 1 || write_gran == 256);
    bool result = false;
    static uint8_t cmd_data[4 + SpiToolkit::SPI_MAX_BLOCK_SIZE];
    size_t data_size;

    while(size) {
        /* set the flash write enable */
        result = set_write_enabled(true);
        if(!result) break;

        int cmd_size = make_adress_byte_array(info, addr, cmd_data);
        /* make write align and calculate next write address */
        if(addr % write_gran != 0) {
            if(size > write_gran - (addr % write_gran)) {
                data_size = write_gran - (addr % write_gran);
            } else {
                data_size = size;
            }
        } else {
            if(size > write_gran) {
                data_size = write_gran;
            } else {
                data_size = size;
            }
        }
        size -= data_size;
        addr += data_size;
        memcpy(&cmd_data[cmd_size], data, data_size);
        data += data_size;

        result = spi_wrapper_write_read(
            SpiChipCommand_PAGE_PROGRAM, cmd_data, cmd_size + data_size, NULL, 0);
        if(!result) break;
        result = wait_busy(10);
        if(!result) break;
    }
    // FIXME
    //set_write_enabled(false);
    return result;
}

static bool aai_write(struct SpiFlashInfo_t* info, uint32_t addr, size_t size, uint8_t* data) {
    bool result = false;

    /* The address must be even for AAI write mode. So it must write one byte first when address is odd. */
    if(addr % 2 != 0) {
        result = page256_or_1_byte_write(info, addr++, 1, 1, data++);
        if(!result) {
            set_write_enabled(false);
            return false;
        }
        size--;
    }

    result = set_write_enabled(true);
    if(!result) {
        set_write_enabled(false);
        return false;
    }

    uint8_t cmd_data[8], cmd_size;
    cmd_size = make_adress_byte_array(info, addr, cmd_data);
    while(size >= 2) {
        cmd_data[cmd_size] = *data;
        cmd_data[cmd_size + 1] = *(data + 1);

        result = spi_wrapper_write_read(
            SpiChipCommand_AAI_WORD_PROGRAM, cmd_data, cmd_size + 2, NULL, 0);
        if(!result) break;
        result = wait_busy(20);
        if(!result) break;
        cmd_size = 0;
        size -= 2;
        addr += 2;
        data += 2;
    }
    /* set the flash write disable for exit AAI mode */
    result = set_write_enabled(false);
    /* write last one byte data when origin write size is odd */
    if(result && size == 1) {
        result = page256_or_1_byte_write(info, addr, 1, 1, data);
    }

    if(!result) {
        set_write_enabled(false);
    }
    return result;
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

    memset(&last_info, 0, sizeof(last_info));

    for(;;) {
        uint8_t id[3];
        if(spi_wrapper_write_read(SpiChipCommand_JEDEC_ID, NULL, 0, id, 3)) {
            FURI_LOG_I(TAG, "RAW CHIP ID: %02X %02X %02X", id[0], id[1], id[2]);

            last_info.vendor_id = id[0];
            last_info.type_id = id[1];
            last_info.capacity_id = id[2];
            last_info.write_mode = CHIP_WM_PAGE_256B;

            const ChipInfo_t* chip = spi_chip_get_details(id[0], id[1], id[2]);
            if(chip != nullptr) {
                last_info.name = chip->name;
                last_info.size = chip->size;
                last_info.write_mode = chip->write_mode;
                last_info.erase_gran = chip->erase_gran;
                last_info.erase_gran_cmd = chip->erase_gran_cmd;
                last_info.valid = true;
            }

            if(read_sfdp(&last_info)) {
                FURI_LOG_I(TAG, "SFDP OK");
                last_info.valid = true;
            } else if(chip != nullptr) {
                FURI_LOG_I(TAG, "SFDP Failed, use table info");
                last_info.name = chip->name;
                last_info.size = chip->size;
                last_info.write_mode = chip->write_mode;
                last_info.erase_gran = chip->erase_gran;
                last_info.erase_gran_cmd = chip->erase_gran_cmd;
                last_info.valid = true;
            } else {
                FURI_LOG_I(TAG, "Can't define chip type");
            }
            break;
        }
    }
    //if(!last_info.valid) {
    //    // Chip access error or unknown type
    //    //spi_wrapper_deinit();
    //}
    return last_info.valid;
}

bool SpiToolkit::chip_erase() {
    FURI_LOG_I(TAG, "Erasing chip");

#ifdef FLASHMGR_MOCK
    osDelay(4000);
    // TODO: chip_erase
    return true;
#endif // FLASHMGR_MOCK

    bool result = true;

    if(last_info.valid) {
        SpiLock lock;
        if(release_deep_sleep() && wait_busy(10)) {
            if(set_write_enabled(true)) {
                if(last_info.write_mode & CHIP_WM_DUAL_BUFFER) {
                    uint8_t cmd[3] = {0x94, 0x80, 0x9A};
                    result = spi_wrapper_write_read(SpiChipCommand_ERASE_CHIP, cmd, 3, NULL, 0);
                } else {
                    result = spi_wrapper_write_read(SpiChipCommand_ERASE_CHIP, NULL, 0, NULL, 0);
                }
                if(result) {
                    result = wait_busy(2 * 60 * 100); // FIXME!
                    set_write_enabled(false);
                    result &= wait_busy(2 * 60 * 100);
                    if(!result) {
                        FURI_LOG_I(TAG, "Chip Erase timeout error");
                    }
                    return result;
                }
                FURI_LOG_I(TAG, "Can't start Chip Erase");
            } else {
                FURI_LOG_I(TAG, "Can't change Write Enable");
            }
        } else {
            FURI_LOG_I(TAG, "Chip status: BUSY");
        }
        set_write_enabled(false);
    } else {
        FURI_LOG_I(TAG, "Chip info not valid");
    }
    return true;
}

bool SpiToolkit::sector_erase(uint16_t n_sector) {
    SpiLock lock;

    FURI_LOG_I(TAG, "Erasing sector %d", n_sector);
#ifdef FLASHMGR_MOCK
    osDelay(1000);
    // TODO: sector_erase
    return true;
#endif // FLASHMGR_MOCK

    return false;
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
    return true;
#endif // FLASHMGR_MOCK

    bool result = false;

    if(last_info.valid) {
        if((offset + data_len) <= last_info.size) {
            SpiLock lock;
            if(last_info.write_mode & CHIP_WM_PAGE_256B) {
                result =
                    page256_or_1_byte_write(&last_info, offset, data_len, 256, (uint8_t*)p_data);
            } else if(last_info.write_mode & CHIP_WM_AAI) {
                result = aai_write(&last_info, offset, data_len, (uint8_t*)p_data);
            } else if(last_info.write_mode & CHIP_WM_DUAL_BUFFER) {
                FURI_LOG_I(TAG, "Dual-buffer mode not implemented");
            }
        } else {
            FURI_LOG_I(TAG, "Flash address is out of bound.");
        }
    } else {
        FURI_LOG_I(TAG, "Chip info not valid");
    }
    return result;
}

bool SpiToolkit::read_block(const size_t offset, uint8_t* const p_data, const size_t data_len) {
    furi_assert(p_data);
    furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

#ifdef FLASHMGR_MOCK
    osDelay(10);
    FURI_LOG_I(TAG, "memset @ %x len %x", p_data, data_len);
    memset(p_data, 0xCD, data_len);
    return true;
#endif // FLASHMGR_MOCK

    if(last_info.valid) {
        if((offset + data_len) <= last_info.size) {
            SpiLock lock;
            if(release_deep_sleep() && wait_busy(10)) {
                FURI_LOG_I(TAG, "Reading %d bytes @ %x", data_len, offset);
                uint8_t cmd[4];
                if(spi_wrapper_write_read(
                       SpiChipCommand_READ_DATA,
                       cmd,
                       make_adress_byte_array(&last_info, offset, cmd),
                       p_data,
                       data_len))
                    return true;
                FURI_LOG_I(TAG, "Read data failed");
            } else {
                FURI_LOG_I(TAG, "Chip status: BUSY");
            }
        } else {
            FURI_LOG_I(TAG, "Flash address is out of bound.");
        }
    } else {
        FURI_LOG_I(TAG, "Chip info not valid");
    }
    return false;
}

SpiFlashInfo_t const* SpiToolkit::get_info() const {
    if(last_info.valid) {
        return &last_info;
    }
    return nullptr;
}