fips_begin_lib(asmx)
    fips_vs_warning_level(3)
    fips_dir(src)
    fips_files(asmx.c asmx.h asm6502.c)
fips_end_lib()
target_include_directories(asmx INTERFACE src)

