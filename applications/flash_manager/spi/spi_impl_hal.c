#include "spi_impl.h"

#ifndef FLASMMGR_SPI_BITBANG

#include <furi.h>
#include <furi-hal.h>

#include <furi-hal-spi.h>
#include <furi-hal-spi-config.h>

#define SPI_TIMEOUT 500

static FuriHalSpiBusHandle* const p_spi_bus = &furi_hal_spi_bus_handle_external;

bool spi_wrapper_init() {
    furi_hal_spi_bus_handle_init(&furi_hal_spi_bus_handle_external);
    return true;
}

void spi_wrapper_deinit() {
    furi_hal_spi_bus_handle_deinit(&furi_hal_spi_bus_handle_external);
}

void spi_wrapper_acquire_bus() {
    furi_hal_spi_acquire(&furi_hal_spi_bus_handle_external);
}

void spi_wrapper_release_bus() {
    furi_hal_spi_release(&furi_hal_spi_bus_handle_external);
}

bool spi_wrapper_write_read(
    uint8_t opCode,
    uint8_t* write_data,
    int write_len,
    uint8_t* read_data,
    int read_len) {
    uint8_t localOpCode[10] = {opCode};
    if(!furi_hal_spi_bus_tx(&furi_hal_spi_bus_handle_external, &localOpCode, 1, SPI_TIMEOUT)) {
        return false;
    }

    if(write_data && write_len &&
       !furi_hal_spi_bus_tx(
           &furi_hal_spi_bus_handle_external, write_data, write_len, SPI_TIMEOUT)) {
        return false;
    }
    return furi_hal_spi_bus_rx(
        &furi_hal_spi_bus_handle_external, read_data, read_len, SPI_TIMEOUT);
}

#endif // FLASMMGR_SPI_BITBANG