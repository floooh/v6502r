//------------------------------------------------------------------------------
//  sim.c
//
//  Wrapper functions around perfect6502.
//------------------------------------------------------------------------------
#include "perfect6502.h"
#include "sim.h"
#include "trace.h"
#include "gfx.h"

void sim_init_or_reset(sim_t* sim) {
    assert(sim);
    if (sim->p6502_state) {
        destroyChip(sim->p6502_state);
        sim->p6502_state = 0;
    }
    sim->p6502_state = initAndResetChip();
    sim->paused = true;
}

void sim_shutdown(sim_t* sim) {
    assert(sim);
    if (sim->p6502_state) {
        destroyChip(sim->p6502_state);
        sim->p6502_state = 0;
    }
}

void sim_start(sim_t* sim, trace_t* trace) {
    assert(sim);
    sim_step(sim, trace, 17);
}

void sim_frame(sim_t* sim, trace_t* trace) {
    assert(sim && trace);
    if (!sim->paused) {
        sim_step(sim, trace, 1);
    }
    sim_update_nodestate(sim, gfx_get_nodestate());
}

void sim_pause(sim_t* sim, bool paused) {
    assert(sim);
    sim->paused = paused;
}

bool sim_paused(const sim_t* sim) {
    return sim->paused;
}

void sim_step(sim_t* sim, trace_t* trace, int num_half_cycles) {
    assert(sim && trace && sim->p6502_state);
    for (int i = 0; i < num_half_cycles; i++) {
        step(sim->p6502_state);
        trace_store(trace, sim);
    }
}

void sim_step_op(sim_t* sim, trace_t* trace) {
    int num_sync = 0;
    do {
        sim_step(sim, trace, 1);
        if (readSYNC(sim->p6502_state)) {
            num_sync++;
        }
    }
    while (num_sync != 2);
}

void sim_update_nodestate(sim_t* sim, range_t from_buffer) {
    assert(sim && from_buffer.ptr && (from_buffer.size == MAX_NODES));
    // read the transistor simulation's node state into app.nodestate for visualization
    assert(sim && sim->p6502_state);
    p6502_read_node_state_as_bytes(sim->p6502_state, from_buffer.ptr, MAX_NODES);
}

void sim_w8(uint16_t addr, uint8_t val) {
    memory[addr] = val;
}

uint8_t sim_r8(uint16_t addr) {
    return memory[addr];
}

void sim_w16(uint16_t addr, uint16_t val) {
    memory[addr] = (uint8_t) val;
    memory[(addr+1)&0xFFFF] = val>>8;
}

uint16_t sim_r16(uint16_t addr) {
    return (memory[(addr+1) & 0xFFFF]<<8) | memory[addr];
}

void sim_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr) {
    for (uint16_t i = 0; i < num_bytes; i++) {
        memory[(addr+i)&0xFFFF] = ptr[i];
    }
}

void sim_clear(uint16_t addr, uint16_t num_bytes) {
    for (uint16_t i = 0; i < num_bytes; i++) {
        memory[(addr+i)&0xFFFF] = 0;
    }
}

uint8_t sim_get_a(const sim_t* sim) {
    assert(sim);
    return readA(sim->p6502_state);
}

uint8_t sim_get_x(const sim_t* sim) {
    assert(sim);
    return readX(sim->p6502_state);
}

uint8_t sim_get_y(const sim_t* sim) {
    assert(sim);
    return readY(sim->p6502_state);
}

uint8_t sim_get_sp(const sim_t* sim) {
    assert(sim);
    return readSP(sim->p6502_state);
}

uint16_t sim_get_pc(const sim_t* sim) {
    assert(sim);
    return readPC(sim->p6502_state);
}

uint8_t sim_get_ir(const sim_t* sim) {
    assert(sim);
    return readIR(sim->p6502_state);
}

uint16_t sim_get_addr(const sim_t* sim) {
    assert(sim);
    return readAddressBus(sim->p6502_state);
}

uint8_t sim_get_data(const sim_t* sim) {
    assert(sim);
    return readDataBus(sim->p6502_state);
}

uint8_t sim_get_p(const sim_t* sim) {
    assert(sim);
    return readP(sim->p6502_state);
}

bool sim_get_clk0(const sim_t* sim) {
    assert(sim);
    return readCLK0(sim->p6502_state) != 0;
}

bool sim_get_rw(const sim_t* sim) {
    assert(sim);
    return readRW(sim->p6502_state) != 0;
}

bool sim_get_sync(const sim_t* sim) {
    assert(sim);
    return readSYNC(sim->p6502_state) != 0;
}

uint32_t sim_get_cycle(void) {
    return cycle + 1;
}

void sim_set_cycle(uint32_t c) {
    cycle = c - 1;
}

void sim_set_rdy(sim_t* sim, bool high) {
    assert(sim);
    writeRDY(sim->p6502_state, high ? 1 : 0);
}

bool sim_get_rdy(const sim_t* sim) {
    assert(sim);
    return 0 != readRDY(sim->p6502_state);
}

void sim_set_irq(sim_t* sim, bool high) {
    assert(sim);
    writeIRQ(sim->p6502_state, high ? 1 : 0);
}

bool sim_get_irq(const sim_t* sim) {
    return 0 != readIRQ(sim->p6502_state);
}

void sim_set_nmi(sim_t* sim, bool high) {
    writeNMI(sim->p6502_state, high ? 1 : 0);
}

bool sim_get_nmi(const sim_t* sim) {
    return 0 != readNMI(sim->p6502_state);
}

void sim_set_res(sim_t* sim, bool high) {
    writeRES(sim->p6502_state, high ? 1 : 0);
}

bool sim_get_res(const sim_t* sim) {
    return 0 != readRES(sim->p6502_state);
}

bool sim_read_node_values(const sim_t* sim, uint8_t* ptr, int max_bytes) {
    assert(sim);
    return 0 != p6502_read_node_values(sim->p6502_state, ptr, max_bytes);
}

bool sim_read_transistor_on(const sim_t* sim, uint8_t* ptr, int max_bytes) {
    assert(sim);
    return 0 != p6502_read_transistor_on(sim->p6502_state, ptr, max_bytes);
}

void sim_write_node_values(sim_t* sim, const uint8_t* ptr, int max_bytes) {
    assert(sim);
    p6502_write_node_values(sim->p6502_state, ptr, max_bytes);
}

void sim_write_transistor_on(sim_t* sim, const uint8_t* ptr, int max_bytes) {
    assert(sim);
    p6502_write_transistor_on(sim->p6502_state, ptr, max_bytes);
}

