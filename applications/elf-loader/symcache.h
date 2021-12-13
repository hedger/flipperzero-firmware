#pragma once
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


bool fw_sym_cache_init();
void fw_sym_cache_free();
bool fw_sym_cache_ready();
void* fw_sym_cache_resolve(const char* symname);


#ifdef __cplusplus
}
#endif
