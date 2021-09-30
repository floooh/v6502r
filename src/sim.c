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
#include "nodegroups.h"
#include "sim.h"
#include "trace.h"
#include <stdlib.h> // qsort, bsearch, strtol
#include <ctype.h>  // isdigit

#define MAX_NODE_GROUPS (64)
#define RANGE(arr) ((range_t){.ptr=arr,.size=sizeof(arr)})

static struct {
    bool valid;
    bool paused;
    void* cpu_state;
    int num_sorted_nodes;
    sim_named_node_t sorted_nodes[MAX_NODES];
    int num_nodegroups;
    sim_nodegroup_t nodegroups[MAX_NODE_GROUPS];
} sim;

static void sim_init_nodegroups(void);

static int qsort_cb(const void* a, const void* b) {
    const sim_named_node_t* n0 = a;
    const sim_named_node_t* n1 = b;
    return strcmp(n0->node_name, n1->node_name);
}

static int bsearch_cb(const void* key, const void* elem) {
    const sim_named_node_t* n = elem;
    return strcmp((const char*)key, n->node_name);
}

void sim_init(void) {
    assert(!sim.valid);
    assert(0 == sim.cpu_state);
    sim.cpu_state = cpu_initAndResetChip();
    assert(sim.cpu_state);
    sim.paused = true;
    sim.valid = true;
    sim_init_nodegroups();

    // create a sorted node array for fast lookup by node name
    assert(num_node_names < MAX_NODES);
    for (int i = 0; i < num_node_names; i++) {
        if (node_names[i][0]) {
            sim_named_node_t* n = &sim.sorted_nodes[sim.num_sorted_nodes++];
            n->node_name = node_names[i];
            n->node_index = i;
        }
    }
    qsort(sim.sorted_nodes, (size_t)sim.num_sorted_nodes, sizeof(sim_named_node_t), qsort_cb);
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
    // special case 'number'
    if (isdigit(name[0])) {
        int node_index = strtol(name, 0, 10);
        if ((node_index >= 1) && (node_index < sim_get_num_nodes())) {
            return node_index;
        }
    }
    else {
        const sim_named_node_t* n = bsearch(name, sim.sorted_nodes, (size_t)sim.num_sorted_nodes, sizeof(sim_named_node_t), bsearch_cb);
        if (n) {
            return n->node_index;
        }
    }
    // fallthrough: not found or invalid '#' number
    return -1;
}

// returns empty string for unnamed nodes
const char* sim_get_node_name_by_index(int node_index) {
    assert((node_index >= 0) && (node_index < MAX_NODES));
    if (node_index < num_node_names) {
        return node_names[node_index];
    }
    else {
        return "";
    }
}

