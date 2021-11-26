#include "spi_api.h"

#include <furi.h>
#include <furi-hal.h>

#define TAG "SPIAPI"

SpiToolkit::SpiToolkit() {

}

bool SpiToolkit::detect_flash() {
  // TODO: implement
  osDelay(1000);

  last_info.valid = true;
  last_info.vendor_id = 0x13371337;
  last_info.size = 0x100000;
  return true;
}


bool SpiToolkit::chip_erase() {
  FURI_LOG_I(TAG, "Erasing chip");

  osDelay(4000);
  // TODO: implement

  return true;
}


bool SpiToolkit::sector_erase(uint16_t n_sector) {
  FURI_LOG_I(TAG, "Erasing sector %d", n_sector);

  osDelay(1000);
  // TODO: implement
  return true;
}


bool SpiToolkit::write_block(const size_t offset, const uint8_t* const p_data, const size_t data_len) {
  furi_assert(p_data);
  furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

  FURI_LOG_I(TAG, "Writing %d bytes @ %x", data_len, offset);

  osDelay(100);
  // TODO: implement
  return true;
}


bool SpiToolkit::read_block(const size_t offset, uint8_t* const p_data, const size_t data_len)
{
  furi_assert(p_data);
  furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));
  
  FURI_LOG_I(TAG, "Reading %d bytes @ %x", data_len, offset);

  osDelay(10);
  // TODO: implement
  memset(p_data, 0xCD, data_len);

  return true;
}

SpiFlashInfo_t const* SpiToolkit::get_info() const {
  if (last_info.valid) {
    return &last_info;
  }

  return nullptr;
}