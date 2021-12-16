#pragma once

#include "elfio_loadsym.h"

class SymbolExclusionFilter;

class SymbolExclusionProcessor : public SymbolProcessor {
private:
    SymbolExclusionFilter* parent;

public:
    SymbolExclusionProcessor(SymbolExclusionFilter* _parent)
        : parent(_parent) {
    }

    ~SymbolExclusionProcessor(){};

    bool init_database(const char* fname) override {
        return true;
    }

    bool process_symbol(
        const char* symname,
        uint32_t address,
        uint8_t bind,
        uint8_t type,
        uint8_t other) override;
};