#include <furi.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include "elf-lib/loader.h"
#include "elf-loader-sys-api.h"
#include "elf-addr-resolver.h"

#define ELF_FOLDER "/ext/elf"
#define ELF_EXT ".elf"
#define TAG "elf_loader_app"

int32_t elf_loader_app(void* p) {
    ELFResolver resolver = elf_resolve_from_cache;
    if (!fw_sym_cache_init()) { // TODO: message box
        FURI_LOG_I(TAG, "Failed to init symbol cache");
        return -1;
    }

    //ELFResolver resolver = elf_resolve_from_table;
    Storage* storage = furi_record_open("storage");
    DialogsApp* dialogs = furi_record_open("dialogs");
    const uint8_t app_name_size = 128;
    char* app_name = furi_alloc(app_name_size + 1);
    // furi_log_set_level(5);
    string_t file_path;

    if(dialog_file_select_show(dialogs, ELF_FOLDER, ELF_EXT, app_name, app_name_size, NULL)) {
        string_init_printf(file_path, "%s/%s%s", ELF_FOLDER, app_name, ELF_EXT);
        FURI_LOG_I(TAG, "ELF Loader start for '%s'", string_get_cstr(file_path));
        int ret = loader_exec_elf(string_get_cstr(file_path), resolver, storage);
        FURI_LOG_I(TAG, "ELF Loader returned: %i", ret);
    }

    fw_sym_cache_free();
    string_clear(file_path);
    free(app_name);
    // furi_log_set_level(FURI_LOG_LEVEL);
    furi_record_close("dialogs");
    furi_record_close("storage");
    return 0;
}
