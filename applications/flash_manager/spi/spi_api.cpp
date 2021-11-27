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


#define SPI_PIN_MOSI    0x01
#define SPI_PIN_MISO    0x02
#define SPI_PIN_SCK     0x04
#define SPI_PIN_CS      0x08

GpioPin SPI_MOSI = { GPIOA, GPIO_PIN_7 };
GpioPin SPI_MISO = { GPIOA, GPIO_PIN_6 };
GpioPin SPI_SCK  = { GPIOB, GPIO_PIN_3 };
GpioPin SPI_CS   = { GPIOA, GPIO_PIN_4 };

static uint32_t read_pins( void)
{
    uint32_t pins = 0;
    if ( hal_gpio_read( &SPI_MOSI ) )
        pins |= SPI_PIN_MOSI;
    if ( hal_gpio_read( &SPI_MISO ) )
        pins |= SPI_PIN_MISO;
    if ( hal_gpio_read( &SPI_SCK ) )
        pins |= SPI_PIN_SCK;
    if ( hal_gpio_read( &SPI_CS ) )
        pins |= SPI_PIN_CS;
    return pins;
}

/**
  * @brief Disconnect pins (set to Analog state)
  */
void SpiToolkit::disconnect( ) {
    hal_gpio_init( &SPI_CS, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
    hal_gpio_init( &SPI_SCK, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
    hal_gpio_init( &SPI_MOSI, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
    hal_gpio_init( &SPI_MISO, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
}

bool SpiToolkit::detect_flash() {

    last_info.valid = false;

    for(;;)
    {
        // Check pins
        hal_gpio_init( &SPI_CS, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_SCK, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_MOSI, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );
        if ( read_pins( ) != 0 ) break;

        hal_gpio_init( &SPI_CS, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh );
        if ( read_pins( ) != SPI_PIN_CS ) break;

        hal_gpio_init( &SPI_SCK, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh );
        if ( read_pins( ) != ( SPI_PIN_CS | SPI_PIN_SCK ) ) break;
        hal_gpio_init( &SPI_SCK, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );

        hal_gpio_init( &SPI_MOSI, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh );
        if ( read_pins( ) != ( SPI_PIN_CS | SPI_PIN_MOSI ) ) break;
        hal_gpio_init( &SPI_MOSI, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );

        hal_gpio_init( &SPI_MISO, GpioModeInput, GpioPullUp, GpioSpeedVeryHigh );
        if ( read_pins( ) != ( SPI_PIN_CS | SPI_PIN_MISO ) ) break;
        hal_gpio_init( &SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );

        // Prepare pins state
        hal_gpio_write( &SPI_CS, true );
        hal_gpio_write( &SPI_SCK, false );
        hal_gpio_write( &SPI_MOSI, false );
        hal_gpio_write( &SPI_MISO, false );
        // Initialize pins
        hal_gpio_init( &SPI_CS, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_SCK, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_MOSI, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_MISO, GpioModeInput, GpioPullDown, GpioSpeedVeryHigh );

        // TODO: Read Identification info

        last_info.valid = true;
        break;
    }

    if ( last_info.valid )
    {
        last_info.vendor_id = 0x13371337;
        last_info.size = 0x10000;
    }
    else
    {
        hal_gpio_init( &SPI_CS, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_SCK, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_MOSI, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
        hal_gpio_init( &SPI_MISO, GpioModeAnalog, GpioPullNo, GpioSpeedVeryHigh );
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


bool SpiToolkit::write_block(const size_t offset, const uint8_t* const p_data, const size_t data_len) {
  furi_assert(p_data);
  furi_assert(data_len && (data_len <= SPI_MAX_BLOCK_SIZE));

  FURI_LOG_I(TAG, "Writing %d bytes @ %x", data_len, offset);

  osDelay(100);
  // TODO: write_block
  return true;
}


bool SpiToolkit::read_block(const size_t offset, uint8_t* const p_data, const size_t data_len)
{
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
  if (last_info.valid) {
    return &last_info;
  }

  return nullptr;
}