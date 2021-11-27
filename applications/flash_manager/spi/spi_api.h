#pragma once

#include <stddef.h>
#include <stdint.h>

struct SpiFlashInfo_t {
    bool valid;
    size_t size;
    int32_t vendor_id;
    uint8_t type_id;
    uint8_t capacity_id;
    const char* name;
    uint16_t write_mode;
    uint32_t erase_gran;
    uint8_t erase_gran_cmd;
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