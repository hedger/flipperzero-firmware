#pragma once
#include <storage/storage.h>
#include <tkvdb.h>

template <typename T> struct TkvObject {
    tkvdb_datum datum;

    TkvObject() {
        datum.size = sizeof(T);
    }

    inline void set(T& _val) {
        datum.size = sizeof(T);
        datum.data = &_val;
    }
};

template <> struct TkvObject<char*> {
    tkvdb_datum datum;

    TkvObject() {
    }

    inline void set(char* _val) {
        //char yoba[] = "alloyoba";
        //datum.data = &yoba[0]; //_val;
        datum.data = _val;
        datum.size = strlen(_val);
    }
};

template <typename tK, typename tV> class TkvDatabase {
private:
    const char* db_filename;
    tkvdb* db;
    tkvdb_tr* transaction;
    const uint16_t max_queries_in_transaction;
    uint16_t transaction_query_count;
    TkvObject<tK> key;
    TkvObject<tV> value;
    bool readonly, dirty;

    void check_transaction_count() {
        if(transaction_query_count++ > max_queries_in_transaction) {
            if (dirty) {
                FURI_LOG_I("cpptkv", "autocommitting on count check");
                commit();
            } else {
                transaction->rollback(transaction);
            }
            transaction->begin(transaction);
            transaction_query_count = 0;
        }
    }

public:
    TkvDatabase(const char* filename, bool _readonly)
        : db_filename(filename)
        , db(nullptr)
        , transaction(nullptr)
        , max_queries_in_transaction(13) // magic!
        , transaction_query_count(0)
        , readonly(_readonly)
        , dirty(false) {
    }

    ~TkvDatabase() {
        close();
    }

    bool open() {
        tkvdb_params* open_params = nullptr;
        if(!readonly) {
            open_params = tkvdb_params_create();
            tkvdb_param_set(open_params, TKVDB_PARAM_DBFILE_OPEN_FLAGS, FSAM_READ | FSAM_WRITE);
            tkvdb_param_set(open_params, TKVDB_PARAM_DBFILE_OPEN_MODE, FSOM_OPEN_ALWAYS);
        }
        db = tkvdb_open(db_filename, open_params);
        if(!readonly) {
            tkvdb_params_free(open_params);
        }

        if(!db) {
            return false;
        }

        transaction = tkvdb_tr_create(db, nullptr);
        transaction->begin(transaction);
        return true;
    }

    void close() {
        if(transaction) {
            if(dirty) {
                FURI_LOG_I("cpptkv", "autocommitting on close");
                commit();
            }
            transaction->rollback(transaction);
            transaction->free(transaction);
            transaction = nullptr;
        }

        if(db) {
            tkvdb_close(db);
            db = nullptr;
        }
    }

    bool commit() {
        FURI_LOG_I("cpptkv", "COMMIT()");
        dirty = false;
        return (transaction->commit(transaction) == TKVDB_OK);
    }

    bool drop_contents() {
        close();
        storage_common_remove(furi_record_open("storage"), db_filename);
        furi_record_close("storage");
        open();
        return true; // FIXME
    }

    bool get(const tK& _key, tV* _value) {
        check_transaction_count();
        key.set(_key);
        if(transaction->get(transaction, &key.datum, &value.datum) != TKVDB_OK) {
            return false;
        }
        furi_assert(value.datum.size == sizeof(tV));
        *_value = *reinterpret_cast<tV*>(value.datum.data);
        return true;
    }

    bool put(const tK& _key, tV& _value) {
        FURI_LOG_I("cpptkv", "put() %d", value);
        check_transaction_count();
        key.set(_key);
        value.set(_value);
        dirty = true;
        return (transaction->put(transaction, &key.datum, &value.datum) == TKVDB_OK);
    }
};