#include "symcache.h"
#include <toolbox/version.h>
#include <storage/storage.h>

#define TAG "ElfSym"

#define CACHE_ID_STRING "os_FLIPPER_BUILD"

#define TKV_SYM_PATH "/ext/elf/fwdef.tkv"

#define SYM_NOT_FOUND_VA 0xFFFFFFFF
#define CACHE_FILE_MAGIC 3250633246

#define PAGE_SIZE 512
#define ENTRIES_PER_PAGE PAGE_SIZE / sizeof(sym_entry_t)

typedef struct sym_entry {
    uint32_t address;
    uint32_t hash;
} sym_entry_t;

extern sym_entry_t __sym_data_start__;
static sym_entry_t* sym_cache = NULL;
static uint16_t n_cache_entries = 0;

bool fw_sym_cache_init() {
    sym_cache = &__sym_data_start__;
    do {
        if(sym_cache->address != CACHE_FILE_MAGIC) {
            FURI_LOG_W(TAG, "Sym cache file magic mismatch");
            break;
        }

        n_cache_entries = sym_cache->hash;
        FURI_LOG_I(TAG, "Init cache with %d entries.", n_cache_entries);

        uint32_t version_id_from_os = strtoul(version_get_githash(NULL), NULL, 16);
        uint32_t version_id_from_cache = fw_sym_cache_resolve(CACHE_ID_STRING);

        if(version_id_from_cache != version_id_from_os) {
            FURI_LOG_W(
                TAG,
                "Sym cache file FW mismatch: running %x, cache for %x",
                version_id_from_cache,
                version_id_from_os);
            break;
        }

        return true;
    } while(0);

    fw_sym_cache_free();
    return false;
}

void fw_sym_cache_free() {
    if(!fw_sym_cache_ready()) {
        return;
    }

    sym_cache = NULL;
    n_cache_entries = 0;
}

bool fw_sym_cache_ready() {
    return sym_cache != NULL;
}

static uint32_t elf_gnu_hash(const unsigned char* s) {
    uint32_t h = 0x1505;
    for(unsigned char c = *s; c != '\0'; c = *++s) h = (h << 5) + h + c;
    return h;
}

static int sym_compare(const void* target_hash, const void* entry) {
    return *(uint32_t*)target_hash - ((sym_entry_t*)entry)->hash;
}

uint32_t fw_sym_cache_resolve(char* symname) {
    if(!fw_sym_cache_ready()) {
        FURI_LOG_W(TAG, "Cache is not open!");
        return SYM_NOT_FOUND_VA;
    }

    uint32_t gnu_sym_hash = elf_gnu_hash((const unsigned char*)symname);

    sym_entry_t* sym_data = bsearch(
        &gnu_sym_hash, sym_cache + 1, n_cache_entries, sizeof(sym_entry_t), &sym_compare);
    if (!sym_data) {
        return SYM_NOT_FOUND_VA;
    }

    return sym_data->address;
}
