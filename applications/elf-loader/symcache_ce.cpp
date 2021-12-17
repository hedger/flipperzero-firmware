#include "symcache.h"
#include "compilesort.hpp"

#include <gui/gui.h>

#include <furi.h>
#include <array>
#include <algorithm>

#define TAG "ElfSym"

#define CACHE_ID_STRING "os_FLIPPER_BUILD"

#define SYM_NOT_FOUND_VA 0xFFFFFFFF
#define CACHE_FILE_MAGIC 3250633246

#define API_METHOD(x)                                    \
    sym_entry {                                          \
        .hash = elf_gnu_hash(#x), .address = (uint32_t)x \
    }

struct sym_entry {
    uint32_t hash;
    uint32_t address;
};

constexpr bool operator<(const sym_entry& k1, const sym_entry& k2) {
    return k1.hash < k2.hash;
}

bool fw_sym_cache_init() {
    do {
        return true;
    } while(0);

    fw_sym_cache_free();
    return false;
}

void fw_sym_cache_free() {
    if(!fw_sym_cache_ready()) {
        return;
    }
}

bool fw_sym_cache_ready() {
    return true;
}

static constexpr uint32_t elf_gnu_hash(const char* s) {
    uint32_t h = 0x1505;
    for(unsigned char c = *s; c != '\0'; c = *++s) h = (h << 5) + h + c;
    return h;
}

constexpr auto syms = sort(create_array_t<sym_entry>(
    API_METHOD(acquire_mutex),
    API_METHOD(canvas_draw_frame),
    API_METHOD(canvas_draw_rframe),
    API_METHOD(canvas_draw_box),
    API_METHOD(canvas_set_color),
    API_METHOD(canvas_set_font),
    API_METHOD(canvas_draw_str),
    API_METHOD(snprintf),
    API_METHOD(canvas_draw_str_aligned),
    API_METHOD(release_mutex),
    API_METHOD(osMessageQueuePut),
    API_METHOD(memset),
    API_METHOD(rand),
    API_METHOD(memmove),
    API_METHOD(srand),
    API_METHOD(osMessageQueueNew),
    API_METHOD(furi_alloc),
    API_METHOD(init_mutex),
    API_METHOD(furi_log_print),
    API_METHOD(free),
    API_METHOD(view_port_alloc),
    API_METHOD(view_port_draw_callback_set),
    API_METHOD(view_port_input_callback_set),
    API_METHOD(osTimerNew),
    API_METHOD(osKernelGetTickFreq),
    API_METHOD(osTimerStart),
    API_METHOD(furi_record_open),
    API_METHOD(gui_add_view_port),
    API_METHOD(osMessageQueueGet),
    API_METHOD(view_port_update),
    API_METHOD(osTimerDelete),
    API_METHOD(view_port_enabled_set),
    API_METHOD(gui_remove_view_port),
    API_METHOD(furi_record_close),
    API_METHOD(view_port_free),
    API_METHOD(osMessageQueueDelete),
    API_METHOD(delete_mutex)));

uint32_t fw_sym_cache_resolve(char* symname) {
    if(!fw_sym_cache_ready()) {
        FURI_LOG_W(TAG, "Cache is not open!");
        return SYM_NOT_FOUND_VA;
    }

    uint32_t gnu_sym_hash = elf_gnu_hash(symname);

    sym_entry key = {.hash = gnu_sym_hash};

    auto find_res = std::lower_bound(syms.cbegin(), syms.cend(), key);
    if((find_res == syms.cend() || (find_res->hash != gnu_sym_hash))) {
        FURI_LOG_W(TAG, "Cant find symbol '%s' (hash %x)!", symname, gnu_sym_hash);
        return SYM_NOT_FOUND_VA;
    }
    
    return find_res->address;
}