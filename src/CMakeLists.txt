if (FIPS_EMSCRIPTEN)
    set(slang "glsl300es")
elseif (FIPS_WINDOWS)
    set(slang "hlsl4:glsl430")
elseif (FIPS_OSX)
    if (FIPS_IOS)
        set(slang "metal_ios:metal_sim")
    else()
        set(slang "metal_macos")
    endif()
else()
    set(slang "glsl430")
endif()

fips_begin_lib(common)
    fips_files(dummy.c)
    sokol_shader(gfx.glsl ${slang})
    fips_dir(res)
    fips_generate(FROM fonts.yml TYPE dump HEADER fonts.h)
    fips_generate(FROM markdown.yml TYPE dump HEADER markdown.h)
    fips_files(help_assembler.md about.md)
    fips_deps(sokol texteditor)
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
    imgui_util.cc imgui_util.h
    common.h
)

fips_begin_app(v6502r windowed)
    fips_files(${SOURCES})
    fips_dir(m6502)
    fips_files(segdefs.c segdefs.h)
    fips_files(nodenames.c nodenames.h nodegroups.h)
    fips_deps(common perfect6502 asmx_6502)
fips_end_app()
target_compile_definitions(v6502r PRIVATE CHIP_6502)
target_include_directories(v6502r PRIVATE m6502)
if (FIPS_EMSCRIPTEN)
    target_link_options(v6502r PUBLIC --shell-file ${CMAKE_SOURCE_DIR}/src/m6502/shell.html)
endif()

fips_begin_app(vz80r windowed)
    fips_files(${SOURCES})
    fips_dir(z80)
    fips_files(segdefs.c segdefs.h)
    fips_files(nodenames.c nodenames.h nodegroups.h)
    fips_deps(common perfectz80 asmx_z80)
fips_end_app()
target_compile_definitions(vz80r PRIVATE CHIP_Z80)
target_include_directories(vz80r PRIVATE z80)
if (FIPS_EMSCRIPTEN)
    target_link_options(vz80r PUBLIC --shell-file ${CMAKE_SOURCE_DIR}/src/z80/shell.html)
endif()

fips_begin_app(v2a03r windowed)
    fips_files(${SOURCES})
    fips_dir(2a03)
    fips_files(segdefs.c segdefs.h)
    fips_files(nodenames.c nodenames.h nodegroups.h)
    fips_deps(common perfect2a03 asmx_6502)
fips_end_app()
target_compile_definitions(v2a03r PRIVATE CHIP_2A03)
target_include_directories(v2a03r PRIVATE 2a03)
if (FIPS_EMSCRIPTEN)
    target_link_options(v2a03r PUBLIC --shell-file ${CMAKE_SOURCE_DIR}/src/2a03/shell.html)
endif()
