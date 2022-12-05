#
# project: v6502r
#
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)
project(v6502r)
# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

# shutup MSVC
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
include_directories(ext)
fips_setup()
add_subdirectory(src)
fips_ide_group(Imports)
fips_import_fips_imgui_imgui()
add_subdirectory(ext/perfect6502)
add_subdirectory(ext/texteditor)
add_subdirectory(ext/asmx)
add_subdirectory(ext/sokol)
