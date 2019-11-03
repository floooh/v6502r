fips_begin_lib(perfect6502)
    fips_vs_warning_level(3)
    fips_files(
        netlist_6502.h types.h
        netlist_sim.h netlist_sim.c
        perfect6502.h perfect6502.c
    )
fips_end_lib()
