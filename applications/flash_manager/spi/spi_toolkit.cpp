#include "spi_toolkit.h"
#include "spi_chips.h"

#include "spi_impl.h"

#include <furi.h>


#define TAG "SPIAPI"

SpiToolkit::SpiToolkit() {
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

static bool read_sfdp() {
    return false;
}

bool SpiToolkit::detect_flash() {
#ifdef FLASHMGR_MOCK
        osDelay(2500);
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
        spi_wrapper_deinit();
    }
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