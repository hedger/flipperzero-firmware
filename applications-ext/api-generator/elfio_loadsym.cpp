#include "elfio_loadsym.h"

#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>

#include <iostream>
#include <set>

using namespace ELFIO;

static const char* BUILD_ID_STRING = "os_FLIPPER_BUILD";

Elf64_Addr get_ptr_from_ptr(const section* section, Elf64_Addr va) {
    // internally read 32 bits
    if((va < section->get_address()) || (va > section->get_address() + section->get_size())) {
        std::cerr << "va " << va << "lies outside section " << section->get_name() << std::endl;
        return 0;
    }

    Elf64_Addr va_section_offs = va - section->get_address();
    return *(Elf32_Addr*)(section->get_data() + va_section_offs);
}

std::string get_cstr_from_ptr(const section* section, Elf64_Addr va) {
    if((va < section->get_address()) || (va > section->get_address() + section->get_size())) {
        std::cerr << "va " << va << "lies outside section " << section->get_name() << std::endl;
        return "";
    }

    Elf64_Addr va_section_offs = va - section->get_address();
    const char* cstr_ptr = section->get_data();
    //std::cout << " offs = " << va_section_offs;
    return std::string(cstr_ptr + va_section_offs);
}

bool process_elf(const char* fwname, sym_cb callback) {
    auto& out = std::cout;

    elfio reader;
    std::set<uint32_t> gnu_sym_hashes;

    if(!reader.load(fwname)) {
        std::cerr << "Failed to open firmware " << fwname << std::endl;
        return false;
    }

    Elf_Half n = reader.sections.size();

    section* p_rodata = nullptr;
    out << "Looking for .rodata... ";
    for(Elf_Half i = 0; i < n; ++i) { // For all sections
        section* sec = reader.sections[i];
        if(".rodata" == sec->get_name()) {
            out << " is #" << i << std::endl;
            p_rodata = sec;
            break;
        }
    }
    if(p_rodata == nullptr) {
        std::cerr << ".rodata not found!" << std::endl;
        // return false;
    }
    // out << get_cstr_from_ptr(p_rodata, 0x080B79F8);
    // return false;

    for(Elf_Half i = 0; i < n; ++i) { // For all sections
        section* sec = reader.sections[i];
        if(SHT_SYMTAB == sec->get_type() || SHT_DYNSYM == sec->get_type()) {
            symbol_section_accessor symbols(reader, sec);

            Elf_Xword sym_no = symbols.get_symbols_num();
            if(sym_no > 0) {
                for(Elf_Xword i = 0; i < sym_no; ++i) {
                    std::string name;
                    Elf64_Addr value = 0;
                    Elf_Xword size = 0;
                    unsigned char bind = 0;
                    unsigned char type = 0;
                    Elf_Half section = 0;
                    unsigned char other = 0;
                    symbols.get_symbol(i, name, value, size, bind, type, section, other);

                    // out << name << " value " << value << " size " << size << "
                    // bind " << (int)bind << " type " << (int)type << " section " <<
                    // section << " other " << (int)other << std::endl;

                    if(name == "version") {
                        out << "Found VERSION symbol" << std::endl;
                        std::string git_commit =
                            get_cstr_from_ptr(p_rodata, get_ptr_from_ptr(p_rodata, value));
                        // Elf64_Addr version_ptr = get_ptr_from_ptr(p_rodata, value);
                        // out << "struct ptr = " << std::hex << version_ptr <<
                        // std::endl; out << "git_hash " << git_commit;
                        const char* git_commit_cstr = git_commit.c_str();
                        uint32_t version_id_from_hash = strtoul(git_commit_cstr, NULL, 16);
                        // out << " version id=" << version_id_from_hash;
                        callback(
                            BUILD_ID_STRING, version_id_from_hash, STB_GLOBAL, STT_OBJECT, other);
                    }

                    if(((type != STT_OBJECT) && (type != STT_FUNC)) || (bind != STB_GLOBAL)) {
                        continue;
                    }

                    uint32_t sym_hash =
                        elf_gnu_hash(reinterpret_cast<const unsigned char*>(name.c_str()));

                    if (gnu_sym_hashes.find(sym_hash) != gnu_sym_hashes.cend()) {
                        std::cerr << "HASH COLLISION DETECTED FOR SYMBOL " << name << " = " << sym_hash << std::endl;
                    }
                    gnu_sym_hashes.insert(sym_hash);
                    //out << name << " = " << sym_hash << std::endl;

                    callback(name.c_str(), static_cast<uint32_t>(value), bind, type, other);
                }

                out << "total symbols processed: " << gnu_sym_hashes.size() << std::endl;
                out << std::endl;
            }
        }
    }
    return true;
}
