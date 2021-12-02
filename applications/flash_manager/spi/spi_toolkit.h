#pragma once

#include <stddef.h>
#include <stdint.h>

/* maximum number of erase type support on JESD216 (V1.0) */
#define SFDP_ERASE_TYPE_MAX_NUM 4

struct SpiFlashInfo_t {
    bool valid;
    size_t size;
    uint8_t vendor_id;
    uint8_t type_id;
    uint8_t capacity_id;
    const char* name;
    uint16_t write_mode;
    uint32_t erase_gran;
    uint8_t erase_gran_cmd;
    uint8_t minor_rev;
    uint8_t major_rev;
    uint8_t erase_4k_cmd;
    uint16_t write_gran;
    uint8_t vola_sr_we_cmd;
    bool erase_4k;
    bool sr_is_non_vola;
    bool addr_3_byte;
    bool addr_4_byte;
    struct {
        uint32_t size; /**< erase sector size (bytes). 0x00: not available */
        uint8_t cmd; /**< erase command */
    } eraser[SFDP_ERASE_TYPE_MAX_NUM]; /**< supported eraser types table */
};

class SpiToolkit {
public:
    static const size_t SPI_MAX_BLOCK_SIZE = 256;

    SpiToolkit();
    ~SpiToolkit();

    // Disconnect pins (set to Analog state)
    void disconnect();

    // on success, updates last_info for get_info()
    bool detect_flash();

    bool chip_erase();
    bool sector_erase(uint16_t n_sector);

    bool write_block(const size_t offset, const uint8_t* const p_data, const size_t data_len);
    bool read_block(const size_t offset, uint8_t* const p_data, const size_t data_len);

    SpiFlashInfo_t const* get_info() const;

private:
    SpiFlashInfo_t last_info;
};