sim_named_node_range_t sim_get_sorted_nodes(void) {
    return (sim_named_node_range_t){
        .ptr = sim.sorted_nodes,
        .num = sim.num_sorted_nodes
    };
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

bool sim_write_node_visual_state(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return cpu_read_node_state_as_bytes(sim.cpu_state, gfx_visual_node_active, gfx_visual_node_inactive, to_buffer.ptr, to_buffer.size);
}

bool sim_write_node_values(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return cpu_read_node_values(sim.cpu_state, to_buffer.ptr, to_buffer.size);
}

bool sim_write_transistor_on(range_t to_buffer) {
    assert(sim.valid && to_buffer.ptr && (to_buffer.size > 0));
    return cpu_read_transistor_on(sim.cpu_state, to_buffer.ptr, to_buffer.size);
}

bool sim_read_node_values(range_t from_buffer) {
    assert(sim.valid && from_buffer.ptr && (from_buffer.size > 0));
    return cpu_write_node_values(sim.cpu_state, from_buffer.ptr, from_buffer.size);
}

bool sim_read_transistor_on(range_t from_buffer) {
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

bool sim_get_node_state(int node_index) {
    return cpu_read_node(sim.cpu_state, node_index);
}

void sim_set_node_state(int node_index, bool high) {
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

static void sim_add_nodegroup(const char* name, range_t nodegroup) {
    assert(name);
    assert(sim.num_nodegroups < MAX_NODE_GROUPS);
    sim.nodegroups[sim.num_nodegroups++] = (sim_nodegroup_t){
        .group_name = name,
        .num_nodes = nodegroup.size / sizeof(uint32_t),
        .nodes = nodegroup.ptr
    };
}

static void sim_init_nodegroups(void) {
    // setup node groups
    #if defined(CHIP_6502)

    #elif defined(CHIP_Z80)
    sim_add_nodegroup("Addr", RANGE(nodegroup_ab));
    sim_add_nodegroup("Data", RANGE(nodegroup_db));
    sim_add_nodegroup("IR", RANGE(nodegroup_ir));
    sim_add_nodegroup("A", RANGE(nodegroup_reg_a));
    sim_add_nodegroup("F", RANGE(nodegroup_reg_f));
    sim_add_nodegroup("B", RANGE(nodegroup_reg_b));
    sim_add_nodegroup("C", RANGE(nodegroup_reg_c));
    sim_add_nodegroup("D", RANGE(nodegroup_reg_d));
    sim_add_nodegroup("E", RANGE(nodegroup_reg_e));
    sim_add_nodegroup("H", RANGE(nodegroup_reg_h));
    sim_add_nodegroup("L", RANGE(nodegroup_reg_l));
    sim_add_nodegroup("A'", RANGE(nodegroup_reg_aa));
    sim_add_nodegroup("F'", RANGE(nodegroup_reg_ff));
    sim_add_nodegroup("B'", RANGE(nodegroup_reg_bb));
    sim_add_nodegroup("C'", RANGE(nodegroup_reg_cc));
    sim_add_nodegroup("D'", RANGE(nodegroup_reg_dd));
    sim_add_nodegroup("E'", RANGE(nodegroup_reg_ee));
    sim_add_nodegroup("H'", RANGE(nodegroup_reg_hh));
    sim_add_nodegroup("L'", RANGE(nodegroup_reg_ll));
    sim_add_nodegroup("IXH", RANGE(nodegroup_reg_ixh));
    sim_add_nodegroup("IXL", RANGE(nodegroup_reg_ixl));
    sim_add_nodegroup("IYH", RANGE(nodegroup_reg_iyh));
    sim_add_nodegroup("IYL", RANGE(nodegroup_reg_iyl));
    sim_add_nodegroup("SPH", RANGE(nodegroup_reg_sph));
    sim_add_nodegroup("SPL", RANGE(nodegroup_reg_spl));
    sim_add_nodegroup("WZH", RANGE(nodegroup_reg_w));
    sim_add_nodegroup("WZL", RANGE(nodegroup_reg_z));
    sim_add_nodegroup("RegBits", RANGE(nodegroup_regbits));
    sim_add_nodegroup("PCH", RANGE(nodegroup_pch));
    sim_add_nodegroup("PCL", RANGE(nodegroup_pcl));
    sim_add_nodegroup("PCBits", RANGE(nodegroup_pcbits));
    sim_add_nodegroup("UBus", RANGE(nodegroup_ubus));
    sim_add_nodegroup("VBus", RANGE(nodegroup_vbus));
    sim_add_nodegroup("ALU_A", RANGE(nodegroup_alua));
    sim_add_nodegroup("ALU_B", RANGE(nodegroup_alub));
    sim_add_nodegroup("ALU_Bus", RANGE(nodegroup_alubus));
    sim_add_nodegroup("ALU_Lat", RANGE(nodegroup_alulat));
    sim_add_nodegroup("ALU_Out", RANGE(nodegroup_aluout));
    sim_add_nodegroup("DBus", RANGE(nodegroup_dbus));
    sim_add_nodegroup("DLatch", RANGE(nodegroup_dlatch));
    #endif
}

sim_nodegroup_range_t sim_get_nodegroups(void) {
    return (sim_nodegroup_range_t) {
        .num = sim.num_nodegroups,
        .ptr = sim.nodegroups
    };
}
