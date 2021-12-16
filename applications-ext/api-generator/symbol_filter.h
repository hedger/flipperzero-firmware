#pragma once

#include "elfio_loadsym.h"
#include "symbol_excl_processor.h"

#include <filesystem>
#include <map>
#include <string>
#include <set>

#include <iostream>

namespace fs = std::filesystem;

class SymbolExclusionFilter {
private:
    std::set<std::string> app_object_sources;
    std::set<std::string> object_src_file_types{".c", ".cpp"};

    std::set<std::string> ignored_symbols;

    SymbolExclusionProcessor excl_processor;

    bool gather_symbols_from_obj(std::string objpath) {
        if(!process_elf(objpath, &excl_processor)) {
            return false;
        }
    }

public:
    void save_excluded_symbol(std::string symname) {
        ignored_symbols.insert(symname);
    }

    bool should_skip(std::string symname) {
        return (ignored_symbols.find(symname) != ignored_symbols.cend());
    }

    SymbolExclusionFilter()
        : excl_processor(this) {
    }

    void gather_sources(std::string rootpath, std::initializer_list<std::string> preserve_apps) {
        // fs::current_path(rootpath);
        std::set<std::string> preserved_apps(preserve_apps);

        for(auto& p : fs::recursive_directory_iterator(rootpath)) {
            if(!fs::is_regular_file(p)) {
                continue;
            }
            fs::path filepath(p);

            bool preserve_symbol = false;
            for(auto& preserved_app : preserved_apps) {
                if(p.path().string().find(preserved_app) != std::string::npos) {
                    std::cout << "preserving " << p << std::endl;
                    preserve_symbol = true;
                    break;
                }
            }

            if(preserve_symbol) {
                continue;
            }
            //std::cout << p.path() << p.path().filename() << '\n';

            auto filename = p.path().filename();
            std::string basename = filename.stem();
            std::string extension = filename.extension();
            if(object_src_file_types.find(extension) == object_src_file_types.cend()) {
                continue;
            }

            //std::cout << basename << " + " << extension << std::endl;
            app_object_sources.insert(basename);
        }
        std::cout << "got " << app_object_sources.size() << " source files" << std::endl;
    }

    void gather_symbols(std::string rootpath) {
        for(auto& p : fs::recursive_directory_iterator(rootpath)) {
            if(!fs::is_regular_file(p)) {
                continue;
            }
            fs::path filepath(p);
            //std::cout << p.path() << p.path().filename() << '\n';

            auto filename = p.path().filename();
            std::string basename = filename.stem();
            std::string extension = filename.extension();
            if(extension != ".o") {
                continue;
            }

            if(app_object_sources.find(basename) == app_object_sources.cend()) {
                continue;
            }

            // std::cout << p.path() << std::endl;
            // app_object_sources.insert(p.path());

            std::cout << "Adding " << p << " to ignore" << std::endl;
            if(!process_elf(p.path(), &excl_processor)) {
                // ...
            }

            std::cout << "Gathered " << ignored_symbols.size() << " symbols to ignore"
                      << std::endl;
        }
    }
};