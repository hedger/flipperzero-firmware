#include "flash_manager_file_tools.h"

static inline const char* get_suffix_for_chip(ChipType chip) {
    switch(chip) {
    case ChipType::SPI:
        return ".spi";
    case ChipType::I2C:
        return ".i2c";
    default:
        return "";
    }
}

const char* FlashManagerFileTools::get_app_folder() {
    return dump_directory;
}

bool FlashManagerFileTools::make_app_folder() {
    //if(!storage_simply_mkdir(storage, app_folder)) {
    //    dialog_message_show_storage_error(dialogs, "Cannot create\napp folder");
    //}
    //FURI_LOG_I(TAG, "folder check ok");
    return file_worker.mkdir(dump_directory);
}

bool FlashManagerFileTools::open_dump_file_read(const char* filename, ChipType chip) {
    std::string full_filename = make_full_name(filename, chip);
    ;
    return file_worker.open(full_filename.c_str(), FSAM_READ, FSOM_OPEN_EXISTING);
}

bool FlashManagerFileTools::open_dump_file_write(const char* filename, ChipType chip) {
    std::string dirname(dump_directory);
    auto full_filename = make_full_name(filename, chip);

    if(!file_worker.mkdir(dirname.c_str())) return false;

    return file_worker.open(full_filename.c_str(), FSAM_WRITE, FSOM_CREATE_ALWAYS);
}

bool FlashManagerFileTools::is_dump_file_exist(const char* filename, bool* exist) {
    std::string full_path = make_full_name(filename, ChipType::Unknown);
    return file_worker.is_file_exist(full_path.c_str(), exist);
}

std::string FlashManagerFileTools::make_full_name(const std::string& name, ChipType chip) const {
    if(!name.empty() && (name[0] == '/')) { // assume it's absolute path -> no processing needed
        return name;
    }
    return std::string("") + dump_directory + "/" + name + get_suffix_for_chip(chip) +
           dump_extension;
}

bool FlashManagerFileTools::rename_dump_file(const char* filename, const char* newname) {
    std::string old_filename = make_full_name(filename, ChipType::Unknown);
    std::string new_filename = make_full_name(newname, ChipType::Unknown);
    return file_worker.rename(old_filename.c_str(), new_filename.c_str());
}

bool FlashManagerFileTools::remove_dump_file(const char* name, ChipType chip) {
    std::string full_filename = make_full_name(name, chip);
    return file_worker.remove(full_filename.c_str());
}

bool FlashManagerFileTools::close() {
    return file_worker.close();
}

bool FlashManagerFileTools::check_errors() {
    return file_worker.check_errors();
}

uint32_t FlashManagerFileTools::get_size() {
    return file_worker.get_size();
}

bool FlashManagerFileTools::write_buffer(const uint8_t* data, size_t length) {
    return file_worker.write(data, length);
}

bool FlashManagerFileTools::read_buffer(uint8_t* data, size_t max_length) {
    return file_worker.read(data, max_length);
}

bool FlashManagerFileTools::seek(size_t offset) {
    return file_worker.seek(offset, true);
}

size_t FlashManagerFileTools::ftell() {
    uint64_t position;
    file_worker.tell(&position);
    return static_cast<size_t>(position);
}