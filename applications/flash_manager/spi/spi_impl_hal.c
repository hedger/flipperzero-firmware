#include "spi_impl.h"

#ifndef FLASMMGR_SPI_BITBANG

#include <furi.h>
#include <furi-hal.h>

#include <furi-hal-gpio.h>

bool spi_wrapper_init() {
    return false;
}

void spi_wrapper_deinit() {
}

bool write_read(
    uint8_t opCode,
    uint8_t* write_data,
    int write_len,
    uint8_t* read_data,
    int read_len) {

    return false;
}

#endif // FLASMMGR_SPI_BITBANG