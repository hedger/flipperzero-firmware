#pragma once

#include <stdint.h>

void sector_cache_flush();
uint8_t* sector_cache_get(uint32_t n_sector);
void sector_cache_put(uint32_t n_sector, uint8_t* data);
void sector_cache_invalidate_range(uint32_t start_sector, uint32_t end_sector);
