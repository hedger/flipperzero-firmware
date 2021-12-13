#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


bool fw_sym_cache_init();
void fw_sym_cache_free();
bool fw_sym_cache_ready();
uint32_t fw_sym_cache_resolve(const char* symname);


#ifdef __cplusplus
}
#endif
