#
# project: v6502r
#
cmake_minimum_required(VERSION 3.0)
project(v6502r)
# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

# shutup MSVC
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
include_directories(ext)
fips_setup()
fips_add_subdirectory(src)
fips_ide_group(Imports)
fips_import_fips_imgui_imgui()
fips_add_subdirectory(ext/perfect6502)
fips_add_subdirectory(ext/texteditor)
fips_add_subdirectory(ext/asmx)
fips_finish()


