/* calculate words frequency */
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
//#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <tkvdb.h>

#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "elfio_loadsym.h"

#define FLIPPER_FW_SYM_DEFINITION_FILE "fwdef.tkv"

uint32_t elf_gnu_hash(const unsigned char* s) {
    uint32_t h = 0x1505;
    for(unsigned char c = *s; c != '\0'; c = *++s) h = (h << 5) + h + c;
    return h;
}

class TkvDbProcessor : public SymbolProcessor {
protected:
    tkvdb* db;
    tkvdb_tr* transaction;
    tkvdb_datum output_key, output_value;

public:
    virtual bool init_database(const char* fname) override {
        remove(FLIPPER_FW_SYM_DEFINITION_FILE);
        db = tkvdb_open(
            FLIPPER_FW_SYM_DEFINITION_FILE,
            NULL); /* optional, only if you need to keep data on disk */
        transaction = tkvdb_tr_create(db, NULL); /* pass NULL instead of db for RAM-only db */
        transaction->begin(transaction); /* start */
    }

    virtual ~TkvDbProcessor() override {
        transaction->commit(transaction); /* commit */
        transaction->free(transaction);
        tkvdb_close(db); /* close on-disk database */
    }
};

class TkvDbStringKeyProcessor : public TkvDbProcessor {
    virtual bool process_symbol(
        const char* symname,
        uint32_t address,
        uint8_t bind,
        uint8_t type,
        uint8_t other) override {
        output_key.data = (char*)symname;
        output_key.size = strlen(symname);

        uint32_t local_addr = address;
        output_value.data = &local_addr;
        output_value.size = sizeof(uint32_t);

        return (transaction->put(transaction, &output_key, &output_value) == TKVDB_OK);
    }
};

class TkvDbHashKeyProcessor : public TkvDbProcessor {
    virtual bool process_symbol(
        const char* symname,
        uint32_t address,
        uint8_t bind,
        uint8_t type,
        uint8_t other) override {
        uint32_t sym_hash = elf_gnu_hash((const unsigned char*)symname);
        output_key.data = &sym_hash;
        output_key.size = sizeof(uint32_t);

        uint32_t local_addr = address;
        output_value.data = &local_addr;
        output_value.size = sizeof(uint32_t);

        printf("saving %x -> %x\n", sym_hash, local_addr);
        return (transaction->put(transaction, &output_key, &output_value) == TKVDB_OK);
    }
};

class MapSymbolProcessor : public SymbolProcessor {
protected:
    std::map<uint32_t, uint32_t> sym_map;
    std::ofstream ofs;

public:
    static constexpr uint32_t MAP_FILE_MAGIC = 3250633246; //515555521;

    virtual bool init_database(const char* fname) override {
        ofs.open(fname, std::ios::binary | std::ios::out);
        return ofs.is_open();
    }

    virtual bool process_symbol(
        const char* symname,
        uint32_t address,
        uint8_t bind,
        uint8_t type,
        uint8_t other) override {
        uint32_t sym_hash = elf_gnu_hash((const unsigned char*)symname);
        // std::cout << "putting " << std::hex << sym_hash << " -> " << address << std::endl;
        const auto [it_prev_entry, success] = sym_map.insert({sym_hash, address});
        return success;
    }

    virtual ~MapSymbolProcessor() override {
        uint32_t tmp_uint32 = MAP_FILE_MAGIC;
        ofs.write(reinterpret_cast<const char*>(&tmp_uint32), sizeof(uint32_t));
        tmp_uint32 = sym_map.size();
        ofs.write(reinterpret_cast<const char*>(&tmp_uint32), sizeof(uint32_t));

        uint32_t idx = 0;
        std::for_each(sym_map.cbegin(), sym_map.cend(), [&](auto& pair) {
            uint64_t tmp_uint64 = (static_cast<uint64_t>(pair.first) << 32) | pair.second;
            //std::cout << "putting " << std::hex << pair.first << " -> " << pair.second << " as " << tmp_uint64 << std::endl;
            //std::cout << std::dec << ++idx  << ": " << std::hex << pair.first << " -> " << pair.second << std::endl;
            ofs.write(reinterpret_cast<const char*>(&tmp_uint64), sizeof(uint64_t));
        });
        ofs.close();
    }
};

int main() {
    // printf("key= %x", elf_gnu_hash("os_FLIPPER_BUILD"));
    // return 0;
    // process_elf("flipper-z-f7-firmware-local.elf", dump_sym);
    // return EXIT_SUCCESS;

    tkvdb_datum key, value, ovalue;
    TKVDB_RES opres;

    {
        MapSymbolProcessor processor;
        processor.init_database(FLIPPER_FW_SYM_DEFINITION_FILE);
        if(!process_elf("flipper-z-f7-firmware-local.elf", &processor)) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
    /* READ TEST */
    tkvdb* db = tkvdb_open(
        FLIPPER_FW_SYM_DEFINITION_FILE,
        NULL); /* optional, only if you need to keep data on disk */
    printf("db: %p\n", db);

    tkvdb_tr* transaction =
        tkvdb_tr_create(db, NULL); /* pass NULL instead of db for RAM-only db */
    transaction->begin(transaction); /* start new transaction */

    uint32_t tag_val = 0x84f96087;

    key.data = &tag_val;
    key.size = sizeof(uint32_t);
    opres = transaction->get(transaction, &key, &ovalue); /* get key-value pair */
    printf("query res: %d\n", opres);
    printf(
        "done: ovalue.size=%x, ovalue.data=%p, val=%x\n",
        ovalue.size,
        ovalue.data,
        *(int32_t*)ovalue.data);

    // key.data = "osKernelGetTickFreq";
    // key.size = strlen(key.data);
    // opres = transaction->get(transaction, &key, &ovalue); /* get key-value pair */
    // printf("query res: %d\n", opres);
    // printf("done: ovalue.size=%x, ovalue.data=%p, val=%x\n", ovalue.size,
    //        ovalue.data, *(int32_t *)ovalue.data);

    // key.data = "_ZN3cbc7Details17FuncMemberWrapperILj0E14ViewControllerI9LfRfidAppJ9Subm"
    //            "enuVM7PopupVM10DialogExVM11TextInputVM11ByteInputVM11ContainerVMEEmJPvEE"
    //            "8MetaCallESB_";
    // key.size = strlen(key.data);
    // opres = transaction->get(transaction, &key, &ovalue); /* get key-value pair */
    // printf("query res: %d\n", opres);
    // printf("done: ovalue.size=%x, ovalue.data=%p, val=%x\n", ovalue.size,
    //        ovalue.data, *(int32_t *)ovalue.data);

    transaction->rollback(transaction); /* dismiss */

    transaction->free(transaction);
    tkvdb_close(db); /* close on-disk database */

    return EXIT_SUCCESS;
}
