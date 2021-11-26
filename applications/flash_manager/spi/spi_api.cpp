#include "spi_api.h"

#include <furi.h>
#include <furi-hal.h>

#define TAG "SPIAPI"

SpiToolkit::SpiToolkit() {

}

bool SpiToolkit::detect_flash() {
  // TODO: implement
  last_info.vendor_id = 0x13371337;
  last_info.size = 0x100000;
  return true;
}


bool SpiToolkit::chip_erase() {
  FURI_LOG_I(TAG, "Erasing chip");

  // TODO: implement
  return true;
}


bool SpiToolkit::sector_erase(uint16_t n_sector) {
  FURI_LOG_I(TAG, "Erasing sector %d", n_sector);

  // TODO: implement
  return true;
}


bool SpiToolkit::write_block(size_t offset, void* p_data, size_t data_len) {
  furi_assert(p_data);
  furi_assert(data_len);

  FURI_LOG_I(TAG, "Writing %d bytes @ %x", data_len, offset);

  // TODO: implement
  return true;
}


bool SpiToolkit::read_block(size_t offset, void* p_data, size_t data_len)
{
  furi_assert(p_data);
  furi_assert(data_len);
  
  FURI_LOG_I(TAG, "Reading %d bytes @ %x", data_len, offset);

  memset(p_data, 0xCD, data_len);
  // TODO: implement
  return true;
}

const SpiFlashInfo_t* SpiToolkit::get_info() const {
  if (last_info.valid) {
    return &last_info;
  }

  return nullptr;
}