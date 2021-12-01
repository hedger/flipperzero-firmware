#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool spi_wrapper_init();
void spi_wrapper_deinit();

bool write_read(
    uint8_t opCode,
    uint8_t* write_data,
    int write_len,
    uint8_t* read_data,
    int read_len);

#ifdef __cplusplus
}
#endif