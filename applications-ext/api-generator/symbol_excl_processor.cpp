#include "symbol_excl_processor.h"
#include "symbol_filter.h"

bool SymbolExclusionProcessor::process_symbol(
    const char* symname,
    uint32_t address,
    uint8_t bind,
    uint8_t type,
    uint8_t other) {

    parent->save_excluded_symbol(symname);
    return true;
}
