fips_begin_lib(texteditor)
    fips_vs_warning_level(3)
    fips_files(TextEditor.cpp TextEditor.h)
    fips_deps(imgui)
fips_end_lib()
target_include_directories(texteditor INTERFACE .)

