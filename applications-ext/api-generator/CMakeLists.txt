cmake_minimum_required(VERSION 3.15)
project(flipper_apigen C CXX)
# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

include_directories(tkvdb tkvdb/impl ELFIO/)
# add_compile_definitions(_TKV_64BIT_PTR)

add_executable(flipper_apigen flipper_apigen.cpp tkvdb/tkvdb.c elfio_loadsym.cpp symbol_filter.cpp symbol_excl_processor.cpp)

# add_executable(apitest readsym.c testmain.c)
