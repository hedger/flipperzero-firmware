#include "symcache.h"
#include "toolbox/version.h"
#include "tkvdb-wrapper.hpp"

#define TAG "ElfSym"

#define CACHE_ID_STRING "os_FLIPPER_BUILD"

#define TKV_SYM_PATH "/ext/elf/fwdef.tkv"
#define TKV_HOT_SYM_PATH "/ext/elf/.hotsym.tkv"

#define SYM_NOT_FOUND_VA 0xFFFFFFFF

TkvDatabase<char*, uint32_t>*database, *hot_database;

bool fw_sym_cache_init() {
    if(database || hot_database) {
        fw_sym_cache_free();
    }

    do {
        database = new TkvDatabase<char*, uint32_t>(TKV_SYM_PATH, true);
        hot_database = new TkvDatabase<char*, uint32_t>(TKV_HOT_SYM_PATH, false);

        if(!database->open() || !hot_database->open()) {
            break;
        }

        uint32_t version_id_from_os = strtoul(version_get_githash(NULL), NULL, 16);
        uint32_t version_id_from_db;
        if(!database->get(CACHE_ID_STRING, &version_id_from_db)) {
            FURI_LOG_W(TAG, "Failed to get version from database!");
            break;
        }

        if(version_id_from_os != version_id_from_db) {
            FURI_LOG_W(
                TAG,
                "Symbol cache version mismatch (db: %x, os: %x)!",
                version_id_from_db,
                version_id_from_os);
            break;
        }

        bool reset_hot_db = false;
        if(!hot_database->get(CACHE_ID_STRING, &version_id_from_db)) {
            FURI_LOG_W(TAG, "Failed to get version from HOT database!");
            reset_hot_db = true;
        } else if(version_id_from_os != version_id_from_db) {
            FURI_LOG_W(
                TAG,
                "HOT Symbol cache version mismatch (db: %x, os: %x)",
                version_id_from_db,
                version_id_from_os);
            reset_hot_db = true;
        }

        if(reset_hot_db) {
            FURI_LOG_I(TAG, "Dropping hot db");
            if(!hot_database->drop_contents()) {
                FURI_LOG_I(TAG, "Failed to drop db!");
                break;
            }

            hot_database->put(CACHE_ID_STRING, version_id_from_os);
        }

        return true;
    } while(false);

    fw_sym_cache_free();
    return false;
}

void fw_sym_cache_free() {
    if(database) {
        database->close();
        delete database;
        database = nullptr;
    }

    if(hot_database) {
        hot_database->close();
        delete hot_database;
        hot_database = nullptr;
    }
}

bool fw_sym_cache_ready() {
    return (database != nullptr) && (hot_database != nullptr);
}

uint32_t fw_sym_cache_resolve(char* symname) {
    uint32_t ret;
    if(!hot_database->get(symname, &ret)) {
        FURI_LOG_I(TAG, "cache MISS");
        if(!database->get(symname, &ret)) {
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