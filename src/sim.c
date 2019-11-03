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
}

void sim_shutdown(void) {
    if (p6502_state) {
        destroyChip(p6502_state);
        p6502_state = 0;
    }
}

void sim_start(uint16_t start_addr) {
    // patch reset vector and run through the 9-cycle reset sequence
    sim_w16(0xFFFC, start_addr);
    for (int i = 0; i < 9; i++) {
        step(p6502_state);
        step(p6502_state);
    }
}

void sim_step(int num_half_cycles) {
    assert(p6502_state);
    for (int i = 0; i < num_half_cycles; i++) {
        step(p6502_state);
    }
}

void sim_update_nodestate(void) {
    // read the transistor simulation's node state into app.nodestate for visualization
    assert(p6502_state);
    p6502_read_node_state(p6502_state, app.chipvis.node_state, MAX_NODES);
}

void sim_w8(uint16_t addr, uint8_t val) {
    memory[addr] = val;
}

uint8_t sim_r8(uint16_t addr) {
    return memory[addr];
}

void sim_w16(uint16_t addr, uint16_t val) {
    memory[addr] = val;
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
