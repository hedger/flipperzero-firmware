#pragma  once

#include <stddef.h>
#include <stdint.h>

struct SpiFlashInfo_t {
    bool valid;
    size_t size;
    int32_t vendor_id;
};

class SpiToolkit {
public:
    SpiToolkit();

    // on success, updates last_info for get_info()
    bool detect_flash();

    bool chip_erase();
    bool sector_erase(uint16_t n_sector);

    bool write_block(size_t offset, void* p_data, size_t data_len);
    bool read_block(size_t offset, void* p_data, size_t data_len);

    const SpiFlashInfo_t* get_info() const;

private:
    SpiFlashInfo_t last_info;
};
