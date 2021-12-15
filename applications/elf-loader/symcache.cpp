#include "symcache.h"
#include "toolbox/version.h"
#include "tkvdb-wrapper.hpp"

#define TAG "ElfSym"

#define CACHE_ID_STRING "os_FLIPPER_BUILD"
//#define CACHE_ID_TAG 0x1EBAC0C1
static const uint32_t CACHE_ID_TAG = 0x84f96087;

#define TKV_SYM_PATH "/ext/elf/fwdef.tkv"
#define TKV_HOT_SYM_PATH "/ext/elf/.hotsym.tkv"

#define SYM_NOT_FOUND_VA 0xFFFFFFFF

TkvDatabase<uint32_t, uint32_t>*database, *hot_database;

bool fw_sym_cache_init() {
    uint32_t cache_tag = CACHE_ID_TAG;
    if(database || hot_database) {
        fw_sym_cache_free();
    }

    do {
        database = new TkvDatabase<uint32_t, uint32_t>(TKV_SYM_PATH, true);
        hot_database = new TkvDatabase<uint32_t, uint32_t>(TKV_HOT_SYM_PATH, false);

        if(!database->open() || !hot_database->open()) {
            break;
        }

        uint32_t version_id_from_os = strtoul(version_get_githash(NULL), NULL, 16);
        uint32_t version_id_from_db;
        if(!database->get(cache_tag, &version_id_from_db)) {
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
        if(!hot_database->get(cache_tag, &version_id_from_db)) {
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
            FURI_LOG_W(TAG, "Dropping hot db");
            if(!hot_database->drop_contents()) {
                FURI_LOG_W(TAG, "Failed to drop db!");
                break;
            }

            hot_database->put(cache_tag, version_id_from_os);
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

static uint32_t elf_gnu_hash( const unsigned char* s )
{
    uint32_t h = 0x1505;
    for ( unsigned char c = *s; c != '\0'; c = *++s )
        h = ( h << 5 ) + h + c;
    return h;
}

uint32_t fw_sym_cache_resolve(char* symname) {
    uint32_t ret;
    uint32_t sym_hash = elf_gnu_hash((const unsigned char*)symname);

    if(!hot_database->get(sym_hash, &ret)) {
        FURI_LOG_D(TAG, "cache MISS");
        if(!database->get(sym_hash, &ret)) {
            FURI_LOG_W(TAG, "main db MISS for '%s'", symname);
            return SYM_NOT_FOUND_VA;
        } else {
            FURI_LOG_I(TAG, "putting '%s' in cache", symname);
            hot_database->put(sym_hash, ret);
        }
    } else {
        FURI_LOG_D(TAG, "cache HIT!");
    }

    return ret;
}