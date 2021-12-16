#pragma once

#include <string>
#include <cstdint>

class SymbolProcessor {
public:
    virtual ~SymbolProcessor() {};
    virtual bool init_database(const char* fname) = 0;
    virtual bool process_symbol(
        const char* symname,
        uint32_t address,
        uint8_t bind,
        uint8_t type,
        uint8_t other) = 0;
};

bool process_elf(std::string objname, SymbolProcessor* processor);
