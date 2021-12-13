#include "symcache.h"

#include <file_helper.h>
#include <storage/storage.h>
#include <string.h>
#include <flipper_file/flipper_file.h>

#define TAG "ElfSym"

#define READ_BUF_LEN 4096
#define SYM_PATH "/ext/elf/fwdef.cfg"

static Storage* storage = NULL;
static FlipperFile* flipper_file = NULL;

bool fw_sym_cache_init() {
    if(fw_sym_cache_ready()) {
        return true;
    };

    storage = furi_record_open("storage");
    flipper_file = flipper_file_alloc(storage);
    if(!flipper_file_open_existing(flipper_file, SYM_PATH)) {
        fw_sym_cache_free();
        return false;
    }
    // TODO: compare fw version with cache

    return true;
}

void fw_sym_cache_free() {
    if(!fw_sym_cache_ready()) {
        return;
    }
    flipper_file_close(flipper_file);
    flipper_file_free(flipper_file);
    flipper_file = NULL;
    furi_record_close("storage");
    storage = NULL;
}

bool fw_sym_cache_ready() {
    return (storage && flipper_file);
}

void* fw_sym_cache_resolve(const char* symname) {
    if(!fw_sym_cache_ready()) {
        return NULL;
    }

    uint32_t ret_address = 0;
    if(!flipper_file_read_uint32(flipper_file, symname, &ret_address, 1)) {
        FURI_LOG_E(TAG, "Missing Symbol %s", symname);
    }

    flipper_file_rewind(flipper_file);
    return (void*)ret_address;
}
