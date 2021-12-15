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

#include "elfio_loadsym.h"

#define FLIPPER_FW_SYM_DEFINITION_FILE "fwdef.tkv"

static tkvdb_tr *output_transaction;
static tkvdb_datum output_key, output_value;


static void dump_sym(const char *symname, uint32_t address, uint8_t bind,
                 uint8_t type, uint8_t other) {
  printf("(%d) @ %8x '%s'\n", type, address, symname);
}

uint32_t elf_gnu_hash( const unsigned char* s )
{
    uint32_t h = 0x1505;
    for ( unsigned char c = *s; c != '\0'; c = *++s )
        h = ( h << 5 ) + h + c;
    return h;
}

static void process_sym_hash(const char *symname, uint32_t address, uint8_t bind,
                 uint8_t type, uint8_t other) {
  //printf("(%d) @ %8x '%s'\n", type, address, symname);

  uint32_t sym_hash = elf_gnu_hash((const unsigned char*)symname);
  output_key.data = &sym_hash;
  output_key.size = sizeof(uint32_t);

  uint32_t local_addr = address;
  output_value.data = &local_addr;
  output_value.size = sizeof(uint32_t);

  printf("saving %x -> %x\n", sym_hash, local_addr);
  output_transaction->put(output_transaction, &output_key, &output_value);
}


static void process_sym(const char *symname, uint32_t address, uint8_t bind,
                 uint8_t type, uint8_t other) {
  //printf("(%d) @ %8x '%s'", type, address, symname);

  output_key.data = (char *)symname;
  output_key.size = strlen(symname);

  uint32_t local_addr = address;
  output_value.data = &local_addr;
  output_value.size = sizeof(uint32_t);

  output_transaction->put(output_transaction, &output_key, &output_value);
  //printf(" saved\n");
};

int main() {
	// printf("key= %x", elf_gnu_hash("os_FLIPPER_BUILD"));
	// return 0;
  // process_elf("flipper-z-f7-firmware-local.elf", dump_sym);
  // return EXIT_SUCCESS;

  tkvdb_datum key, value, ovalue;
  TKVDB_RES opres;

  remove(FLIPPER_FW_SYM_DEFINITION_FILE);

  tkvdb *db =
      tkvdb_open(FLIPPER_FW_SYM_DEFINITION_FILE,
                 NULL); /* optional, only if you need to keep data on disk */
  printf("db: %p\n", db);

  tkvdb_tr *transaction =
      tkvdb_tr_create(db, NULL); /* pass NULL instead of db for RAM-only db */
  output_transaction = transaction;

  transaction->begin(transaction); /* start */
  if (!process_elf("flipper-z-f7-firmware-local.elf", process_sym_hash)) {
    return EXIT_FAILURE;
    // disregard everything
  }
  transaction->commit(transaction); /* commit */
  // return EXIT_SUCCESS;

  /* READ TEST */
  transaction->begin(transaction); /* start new transaction */

  uint32_t tag_val = 0x84f96087;

  key.data = &tag_val;
  key.size = sizeof(uint32_t);
  opres = transaction->get(transaction, &key, &ovalue); /* get key-value pair */
  printf("query res: %d\n", opres);
  printf("done: ovalue.size=%x, ovalue.data=%p, val=%x\n", ovalue.size,
         ovalue.data, *(int32_t *)ovalue.data);

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
