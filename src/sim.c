//------------------------------------------------------------------------------
//  sim.c
//
//  Wrapper functions around perfect6502.
//------------------------------------------------------------------------------
#if defined(CHIP_6502)
#include "perfect6502.h"
#elif defined(CHIP_Z80)
#include "perfectz80.h"
#endif
#include "nodenames.h"
#include "sim.h"
#include "trace.h"
#include "gfx.h"
#include <stdlib.h> // qsort, bsearch, strtol

typedef struct {
    const char* name;
    int index;
} sorted_node_t;

static struct {
    bool valid;
    bool paused;
    void* cpu_state;
    int num_sorted_nodes;
    sorted_node_t sorted_nodes[MAX_NODES];
} sim;

static int qsort_cb(const void* a, const void* b) {
    const sorted_node_t* n0 = a;
    const sorted_node_t* n1 = b;
    return strcmp(n0->name, n1->name);
}

static int bsearch_cb(const void* key, const void* elem) {
    const sorted_node_t* n = elem;
    return strcmp((const char*)key, n->name);
}

void sim_init(void) {
    assert(!sim.valid);
    assert(0 == sim.cpu_state);
    sim.cpu_state = cpu_initAndResetChip();
    assert(sim.cpu_state);
    sim.paused = true;
    sim.valid = true;

    // create a sorted node array for fast lookup by node name
    assert(num_node_names < MAX_NODES);
    for (int i = 0; i < num_node_names; i++) {
        if (node_names[i][0]) {
            sorted_node_t* n = &sim.sorted_nodes[sim.num_sorted_nodes++];
            n->name = node_names[i];
            n->index = i;
        }
    }
    qsort(sim.sorted_nodes, (size_t)sim.num_sorted_nodes, sizeof(sorted_node_t), qsort_cb);
}

void sim_shutdown(void) {
    assert(sim.valid);
    assert(0 != sim.cpu_state);
    cpu_destroyChip(sim.cpu_state);
    memset(&sim, 0, sizeof(sim));
}

void sim_start(void) {
    assert(sim.valid);
    // on 6502, run through the initial reset sequence
    #if defined(CHIP_6502)
    sim_step(17);
    #endif
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
        cpu_step(sim.cpu_state);
        trace_store();
    }
}

void sim_step_op(void) {
    assert(sim.valid);
    #if defined(CHIP_6502)
        int num_sync = 0;
        do {
            sim_step(1);
            if (cpu_readSYNC(sim.cpu_state)) {
                num_sync++;
            }
        }
        while (num_sync != 2);
    #elif defined(CHIP_Z80)
        bool prev_m1, cur_m1;
        do {
            prev_m1 = cpu_readM1(sim.cpu_state);
            sim_step(1);
            cur_m1 = cpu_readM1(sim.cpu_state);
        } while (!(prev_m1 && !cur_m1));   // M1 pin is active-low!
    #endif
}

int sim_find_node(const char* name) {
    assert(name);
    // special case '#1234'
    if (name[0] == '#') {
        int node_index = strtol(&name[1], 0, 10);
        if ((node_index >= 1) && (node_index < sim_get_num_nodes())) {
            return node_index;
        }
    }
    else {
        const sorted_node_t* n = bsearch(name, sim.sorted_nodes, (size_t)sim.num_sorted_nodes, sizeof(sorted_node_t), bsearch_cb);
        if (n) {
            return n->index;
        }
    }
    // fallthrough: not found or invalid '#' number
    return -1;
}

void sim_mem_w8(uint16_t addr, uint8_t val) {
    cpu_memory[addr] = val;
}

uint8_t sim_mem_r8(uint16_t addr) {
    return cpu_memory[addr];
}

void sim_mem_w16(uint16_t addr, uint16_t val) {
    cpu_memory[addr] = (uint8_t) val;
    cpu_memory[(addr+1)&0xFFFF] = val>>8;
}

uint16_t sim_mem_r16(uint16_t addr) {
    return (cpu_memory[(addr+1) & 0xFFFF]<<8) | cpu_memory[addr];
}

void sim_mem_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr) {
    for (uint16_t i = 0; i < num_bytes; i++) {
        cpu_memory[(addr+i)&0xFFFF] = ptr[i];
    }
}

void sim_mem_clear(uint16_t addr, uint16_t num_bytes) {
    for (uint16_t i = 0; i < num_bytes; i++) {
        cpu_memory[(addr+i)&0xFFFF] = 0;
    }
}

#if defined(CHIP_Z80)
void sim_io_w8(uint16_t addr, uint8_t val) {
    cpu_io[addr] = val;
}

