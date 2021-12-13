#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sym_cb)(const char* symname, uint32_t address, uint8_t type);

bool process_elf(const char* fwname, sym_cb callback);


#ifdef __cplusplus
}
#endif
