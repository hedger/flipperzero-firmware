#pragma once

#include "chip_type.h"

#include <file_worker_cpp.h>
#include <memory>
#include <string>
#include <cstdint>

class FlashManagerFileTools {
public:
    bool make_app_folder();
    bool open_dump_file_read(const char* filename, ChipType chip);
    bool open_dump_file_write(const char* filename, ChipType chip);
    bool is_dump_file_exist(const char* filename, bool* exist);
    bool rename_dump_file(const char* filename, const char* newname);
    bool remove_dump_file(const char* name, ChipType chip);
    bool close();
    bool check_errors();
    uint32_t get_size();
    bool write_buffer(const uint8_t* data, size_t length);
    bool read_buffer(uint8_t* data, size_t max_length); 
    bool seek(size_t offset);
    size_t ftell();

    // std::unique_ptr<IrdaAppFileParser::IrdaFileSignal> read_signal();
    // bool save_signal(const IrdaAppSignal& signal, const char* name);
    std::string file_select(const char* selected);

    std::string make_name(const std::string& full_name) const;

    static const char* get_app_folder();

private:
    // std::unique_ptr<IrdaFileSignal> parse_signal(const std::string& str) const;
    // std::unique_ptr<IrdaFileSignal> parse_signal_raw(const std::string& str) const;
    std::string make_full_name(const std::string& name, ChipType chip) const;

    static inline const char* const dump_directory = "/ext/flash";
    static inline const char* const dump_extension = ".bin";

    FileWorkerCpp file_worker;
    char file_buf[128];
    size_t file_buf_cnt = 0;
};