//------------------------------------------------------------------------------
//  sim.c
//
//  Wrapper functions around perfect6502.
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "perfect6502.h"

static void* p6502_state;

void sim_init_or_reset(void) {
    if (p6502_state) {
        destroyChip(p6502_state);
        p6502_state = 0;
    }
    p6502_state = initAndResetChip();
    app.sim.paused = true;
}

void sim_shutdown(void) {
    if (p6502_state) {
        destroyChip(p6502_state);
        p6502_state = 0;
    }
}

void sim_start(uint16_t start_addr) {
    // patch reset vector and run through the 9-cycle reset sequence,
    // but stop before the first post-reset instruction byte is set
    sim_w16(0xFFFC, start_addr);
    sim_step(17);
}

void sim_frame(void) {
    if (!app.sim.paused) {
        sim_step(1);
    }
    sim_update_nodestate();
}

void sim_pause(bool paused) {
    app.sim.paused = paused;
}

bool sim_paused(void) {
    return app.sim.paused;
}

void sim_step(int num_half_cycles) {
    assert(p6502_state);
    for (int i = 0; i < num_half_cycles; i++) {
        step(p6502_state);
        trace_store();
    }
}

void sim_step_op(void) {
    int num_sync = 0;
    do {
        sim_step(1);
        if (readSYNC(p6502_state)) {
            num_sync++;
        }
    }
    while (num_sync != 2);
}

void sim_update_nodestate(void) {
    // read the transistor simulation's node state into app.nodestate for visualization
    assert(p6502_state);
    p6502_read_node_state_as_bytes(p6502_state, app.chipvis.node_state, MAX_NODES);
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

uint8_t sim_get_a(void) {
    return readA(p6502_state);
}

uint8_t sim_get_x(void) {
    return readX(p6502_state);
}

uint8_t sim_get_y(void) {
    return readY(p6502_state);
}

uint8_t sim_get_sp(void) {
    return readSP(p6502_state);
}

uint16_t sim_get_pc(void) {
    return readPC(p6502_state);
}

uint8_t sim_get_ir(void) {
    return readIR(p6502_state);
}

uint16_t sim_get_addr(void) {
    return readAddressBus(p6502_state);
}

uint8_t sim_get_data(void) {
    return readDataBus(p6502_state);
}

uint8_t sim_get_p(void) {
    return readP(p6502_state);
}

bool sim_get_clk0(void) {
    return readCLK0(p6502_state) != 0;
}

bool sim_get_rw(void) {
    return readRW(p6502_state) != 0;
}

bool sim_get_sync(void) {
    return readSYNC(p6502_state) != 0;
}

uint32_t sim_get_cycle(void) {
    return cycle+1;
}

void sim_set_cycle(uint32_t c) {
    cycle = c - 1;
}

bool sim_read_node_values(uint8_t* ptr, int max_bytes) {
    return 0 != p6502_read_node_values(p6502_state, ptr, max_bytes);
}

bool sim_read_transistor_on(uint8_t* ptr, int max_bytes) {
    return 0 != p6502_read_transistor_on(p6502_state, ptr, max_bytes);
}

void sim_write_node_values(const uint8_t* ptr, int max_bytes) {
    p6502_write_node_values(p6502_state, ptr, max_bytes);
}

void sim_write_transistor_on(const uint8_t* ptr, int max_bytes) {
    p6502_write_transistor_on(p6502_state, ptr, max_bytes);
}
