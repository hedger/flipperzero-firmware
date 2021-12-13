#include "elfio_loadsym.h"

#include <elfio/elfio.hpp>
#include <iostream>
#include <elfio/elfio_dump.hpp>

using namespace ELFIO;

bool process_elf(const char* fwname, sym_cb callback) {
	elfio reader;

	if (!reader.load(fwname)) {
		std::cerr << "Failed to open firmware " << fwname << std::endl;
		return false;
	}

	auto& out = std::cout;
    Elf_Half n = reader.sections.size();
    for ( Elf_Half i = 0; i < n; ++i ) { // For all sections
        section* sec = reader.sections[i];
        if ( SHT_SYMTAB == sec->get_type() ||
             SHT_DYNSYM == sec->get_type() ) {
            symbol_section_accessor symbols( reader, sec );

            Elf_Xword sym_no = symbols.get_symbols_num();
            if ( sym_no > 0 ) {
                out << "Symbol table (" << sec->get_name() << ")"
                    << std::endl;
                if ( reader.get_class() ==
                     ELFCLASS32 ) { // Output for 32-bit
                    out << "[  Nr ] Value      Size       Type    Bind     "
                           " Sect Name"
                        << std::endl;
                }
                else { // Output for 64-bit
                    out << "[  Nr ] Value              Size               "
                           "Type    Bind      Sect"
                        << std::endl
                        << "        Name" << std::endl;
                }
                for ( Elf_Xword i = 0; i < sym_no; ++i ) {
                    std::string   name;
                    Elf64_Addr    value   = 0;
                    Elf_Xword     size    = 0;
                    unsigned char bind    = 0;
                    unsigned char type    = 0;
                    Elf_Half      section = 0;
                    unsigned char other   = 0;
                    symbols.get_symbol( i, name, value, size, bind, type,
                                        section, other );

                    out << "sym " << name << "type " << (uint32_t)type << " @ " << value << std::endl;
                    callback(name.c_str(), static_cast<uint32_t>(value), type);
                    // symbol_table( out, i, name, value, size, bind, type,
                    //               section, reader.get_class() );
                }

                out << std::endl;
            }
        }
    }
    return true;
}