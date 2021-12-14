#include "symcache.h"

#include <file_helper.h>
#include <storage/storage.h>
#include <string.h>
#include <flipper_file/flipper_file.h>

#include <tkvdb.h>

#define TAG "ElfSym"

#define READ_BUF_LEN 4096
#define SYM_PATH "/ext/elf/fwdef.cfg"
#define TKV_SYM_PATH "/ext/elf/fwdef.tkv"

// max symbols per query till it needs to be discarded b/c consuming memory
#define TKV_TRANSACTION_QUERY_LIMIT 13 

static tkvdb* db;
static tkvdb_tr* transaction;
static int transaction_queries = 0;

bool fw_sym_cache_init() {
    if(fw_sym_cache_ready()) {
        return true;
    };

    db = tkvdb_open(TKV_SYM_PATH, NULL);
    if(!db) {
        fw_sym_cache_free();
        return false;
    }
    transaction = tkvdb_tr_create(db, NULL);
    transaction->begin(transaction); /* start new transaction */
    transaction_queries = 0;

    return true;
}

void fw_sym_cache_free() {
    if(!fw_sym_cache_ready()) {
        return;
    }

    if(db) {
        transaction->rollback(transaction); /* dismiss */
        transaction->free(transaction);
        tkvdb_close(db);
        db = NULL;
        transaction = NULL;
    }

    furi_record_close("storage");
}

bool fw_sym_cache_ready() {
    return (db && transaction);
}

static inline void fw_sym_cache_renew_transaction() {
    if (transaction_queries++ > TKV_TRANSACTION_QUERY_LIMIT) {
        FURI_LOG_W(TAG, "Renewing transaction");
        transaction->rollback(transaction); /* dismiss */
        //transaction->free(transaction);
        
        //transaction = tkvdb_tr_create(db, NULL);
        transaction->begin(transaction); /* start new transaction */
        transaction_queries = 0;
    }
}

uint32_t fw_sym_cache_resolve(const char* symname) {
    if(!fw_sym_cache_ready()) {
        return NULL;
    }

    fw_sym_cache_renew_transaction();

    uint32_t ret_address = 0;

    tkvdb_datum key, ovalue;

    key.data = symname;
    key.size = strlen(key.data);

    TKVDB_RES opres = transaction->get(transaction, &key, &ovalue); /* get key-value pair */
    if(opres != 0) {
        FURI_LOG_W(TAG, "query FAILED! res = %d", opres);
        ret_address = 0xFFFFFFFF;
    } else {
        furi_assert(ovalue.size == sizeof(uint32_t));
        ret_address = *(uint32_t*)(ovalue.data);
    }

    FURI_LOG_I(TAG, "done: ovalue.size=%d, ret_address = %x", ovalue.size, ret_address);

    return ret_address;
}
