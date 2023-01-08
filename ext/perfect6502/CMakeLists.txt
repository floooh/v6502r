fips_begin_lib(perfect6502)
    fips_files(
        netlist_6502.h types.h
        netlist_sim.h netlist_sim.c
        perfect6502.h perfect6502.c
    )
fips_end_lib()
target_include_directories(perfect6502 INTERFACE .)

fips_begin_lib(perfectz80)
    fips_files(
        netlist_z80.h types.h
        netlist_sim.h netlist_sim.c
        perfectz80.h perfectz80.c
    )
fips_end_lib()
target_include_directories(perfectz80 INTERFACE .)

fips_begin_lib(perfect2a03)
    fips_files(
        netlist_2a03.h types.h
        netlist_sim.h netlist_sim.c
        perfect2a03.h perfect2a03.c
    )
fips_end_lib()
target_include_directories(perfect2a03 INTERFACE .)

