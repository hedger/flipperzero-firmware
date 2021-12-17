#include "sectorcache.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SECTOR_SIZE 512
#define N_SECTORS 8

static uint32_t cache_itr = 0;
static uint32_t cached_sectors[N_SECTORS] = {0};
static uint8_t cached_sector_data[N_SECTORS][SECTOR_SIZE];

void sector_cache_flush() {
    memset(&cached_sectors, 0, sizeof(uint32_t) * N_SECTORS);
}

uint8_t* sector_cache_get(uint32_t n_sector) {
    if(n_sector == 0) {
        return NULL;
    }
    for(int sector_i = 0; sector_i < N_SECTORS; ++sector_i) {
        if(cached_sectors[sector_i] == n_sector) {
            return cached_sector_data[sector_i];
        }
    }
    return NULL;
}

void sector_cache_put(uint32_t n_sector, uint8_t* data) {
    cached_sectors[cache_itr % N_SECTORS] = n_sector;
    memcpy(cached_sector_data[cache_itr % N_SECTORS], data, SECTOR_SIZE);
    cache_itr++;
}

void sector_cache_invalidate_range(uint32_t start_sector, uint32_t end_sector) {
    for(int sector_i = 0; sector_i < N_SECTORS; ++sector_i) {
        if((cached_sectors[sector_i] >= start_sector) &&
           (cached_sectors[sector_i] <= end_sector)) {
            cached_sectors[sector_i] = 0;
        }
    }
}
