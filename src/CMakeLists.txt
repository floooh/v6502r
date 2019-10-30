if (FIPS_EMSCRIPTEN)
    add_definitions(-DSOKOL_GLES2)
elseif (FIPS_RASPBERRYPI)
    add_definitions(-DSOKOL_GLES2)
elseif (FIPS_ANDROID)
    add_definitions(-DSOKOL_GLES3)
elseif (FIPS_WINDOWS)
    add_definitions(-DSOKOL_D3D11)
elseif (FIPS_OSX)
    add_definitions(-DSOKOL_METAL)
endif()

fips_begin_lib(sokol)
    fips_vs_warning_level(3)
    if (FIPS_OSX)
        fips_files(sokol.m)
        if (FIPS_IOS)
            fips_frameworks_osx(UIKit Metal MetalKit AudioToolbox)
        else()
            fips_frameworks_osx(Cocoa QuartzCore Metal MetalKit AudioToolbox)
        endif()
    else()
        fips_files(sokol.c)
        if (FIPS_ANDROID)
            fips_libs(GLESv3 EGL OpenSLES log android)
        elseif (FIPS_LINUX)
            fips_libs(X11 GL m dl asound)
        endif()
    endif()
fips_end_lib()

fips_begin_app(v6502r windowed)
    fips_vs_warning_level(3)
    fips_files(
        v6502r.c
        segdefs.c segdefs.h
        ui.h ui.cc
        ui_hello.cc
        ui_chip.cc
    )
    fips_deps(sokol imgui)
fips_end_app()