uint8_t sim_io_r8(uint16_t addr) {
    return cpu_io[addr];
}
#endif

uint16_t sim_get_addr(void) {
    assert(sim.valid);
    return cpu_readAddressBus(sim.cpu_state);
}

uint8_t sim_get_data(void) {
    assert(sim.valid);
    return cpu_readDataBus(sim.cpu_state);
}

uint32_t sim_get_cycle(void) {
    return cpu_cycle + 1;
}

void sim_set_cycle(uint32_t c) {
    cpu_cycle = c - 1;
}

bool sim_get_node_visual_state(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return cpu_read_node_state_as_bytes(sim.cpu_state, to_buffer.ptr, to_buffer.size);
}

bool sim_get_node_values(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return cpu_read_node_values(sim.cpu_state, to_buffer.ptr, to_buffer.size);
}

bool sim_get_transistor_on(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return cpu_read_transistor_on(sim.cpu_state, to_buffer.ptr, to_buffer.size);
}

bool sim_set_node_values(range_t from_buffer) {
    assert(sim.valid && from_buffer.ptr && (from_buffer.size > 0));
    return cpu_write_node_values(sim.cpu_state, from_buffer.ptr, from_buffer.size);
}

bool sim_set_transistor_on(range_t from_buffer) {
    assert(sim.valid && from_buffer.ptr && (from_buffer.size > 0));
    return cpu_write_transistor_on(sim.cpu_state, from_buffer.ptr, from_buffer.size);
}

bool sim_is_ignore_picking_highlight_node(int node_index) {
    // these nodes are all over the place, don't highlight them in picking
    #if defined(CHIP_6502)
    return (p6502_vcc == node_index) || (p6502_vss == node_index);
    #elif defined(CHIP_Z80)
    return (pz80_vcc == node_index) || (pz80_vss == node_index);
    #endif
}

uint16_t sim_get_pc(void) {
    return cpu_readPC(sim.cpu_state);
}

uint8_t sim_get_flags(void) {
    #if defined(CHIP_6502)
    return cpu_readP(sim.cpu_state);
    #elif defined(CHIP_Z80)
    return cpu_readF(sim.cpu_state);
    #else
    #error "Unknown CPU define!"
    #endif
}

int sim_get_num_nodes(void) {
    return (int)cpu_get_num_nodes();
}

bool sim_get_node(int node_index) {
    return cpu_read_node(sim.cpu_state, node_index);
}

void sim_set_node(int node_index, bool high) {
    cpu_write_node(sim.cpu_state, node_index, high);
}

#if defined(CHIP_6502)
uint8_t sim_6502_get_a(void) {
    return cpu_readA(sim.cpu_state);
}

uint8_t sim_6502_get_x(void) {
    return cpu_readX(sim.cpu_state);
}

uint8_t sim_6502_get_y(void) {
    return cpu_readY(sim.cpu_state);
}

uint8_t sim_6502_get_sp(void) {
    return cpu_readSP(sim.cpu_state);
}

uint8_t sim_6502_get_ir(void) {
    return cpu_readIR(sim.cpu_state);
}

uint8_t sim_6502_get_p(void) {
    return cpu_readP(sim.cpu_state);
}

bool sim_6502_get_clk0(void) {
    return cpu_readCLK0(sim.cpu_state);
}

bool sim_6502_get_rw(void) {
    return cpu_readRW(sim.cpu_state);
}

bool sim_6502_get_sync(void) {
    return cpu_readSYNC(sim.cpu_state);
}

void sim_6502_set_rdy(bool high) {
    cpu_writeRDY(sim.cpu_state, high);
}

bool sim_6502_get_rdy(void) {
    return cpu_readRDY(sim.cpu_state);
}

void sim_6502_set_irq(bool high) {
    cpu_writeIRQ(sim.cpu_state, high);
}

bool sim_6502_get_irq(void) {
    return cpu_readIRQ(sim.cpu_state);
}

void sim_6502_set_nmi(bool high) {
    cpu_writeNMI(sim.cpu_state, high);
}

bool sim_6502_get_nmi(void) {
    return cpu_readNMI(sim.cpu_state);
}

void sim_6502_set_res(bool high) {
    cpu_writeRES(sim.cpu_state, high);
}

bool sim_6502_get_res(void) {
    return cpu_readRES(sim.cpu_state);
}
#endif

#if defined(CHIP_Z80)
uint16_t sim_z80_get_af(void) {
    return (cpu_readA(sim.cpu_state) << 8) | cpu_readF(sim.cpu_state);
}

