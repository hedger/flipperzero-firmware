cmake_minimum_required(VERSION 3.15)
project(flipper_apigen C CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
include_directories(tkvdb tkvdb/impl ELFIO/)

# add_compile_definitions(_TKV_64BIT_PTR)

add_executable(flipper_apigen flipper_apigen.c tkvdb/tkvdb.c elfio_loadsym.cpp)
