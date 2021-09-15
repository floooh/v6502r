if (FIPS_EMSCRIPTEN)
    set(slang "glsl100")
elseif (FIPS_WINDOWS)
    set(slang "hlsl4:glsl330")
elseif (FIPS_OSX)
    if (FIPS_IOS)
        set(slang "metal_ios:metal_sim")
    else()
        set(slang "metal_macos")
    endif()
else()
    set(slang "glsl330")
endif()

fips_begin_lib(common)
    fips_vs_warning_level(3)
    fips_files(common.c common.h)
    sokol_shader(gfx.glsl ${slang})
    fips_dir(res)
    fips_generate(FROM fonts.yml TYPE dump HEADER fonts.h)
    fips_generate(FROM markdown.yml TYPE dump HEADER markdown.h)
    fips_files(help_assembler.md about.md favicon.png)
    fips_deps(sokol texteditor asmx)
fips_end_lib()

set(SOURCES
    main.c
    input.c input.h
    util.c util.h
    gfx.c gfx.h
    pick.c pick.h
    sim.c sim.h
    trace.c trace.h
    asm.c asm.h
    ui.cc ui.h
    ui_asm.cc ui_asm.h
)

fips_begin_app(v6502r windowed)
    fips_vs_warning_level(3)
    fips_files(${SOURCES})
    fips_dir(m6502)
    fips_files(segdefs.c segdefs.h)
    fips_files(nodenames.c nodenames.h)
    fips_deps(common perfect6502)
fips_end_app()
target_compile_definitions(v6502r PRIVATE V6502R_6502)
target_include_directories(v6502r PRIVATE m6502)

fips_begin_app(vz80r windowed)
    fips_vs_warning_level(3)
    fips_files(${SOURCES})
    fips_dir(z80)
    fips_files(segdefs.c segdefs.h)
    fips_files(nodenames.c nodenames.h)
    fips_deps(common perfect6502)
fips_end_app()
target_compile_definitions(vz80r PRIVATE V6502R_Z80)
target_include_directories(vz80r PRIVATE z80)
