#pragma once

#include <file-worker-cpp.h>
#include <memory>
#include <string>
#include <cstdint>

class FlashManagerFileTools {
public:
    bool make_app_folder();
    bool open_dump_file_read(const char* filename);
    bool open_dump_file_write(const char* filename);
    bool is_dump_file_exist(const char* filename, bool* exist);
    bool rename_dump_file(const char* filename, const char* newname);
    bool remove_dump_file(const char* name);
    bool close();
    bool check_errors();

    // std::unique_ptr<IrdaAppFileParser::IrdaFileSignal> read_signal();
    // bool save_signal(const IrdaAppSignal& signal, const char* name);
    std::string file_select(const char* selected);

    std::string make_name(const std::string& full_name) const;

private:
    // std::unique_ptr<IrdaFileSignal> parse_signal(const std::string& str) const;
    // std::unique_ptr<IrdaFileSignal> parse_signal_raw(const std::string& str) const;
    std::string make_full_name(const std::string& name) const;

    static inline const char* const dump_directory = "/any/flash";
    static inline const char* const dump_extension = ".bin";

    FileWorkerCpp file_worker;
    char file_buf[128];
    size_t file_buf_cnt = 0;
};
