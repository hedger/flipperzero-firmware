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

typedef enum page_check_res {
    PAGE_CHECK_FOUND = 1 << 1,
    PAGE_CHECK_OVERSHOOT = 1 << 2,
    PAGE_CHECK_UNDERSHOOT = 1 << 3,
    PAGE_CHECK_NOT_FOUND = 1 << 4,
    PAGE_CHECK_ERROR = 1 << 5,
    PAGE_CHECK_FINAL_STATE = (PAGE_CHECK_FOUND | PAGE_CHECK_NOT_FOUND | PAGE_CHECK_ERROR)
} page_check_res;

static sym_entry_t* sym_page = NULL;
static File* cache_file = NULL;
static uint16_t n_cache_entries = 0;

static page_check_res
    check_sym_page_for_hash(uint32_t hash, uint16_t n_valid_entries, uint32_t* p_sym) {
    // check 1st and last
    if(sym_page[0].hash > hash) {
        // missed a page
        return PAGE_CHECK_OVERSHOOT;
    }

    if(sym_page[n_valid_entries - 1].hash < hash) {
        return PAGE_CHECK_UNDERSHOOT;
    }

    // TODO: binary search
    for(int sym_idx = 0; sym_idx < n_valid_entries; ++sym_idx) {
        if(sym_page[sym_idx].hash < hash) continue;

        if(sym_page[sym_idx].hash > hash) return PAGE_CHECK_NOT_FOUND;

        *p_sym = sym_page[sym_idx].address;
        return PAGE_CHECK_FOUND;
    }
    return PAGE_CHECK_ERROR;
}

bool fw_sym_cache_init() {
    cache_file = storage_file_alloc(furi_record_open("storage"));
    furi_record_close("storage");

    sym_entry_t header;

    do {
        if(!storage_file_open(cache_file, TKV_SYM_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
            FURI_LOG_W(TAG, "Failed to open sym cache file");
            break;
        }

        if(!storage_file_read(cache_file, &header, sizeof(header))) {
            break;
        }

        if(header.address != CACHE_FILE_MAGIC) {
            FURI_LOG_W(TAG, "Sym cache file magic mismatch");
            break;
        }

        n_cache_entries = header.hash;
        FURI_LOG_I(TAG, "Init cache with %d entries.", n_cache_entries);

        sym_page = furi_alloc(PAGE_SIZE);

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

    storage_file_free(cache_file);
    cache_file = NULL;
    n_cache_entries = 0;
    free(sym_page);
}

bool fw_sym_cache_ready() {
    return cache_file;
}

static uint32_t elf_gnu_hash(const unsigned char* s) {
    uint32_t h = 0x1505;
    for(unsigned char c = *s; c != '\0'; c = *++s) h = (h << 5) + h + c;
    return h;
}

uint16_t read_page(int32_t page_n) {
    storage_file_seek(cache_file, page_n * PAGE_SIZE, true);
    uint16_t bytes_read = storage_file_read(cache_file, sym_page, PAGE_SIZE);
    return bytes_read / sizeof(sym_entry_t);
}

uint32_t fw_sym_cache_resolve(char* symname) {
    if(!fw_sym_cache_ready()) {
        FURI_LOG_W(TAG, "Cache is not open!");
        return SYM_NOT_FOUND_VA;
    }
    uint32_t ret;
    uint32_t gnu_sym_hash = elf_gnu_hash((const unsigned char*)symname);

    uint32_t est_entry_n = (uint64_t)(gnu_sym_hash)*n_cache_entries / UINT_MAX;
    FURI_LOG_D(TAG, "est entry# for '%s' (%x) = %d", symname, gnu_sym_hash, est_entry_n);
    uint32_t page_offs = (est_entry_n * sizeof(uint64_t)) / PAGE_SIZE;
    FURI_LOG_D(TAG, "	est page  %d", page_offs);

    page_check_res page_check = PAGE_CHECK_ERROR;
    do {
        uint16_t entries = read_page(page_offs);
        page_check = check_sym_page_for_hash(gnu_sym_hash, entries, &ret);
        switch(page_check) {
        case PAGE_CHECK_FOUND:
            FURI_LOG_D(TAG, "	>page HIT for '%s'", symname);
            return ret;
        case PAGE_CHECK_OVERSHOOT:
            FURI_LOG_D(TAG, "	>page overshoot for '%s'", symname);
            page_offs--;
            break;
        case PAGE_CHECK_UNDERSHOOT:
            FURI_LOG_D(TAG, "	>page undershoot for '%s'", symname);
            page_offs++;
            break;
        case PAGE_CHECK_NOT_FOUND:
            FURI_LOG_W(TAG, "Symbol '%s' (%x) not found in cache!", symname, gnu_sym_hash);
            return SYM_NOT_FOUND_VA;
        default:
            break;
        }
    } while((page_check & PAGE_CHECK_FINAL_STATE) == 0);

    return SYM_NOT_FOUND_VA;
}