#
# project: v6502r
#
cmake_minimum_required(VERSION 2.8)
project(v6502r)
# include the fips main cmake file
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

fips_setup()
fips_import_fips_imgui_imgui()
fips_add_subdirectory(src)
fips_finish()