uint16_t sim_z80_get_bc(void) {
    return (cpu_readB(sim.cpu_state) << 8) | cpu_readC(sim.cpu_state);
}

uint16_t sim_z80_get_de(void) {
    return (cpu_readD(sim.cpu_state) << 8) | cpu_readE(sim.cpu_state);
}

uint16_t sim_z80_get_hl(void) {
    return (cpu_readH(sim.cpu_state) << 8) | cpu_readL(sim.cpu_state);
}

uint16_t sim_z80_get_af2(void) {
    return (cpu_readA2(sim.cpu_state) << 8) | cpu_readF2(sim.cpu_state);
}

uint16_t sim_z80_get_bc2(void) {
    return (cpu_readB2(sim.cpu_state) << 8) | cpu_readC2(sim.cpu_state);
}

uint16_t sim_z80_get_de2(void) {
    return (cpu_readD2(sim.cpu_state) << 8) | cpu_readE2(sim.cpu_state);
}

uint16_t sim_z80_get_hl2(void) {
    return (cpu_readH2(sim.cpu_state) << 8) | cpu_readL2(sim.cpu_state);
}

uint16_t sim_z80_get_ix(void) {
    return cpu_readIX(sim.cpu_state);
}

uint16_t sim_z80_get_iy(void) {
    return cpu_readIY(sim.cpu_state);
}

uint16_t sim_z80_get_sp(void) {
    return cpu_readSP(sim.cpu_state);
}

uint16_t sim_z80_get_pc(void) {
    return cpu_readPC(sim.cpu_state);
}

uint16_t sim_z80_get_wz(void) {
    return (cpu_readW(sim.cpu_state) << 8) | cpu_readZ(sim.cpu_state);
}

uint8_t sim_z80_get_f(void) {
    return cpu_readF(sim.cpu_state);
}

uint8_t sim_z80_get_i(void) {
    return cpu_readI(sim.cpu_state);
}

uint8_t sim_z80_get_ir(void) {
    return cpu_readIR(sim.cpu_state);
}

uint8_t sim_z80_get_r(void) {
    return cpu_readR(sim.cpu_state);
}

bool sim_z80_get_m1(void) {
    return cpu_readM1(sim.cpu_state);
}

bool sim_z80_get_clk(void) {
    return cpu_readCLK(sim.cpu_state);
}

bool sim_z80_get_mreq(void) {
    return cpu_readMREQ(sim.cpu_state);
}

bool sim_z80_get_iorq(void) {
    return cpu_readIORQ(sim.cpu_state);
}

bool sim_z80_get_rd(void) {
    return cpu_readRD(sim.cpu_state);
}

bool sim_z80_get_wr(void) {
    return cpu_readWR(sim.cpu_state);
}

void sim_z80_set_intvec(uint8_t val) {
    cpu_setIntVec(val);
}

uint8_t sim_z80_get_intvec(void) {
    return cpu_getIntVec();
}

void sim_z80_set_wait(bool high) {
    cpu_writeWAIT(sim.cpu_state, high);
}

void sim_z80_set_int(bool high) {
    cpu_writeINT(sim.cpu_state, high);
}

void sim_z80_set_nmi(bool high) {
    cpu_writeNMI(sim.cpu_state, high);
}

void sim_z80_set_reset(bool high) {
    cpu_writeRESET(sim.cpu_state, high);
}

void sim_z80_set_busrq(bool high) {
    cpu_writeBUSRQ(sim.cpu_state, high);
}

bool sim_z80_get_halt(void) {
    return cpu_readHALT(sim.cpu_state);
}

bool sim_z80_get_wait(void) {
    return cpu_readWAIT(sim.cpu_state);
}

bool sim_z80_get_rfsh(void) {
    return cpu_readRFSH(sim.cpu_state);
}

bool sim_z80_get_int(void) {
    return cpu_readINT(sim.cpu_state);
}

bool sim_z80_get_nmi(void) {
    return cpu_readNMI(sim.cpu_state);
}

bool sim_z80_get_reset(void) {
    return cpu_readRESET(sim.cpu_state);
}

bool sim_z80_get_busrq(void) {
    return cpu_readBUSRQ(sim.cpu_state);
}

bool sim_z80_get_busak(void) {
    return cpu_readBUSAK(sim.cpu_state);
}

uint8_t sim_z80_get_m(void) {
    return cpu_readM(sim.cpu_state);
}

uint8_t sim_z80_get_t(void) {
    return cpu_readT(sim.cpu_state);
}
#endif
