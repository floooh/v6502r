//------------------------------------------------------------------------------
//  sim.c
//
//  Wrapper functions around perfect6502.
//------------------------------------------------------------------------------
#include "perfect6502.h"
#include "sim.h"
#include "trace.h"
#include "gfx.h"

static struct {
    bool valid;
    bool paused;
    void* p6502_state;
} sim;

void sim_init(void) {
    assert(!sim.valid);
    assert(0 == sim.p6502_state);
    sim.p6502_state = initAndResetChip();
    assert(sim.p6502_state);
    sim.paused = true;
    sim.valid = true;
}

void sim_shutdown(void) {
    assert(sim.valid);
    assert(0 != sim.p6502_state);
    destroyChip(sim.p6502_state);
    memset(&sim, 0, sizeof(sim));
}

void sim_start(void) {
    assert(sim.valid);
    sim_step(17);
}

void sim_frame(void) {
    assert(sim.valid);
    if (!sim.paused) {
        sim_step(1);
    }
}

void sim_set_paused(bool paused) {
    assert(sim.valid);
    sim.paused = paused;
}

bool sim_get_paused(void) {
    assert(sim.valid);
    return sim.paused;
}

void sim_step(int num_half_cycles) {
    assert(sim.valid);
    for (int i = 0; i < num_half_cycles; i++) {
        step(sim.p6502_state);
        trace_store();
    }
}

void sim_step_op(void) {
    assert(sim.valid);
    int num_sync = 0;
    do {
        sim_step(1);
        if (readSYNC(sim.p6502_state)) {
            num_sync++;
        }
    }
    while (num_sync != 2);
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
    assert(sim.valid);
    return readA(sim.p6502_state);
}

uint8_t sim_get_x(void) {
    assert(sim.valid);
    return readX(sim.p6502_state);
}

uint8_t sim_get_y(void) {
    assert(sim.valid);
    return readY(sim.p6502_state);
}

uint8_t sim_get_sp(void) {
    assert(sim.valid);
    return readSP(sim.p6502_state);
}

uint16_t sim_get_pc(void) {
    assert(sim.valid);
    return readPC(sim.p6502_state);
}

uint8_t sim_get_ir(void) {
    assert(sim.valid);
    return readIR(sim.p6502_state);
}

uint16_t sim_get_addr(void) {
    assert(sim.valid);
    return readAddressBus(sim.p6502_state);
}

uint8_t sim_get_data(void) {
    assert(sim.valid);
    return readDataBus(sim.p6502_state);
}

uint8_t sim_get_p(void) {
    assert(sim.valid);
    return readP(sim.p6502_state);
}

bool sim_get_clk0(void) {
    assert(sim.valid);
    return readCLK0(sim.p6502_state) != 0;
}

bool sim_get_rw(void) {
    assert(sim.valid);
    return readRW(sim.p6502_state) != 0;
}

bool sim_get_sync(void) {
    assert(sim.valid);
    return readSYNC(sim.p6502_state) != 0;
}

uint32_t sim_get_cycle(void) {
    return cycle + 1;
}

void sim_set_cycle(uint32_t c) {
    cycle = c - 1;
}

void sim_set_rdy(bool high) {
    assert(sim.valid);
    writeRDY(sim.p6502_state, high ? 1 : 0);
}

bool sim_get_rdy(void) {
    assert(sim.valid);
    return 0 != readRDY(sim.p6502_state);
}

void sim_set_irq(bool high) {
    assert(sim.valid);
    writeIRQ(sim.p6502_state, high ? 1 : 0);
}

bool sim_get_irq(void) {
    assert(sim.valid);
    return 0 != readIRQ(sim.p6502_state);
}

void sim_set_nmi(bool high) {
    assert(sim.valid);
    writeNMI(sim.p6502_state, high ? 1 : 0);
}

bool sim_get_nmi(void) {
    assert(sim.valid);
    return 0 != readNMI(sim.p6502_state);
}

void sim_set_res(bool high) {
    assert(sim.valid);
    writeRES(sim.p6502_state, high ? 1 : 0);
}

bool sim_get_res(void) {
    assert(sim.valid);
    return 0 != readRES(sim.p6502_state);
}

bool sim_get_node_state(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return 0 != p6502_read_node_state_as_bytes(sim.p6502_state, to_buffer.ptr, to_buffer.size);
}

bool sim_get_node_values(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return 0 != p6502_read_node_values(sim.p6502_state, to_buffer.ptr, to_buffer.size);
}

bool sim_get_transistor_on(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return 0 != p6502_read_transistor_on(sim.p6502_state, to_buffer.ptr, to_buffer.size);
}

bool sim_set_node_values(range_t from_buffer) {
    assert(sim.valid && from_buffer.ptr && (from_buffer.size > 0));
    return 0 != p6502_write_node_values(sim.p6502_state, from_buffer.ptr, from_buffer.size);
}

bool sim_set_transistor_on(range_t from_buffer) {
    assert(sim.valid && from_buffer.ptr && (from_buffer.size > 0));
    return 0 != p6502_write_transistor_on(sim.p6502_state, from_buffer.ptr, from_buffer.size);
}
