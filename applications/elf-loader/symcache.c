#include "symcache.h"

#include <file_helper.h>
#include <storage/storage.h>
#include <string.h>
#include <flipper_file/flipper_file.h>

#include "tkvdb/tkvdb.h"

#define TAG "ElfSym"

#define READ_BUF_LEN 4096
#define SYM_PATH "/ext/elf/fwdef.cfg"
#define TKV_SYM_PATH "/ext/elf/fwdef.tkv"

static Storage* storage = NULL;
static File* sym_cache = NULL;
static tkvdb* db;
static tkvdb_tr* transaction;

bool fw_sym_cache_init() {
    if(fw_sym_cache_ready()) {
        return true;
    };

    storage = furi_record_open("storage");
    db = tkvdb_open(storage, TKV_SYM_PATH, NULL);
    if(!db) {
        fw_sym_cache_free();
        return false;
    }
    transaction = tkvdb_tr_create(db, NULL);

    return true;
}

void fw_sym_cache_free() {
    if(!fw_sym_cache_ready()) {
        return;
    }

    if(db) {
        transaction->free(transaction);
        tkvdb_close(db);
    }

    if(sym_cache) {
        storage_file_close(sym_cache);
        storage_file_free(sym_cache);
        sym_cache = NULL;
    }

    furi_record_close("storage");
    storage = NULL;
}

bool fw_sym_cache_ready() {
    return (storage && (sym_cache || db));
}

uint32_t fw_sym_cache_resolve(const char* symname) {
    //if(!fw_sym_cache_ready()) {
    //    return NULL;
    //}

    uint32_t ret_address = 0;

    tkvdb_datum key, ovalue;

    key.data = symname;
    key.size = strlen(key.data);

    transaction->begin(transaction); /* start new transaction */
    TKVDB_RES opres = transaction->get(transaction, &key, &ovalue); /* get key-value pair */
    FURI_LOG_I(TAG, "query res: %d", opres);

    if(opres != 0) {
        FURI_LOG_W(TAG, "query FAILED!");
        ret_address = 0xFFFFFFFF;
    } else {
        furi_assert(ovalue.size == sizeof(uint32_t));
        ret_address = *(uint32_t*)(ovalue.data);
    }

    transaction->rollback(transaction); /* dismiss */

    FURI_LOG_I(
        TAG,
        "done: ovalue.size=%d, ret_address = %x\n",
        ovalue.size,
        ret_address);

    return ret_address;
}