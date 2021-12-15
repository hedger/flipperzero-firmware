#include "symcache.h"
#include "toolbox/version.h"
#include "tkvdb-wrapper.hpp"

#define TAG "ElfSym"

#define CACHE_ID_STRING "os_FLIPPER_BUILD"

#define TKV_SYM_PATH "/ext/elf/fwdef.tkv"
#define TKV_HOT_SYM_PATH "/ext/elf/.hotsym.tkv"

#define SYM_NOT_FOUND_VA 0xFFFFFFFF

TkvDatabase<char*, uint32_t>* database, *hot_database;

bool fw_sym_cache_init() {
    if (database) {
        fw_sym_cache_free();
    }
    database = new TkvDatabase<char*, uint32_t>(TKV_SYM_PATH, true);
    hot_database = new TkvDatabase<char*, uint32_t>(TKV_HOT_SYM_PATH, false);

    if (database->open() && hot_database->open()) {
        return true;
    }

    fw_sym_cache_free();
    return false;
}

void fw_sym_cache_free() {
    if (database) {
        database->close();
        delete database;
        database = nullptr;
    }

    if (hot_database) {
        hot_database->close();
        delete hot_database;
        hot_database = nullptr;
    }
}

bool fw_sym_cache_ready() {
    return true;
}

uint32_t fw_sym_cache_resolve(char* symname) {
    uint32_t ret;
    if (!hot_database->get(symname, &ret)) {
        FURI_LOG_I(TAG, "cache MISS");
        if (!database->get(symname, &ret)) {
            FURI_LOG_I(TAG, "main db MISS");
            return SYM_NOT_FOUND_VA;
        } else {
            FURI_LOG_I(TAG, "putting '%s' in cache", symname);
            hot_database->put(symname, ret);
        }
    } else {
        FURI_LOG_I(TAG, "cache HIT!");
    }

    return ret;
}
