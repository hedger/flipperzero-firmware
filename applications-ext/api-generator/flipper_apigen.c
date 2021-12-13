/* calculate words frequency */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
//#include <unistd.h>
#include <string.h>
#include <inttypes.h>

#include <tkvdb.h>
#include "elfio_loadsym.h"

#define FLIPPER_FW_SYM_DEFINITION_FILE "fwdef.tkv"

static tkvdb_tr *output_transaction;
static tkvdb_datum output_key, output_value;

void process_sym(const char* symname, uint32_t addr, uint8_t type) {
	printf("sym cb: '%s' (%d) @ %x...", symname, type, addr);
	if ((type != 1 /*STB_GLOBAL*/) && (type != 2 /*STB_WEAK*/)) {
		printf(" skipped\n");
		return;
	}

	output_key.data = (char*)symname;
	output_key.size = strlen(symname);

	uint32_t local_addr = addr;
	output_value.data = &local_addr;
	output_value.size = sizeof(addr);

	output_transaction->put(output_transaction, &output_key, &output_value);
	printf(" saved\n");
};

int main() {
	tkvdb_datum key, value, ovalue;
	TKVDB_RES opres;

	remove(FLIPPER_FW_SYM_DEFINITION_FILE);

	tkvdb    *db = tkvdb_open(FLIPPER_FW_SYM_DEFINITION_FILE, NULL);           /* optional, only if you need to keep data on disk */
	printf("db: %p\n", db);

	tkvdb_tr *transaction = tkvdb_tr_create(db, NULL);     /* pass NULL instead of db for RAM-only db */
	output_transaction = transaction;


	transaction->begin(transaction);             /* start */
	process_elf("flipper-z-f7-firmware-local.elf", process_sym);
	transaction->commit(transaction);            /* commit */


	/* READ TEST */
	transaction->begin(transaction);             /* start new transaction */

	key.data = "acquire_mutex";
	key.size = strlen(key.data);
	opres = transaction->get(transaction, &key, &ovalue);   /* get key-value pair */
	printf("query res: %d\n", opres);
	printf("done: ovalue.size=%x, ovalue.data=%p, val=%x\n", ovalue.size, ovalue.data, *(int32_t*)ovalue.data);

	key.data = "_ZN3cbc7Details17FuncMemberWrapperILj0E14ViewControllerI9LfRfidAppJ9SubmenuVM7PopupVM10DialogExVM11TextInputVM11ByteInputVM11ContainerVMEEmJPvEE8MetaCallESB_";
	key.size = strlen(key.data);
	opres = transaction->get(transaction, &key, &ovalue);   /* get key-value pair */
	printf("query res: %d\n", opres);
	printf("done: ovalue.size=%x, ovalue.data=%p, val=%x\n", ovalue.size, ovalue.data, *(int32_t*)ovalue.data);

	transaction->rollback(transaction);          /* dismiss */


	transaction->free(transaction);
	tkvdb_close(db);                             /* close on-disk database */

	return EXIT_SUCCESS;
}
