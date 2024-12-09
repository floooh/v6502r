//------------------------------------------------------------------------------
//  sim.c
//
//  Wrapper functions around perfect6502.
//------------------------------------------------------------------------------
#if defined(CHIP_6502)
#include "perfect6502.h"
#elif defined(CHIP_Z80)
#include "perfectz80.h"
#elif defined(CHIP_2A03)
#include "perfect2a03.h"
#endif
#include "nodenames.h"
#include "nodegroups.h"
#include "sim.h"
#include "trace.h"
#include "gfx.h"
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
    #if defined(CHIP_6502) || defined(CHIP_2A03)
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
        } while (!(prev_m1 && !cur_m1) && cpu_readWAIT(sim.cpu_state));   // M1 pin is active-low!
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
    #elif defined(CHIP_2A03)
    return (p2a03_vcc == node_index) || (p2a03_vss == node_index);
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
    #elif defined(CHIP_2A03)
    return cpu_readP(sim.cpu_state);
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

uint8_t sim_6502_get_op(void) {
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

uint8_t sim_z80_get_op(void) {
    return cpu_readIR(sim.cpu_state);
}

// see https://github.com/floooh/v6502r/issues/3
uint8_t sim_z80_get_im(void) {
    uint8_t im = (cpu_read_node(sim.cpu_state, 205) ? 1:0) | (cpu_read_node(sim.cpu_state, 179) ? 2:0);
    if (im > 0) {
        im -= 1;
    }
    return im;
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

bool sim_z80_get_iff1(void) {
    // NOTE: the Z80 netlist data from visual6502.org doesn't identify
    // the IFF1 flag as named node, so this is just my guess after
    // looking at the diff-view on executing EI vs DI!
    return cpu_read_node(sim.cpu_state, 231);
}

uint8_t sim_z80_get_m(void) {
    return cpu_readM(sim.cpu_state);
}

uint8_t sim_z80_get_t(void) {
    return cpu_readT(sim.cpu_state);
}
#endif

#if defined(CHIP_2A03)
uint8_t sim_2a03_get_a(void) {
    return cpu_readA(sim.cpu_state);
}

uint8_t sim_2a03_get_x(void) {
    return cpu_readX(sim.cpu_state);
}

uint8_t sim_2a03_get_y(void) {
    return cpu_readY(sim.cpu_state);
}

uint8_t sim_2a03_get_sp(void) {
    return cpu_readSP(sim.cpu_state);
}

uint8_t sim_2a03_get_op(void) {
    return cpu_readIR(sim.cpu_state);
}

uint8_t sim_2a03_get_p(void) {
    return cpu_readP(sim.cpu_state);
}

bool sim_2a03_get_clk0(void) {
    return cpu_readCLK0(sim.cpu_state);
}

bool sim_2a03_get_rw(void) {
    return cpu_readRW(sim.cpu_state);
}

bool sim_2a03_get_sync(void) {
    return cpu_readSYNC(sim.cpu_state);
}

void sim_2a03_set_rdy(bool high) {
    cpu_writeRDY(sim.cpu_state, high);
}

bool sim_2a03_get_rdy(void) {
    return cpu_readRDY(sim.cpu_state);
}

void sim_2a03_set_irq(bool high) {
    cpu_writeIRQ(sim.cpu_state, high);
}

bool sim_2a03_get_irq(void) {
    return cpu_readIRQ(sim.cpu_state);
}

void sim_2a03_set_nmi(bool high) {
    cpu_writeNMI(sim.cpu_state, high);
}

bool sim_2a03_get_nmi(void) {
    return cpu_readNMI(sim.cpu_state);
}

void sim_2a03_set_res(bool high) {
    cpu_writeRES(sim.cpu_state, high);
}

bool sim_2a03_get_res(void) {
    return cpu_readRES(sim.cpu_state);
}

uint16_t sim_2a03_get_frm_t(void) {
  return cpu_read_nodes(
      sim.cpu_state, 15,
      (uint16_t[]){p2a03_frm_t0, p2a03_frm_t1, p2a03_frm_t2, p2a03_frm_t3,
                   p2a03_frm_t4, p2a03_frm_t5, p2a03_frm_t6, p2a03_frm_t7,
                   p2a03_frm_t8, p2a03_frm_t9, p2a03_frm_t10, p2a03_frm_t11,
                   p2a03_frm_t12, p2a03_frm_t13, p2a03_frm_t14});
}

uint8_t sim_2a03_get_sq0_out(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq0_out0, p2a03_sq0_out1, p2a03_sq0_out2, p2a03_sq0_out3 });
}

uint8_t sim_2a03_get_sq1_out(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq1_out0, p2a03_sq1_out1, p2a03_sq1_out2, p2a03_sq1_out3 });
}

uint8_t sim_2a03_get_tri_out(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_tri_out0, p2a03_tri_out1, p2a03_tri_out2, p2a03_tri_out3 });
}

uint8_t sim_2a03_get_noi_out(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_noi_out0, p2a03_noi_out1, p2a03_noi_out2, p2a03_noi_out3 });
}

uint8_t sim_2a03_get_pcm_out(void) {
    return cpu_read_nodes(sim.cpu_state, 7, (uint16_t[]){ p2a03_pcm_out0, p2a03_pcm_out1, p2a03_pcm_out2, p2a03_pcm_out3, p2a03_pcm_out4, p2a03_pcm_out5, p2a03_pcm_out6 });
}

uint16_t sim_2a03_get_sq0_p(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_sq0_p0, p2a03_sq0_p1, p2a03_sq0_p2, p2a03_sq0_p3, p2a03_sq0_p4, p2a03_sq0_p5, p2a03_sq0_p6, p2a03_sq0_p7, p2a03_sq0_p8, p2a03_sq0_p9, p2a03_sq0_p10 });
}

uint16_t sim_2a03_get_sq0_t(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_sq0_t0, p2a03_sq0_t1, p2a03_sq0_t2, p2a03_sq0_t3, p2a03_sq0_t4, p2a03_sq0_t5, p2a03_sq0_t6, p2a03_sq0_t7, p2a03_sq0_t8, p2a03_sq0_t9, p2a03_sq0_t10 });
}

uint8_t sim_2a03_get_sq0_c(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq0_c0, p2a03_sq0_c1, p2a03_sq0_c2 });
}

uint8_t sim_2a03_get_sq0_swpb(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq0_swpb0, p2a03_sq0_swpb1, p2a03_sq0_swpb2 });
}

bool sim_2a03_get_sq0_swpdir(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_swpdir);
}

uint8_t sim_2a03_get_sq0_swpp(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq0_swpp0, p2a03_sq0_swpp1, p2a03_sq0_swpp2 });
}

bool sim_2a03_get_sq0_swpen(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_swpen);
}

uint8_t sim_2a03_get_sq0_swpt(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq0_swpt0, p2a03_sq0_swpt1, p2a03_sq0_swpt2 });
}

uint8_t sim_2a03_get_sq0_envp(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq0_envp0, p2a03_sq0_envp1, p2a03_sq0_envp2, p2a03_sq0_envp3 });
}

bool sim_2a03_get_sq0_envmode(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_envmode);
}

bool sim_2a03_get_sq0_lenhalt(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_lenhalt);
}

uint8_t sim_2a03_get_sq0_duty(void) {
    return cpu_read_nodes(sim.cpu_state, 2, (uint16_t[]){ p2a03_sq0_duty0, p2a03_sq0_duty1 });
}

uint8_t sim_2a03_get_sq0_envt(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq0_envt0, p2a03_sq0_envt1, p2a03_sq0_envt2, p2a03_sq0_envt3 });
}

uint8_t sim_2a03_get_sq0_envc(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq0_envc0, p2a03_sq0_envc1, p2a03_sq0_envc2, p2a03_sq0_envc3 });
}

uint8_t sim_2a03_get_sq0_len(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_sq0_len0, p2a03_sq0_len1, p2a03_sq0_len2, p2a03_sq0_len3, p2a03_sq0_len4, p2a03_sq0_len5, p2a03_sq0_len6, p2a03_sq0_len7 });
}

bool sim_2a03_get_sq0_en(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_en);
}

bool sim_2a03_get_sq0_on(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_on);
}

bool sim_2a03_get_sq0_len_reload(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_len_reload);
}

bool sim_2a03_get_sq0_silence(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq0_silence);
}

uint16_t sim_2a03_get_sq1_p(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_sq1_p0, p2a03_sq1_p1, p2a03_sq1_p2, p2a03_sq1_p3, p2a03_sq1_p4, p2a03_sq1_p5, p2a03_sq1_p6, p2a03_sq1_p7, p2a03_sq1_p8, p2a03_sq1_p9, p2a03_sq1_p10 });
}

uint16_t sim_2a03_get_sq1_t(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_sq1_t0, p2a03_sq1_t1, p2a03_sq1_t2, p2a03_sq1_t3, p2a03_sq1_t4, p2a03_sq1_t5, p2a03_sq1_t6, p2a03_sq1_t7, p2a03_sq1_t8, p2a03_sq1_t9, p2a03_sq1_t10 });
}

uint8_t sim_2a03_get_sq1_c(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq1_c0, p2a03_sq1_c1, p2a03_sq1_c2 });
}

uint8_t sim_2a03_get_sq1_swpb(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq1_swpb0, p2a03_sq1_swpb1, p2a03_sq1_swpb2 });
}

bool sim_2a03_get_sq1_swpdir(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_swpdir);
}

uint8_t sim_2a03_get_sq1_swpp(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq1_swpp0, p2a03_sq1_swpp1, p2a03_sq1_swpp2 });
}

bool sim_2a03_get_sq1_swpen(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_swpen);
}

uint8_t sim_2a03_get_sq1_swpt(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_sq1_swpt0, p2a03_sq1_swpt1, p2a03_sq1_swpt2 });
}

uint8_t sim_2a03_get_sq1_envp(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq1_envp0, p2a03_sq1_envp1, p2a03_sq1_envp2, p2a03_sq1_envp3 });
}

bool sim_2a03_get_sq1_envmode(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_envmode);
}

bool sim_2a03_get_sq1_lenhalt(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_lenhalt);
}

uint8_t sim_2a03_get_sq1_duty(void) {
    return cpu_read_nodes(sim.cpu_state, 2, (uint16_t[]){ p2a03_sq1_duty0, p2a03_sq1_duty1 });
}

uint8_t sim_2a03_get_sq1_envt(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq1_envt0, p2a03_sq1_envt1, p2a03_sq1_envt2, p2a03_sq1_envt3 });
}

uint8_t sim_2a03_get_sq1_envc(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_sq1_envc0, p2a03_sq1_envc1, p2a03_sq1_envc2, p2a03_sq1_envc3 });
}

uint8_t sim_2a03_get_sq1_len(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_sq1_len0, p2a03_sq1_len1, p2a03_sq1_len2, p2a03_sq1_len3, p2a03_sq1_len4, p2a03_sq1_len5, p2a03_sq1_len6, p2a03_sq1_len7 });
}

bool sim_2a03_get_sq1_en(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_en);
}

bool sim_2a03_get_sq1_on(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_on);
}

bool sim_2a03_get_sq1_len_reload(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_len_reload);
}

bool sim_2a03_get_sq1_silence(void) {
    return cpu_read_node(sim.cpu_state, p2a03_sq1_silence);
}

uint8_t sim_2a03_get_tri_lin(void) {
    return cpu_read_nodes(sim.cpu_state, 7, (uint16_t[]){ p2a03_tri_lin0, p2a03_tri_lin1, p2a03_tri_lin2, p2a03_tri_lin3, p2a03_tri_lin4, p2a03_tri_lin5, p2a03_tri_lin6 });
}

bool sim_2a03_get_tri_lin_en(void) {
    return cpu_read_node(sim.cpu_state, p2a03_tri_lin_en);
}

uint8_t sim_2a03_get_tri_lc(void) {
    return cpu_read_nodes(sim.cpu_state, 7, (uint16_t[]){ p2a03_tri_lc0, p2a03_tri_lc1, p2a03_tri_lc2, p2a03_tri_lc3, p2a03_tri_lc4, p2a03_tri_lc5, p2a03_tri_lc6 });
}

uint16_t sim_2a03_get_tri_p(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_tri_p0, p2a03_tri_p1, p2a03_tri_p2, p2a03_tri_p3, p2a03_tri_p4, p2a03_tri_p5, p2a03_tri_p6, p2a03_tri_p7, p2a03_tri_p8, p2a03_tri_p9, p2a03_tri_p10 });
}

uint16_t sim_2a03_get_tri_t(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_tri_t0, p2a03_tri_t1, p2a03_tri_t2, p2a03_tri_t3, p2a03_tri_t4, p2a03_tri_t5, p2a03_tri_t6, p2a03_tri_t7, p2a03_tri_t8, p2a03_tri_t9, p2a03_tri_t10 });
}

uint8_t sim_2a03_get_tri_c(void) {
    return cpu_read_nodes(sim.cpu_state, 5, (uint16_t[]){ p2a03_tri_c0, p2a03_tri_c1, p2a03_tri_c2, p2a03_tri_c3, p2a03_tri_c4 });
}

uint8_t sim_2a03_get_tri_len(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_tri_len0, p2a03_tri_len1, p2a03_tri_len2, p2a03_tri_len3, p2a03_tri_len4, p2a03_tri_len5, p2a03_tri_len6, p2a03_tri_len7 });
}

bool sim_2a03_get_tri_en(void) {
    return cpu_read_node(sim.cpu_state, p2a03_tri_en);
}

bool sim_2a03_get_tri_on(void) {
    return cpu_read_node(sim.cpu_state, p2a03_tri_on);
}

bool sim_2a03_get_tri_len_reload(void) {
    return cpu_read_node(sim.cpu_state, p2a03_tri_len_reload);
}

bool sim_2a03_get_tri_silence(void) {
    return cpu_read_node(sim.cpu_state, p2a03_tri_silence);
}

uint8_t sim_2a03_get_noi_freq(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_noi_freq0, p2a03_noi_freq1, p2a03_noi_freq2, p2a03_noi_freq3 });
}

bool sim_2a03_get_noi_lfsrmode(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_lfsrmode);
}

uint16_t sim_2a03_get_noi_t(void) {
    return cpu_read_nodes(sim.cpu_state, 11, (uint16_t[]){ p2a03_noi_t0, p2a03_noi_t1, p2a03_noi_t2, p2a03_noi_t3, p2a03_noi_t4, p2a03_noi_t5, p2a03_noi_t6, p2a03_noi_t7, p2a03_noi_t8, p2a03_noi_t9, p2a03_noi_t10 });
}

uint16_t sim_2a03_get_noi_c(void) {
  return cpu_read_nodes(
      sim.cpu_state, 15,
      (uint16_t[]){p2a03_noi_c0, p2a03_noi_c1, p2a03_noi_c2, p2a03_noi_c3,
                   p2a03_noi_c4, p2a03_noi_c5, p2a03_noi_c6, p2a03_noi_c7,
                   p2a03_noi_c8, p2a03_noi_c9, p2a03_noi_c10, p2a03_noi_c11,
                   p2a03_noi_c12, p2a03_noi_c13, p2a03_noi_c14});
}

uint8_t sim_2a03_get_noi_envp(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_noi_envp0, p2a03_noi_envp1, p2a03_noi_envp2, p2a03_noi_envp3 });
}

bool sim_2a03_get_noi_envmode(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_envmode);
}

bool sim_2a03_get_noi_lenhalt(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_lenhalt);
}

uint8_t sim_2a03_get_noi_envt(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_noi_envt0, p2a03_noi_envt1, p2a03_noi_envt2, p2a03_noi_envt3 });
}

uint8_t sim_2a03_get_noi_envc(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_noi_envc0, p2a03_noi_envc1, p2a03_noi_envc2, p2a03_noi_envc3 });
}

uint8_t sim_2a03_get_noi_len(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_noi_len0, p2a03_noi_len1, p2a03_noi_len2, p2a03_noi_len3, p2a03_noi_len4, p2a03_noi_len5, p2a03_noi_len6, p2a03_noi_len7 });
}

bool sim_2a03_get_noi_en(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_en);
}

bool sim_2a03_get_noi_on(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_on);
}

bool sim_2a03_get_noi_len_reload(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_len_reload);
}

bool sim_2a03_get_noi_silence(void) {
    return cpu_read_node(sim.cpu_state, p2a03_noi_silence);
}

uint8_t sim_2a03_get_pcm_bits(void) {
    return cpu_read_nodes(sim.cpu_state, 3, (uint16_t[]){ p2a03_pcm_bits0, p2a03_pcm_bits1, p2a03_pcm_bits2 });
}

bool sim_2a03_get_pcm_bits_overflow(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_bits_overflow);
}

uint8_t sim_2a03_get_pcm_buf(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_pcm_buf0, p2a03_pcm_buf1, p2a03_pcm_buf2, p2a03_pcm_buf3, p2a03_pcm_buf4, p2a03_pcm_buf5, p2a03_pcm_buf6, p2a03_pcm_buf7 });
}

bool sim_2a03_get_pcm_loadbuf(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_loadbuf);
}

uint8_t sim_2a03_get_pcm_sr(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_pcm_sr0, p2a03_pcm_sr1, p2a03_pcm_sr2, p2a03_pcm_sr3, p2a03_pcm_sr4, p2a03_pcm_sr5, p2a03_pcm_sr6, p2a03_pcm_sr7 });
}

bool sim_2a03_get_pcm_loadsr(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_loadsr);
}

bool sim_2a03_get_pcm_shiftsr(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_shiftsr);
}

uint8_t sim_2a03_get_pcm_freq(void) {
    return cpu_read_nodes(sim.cpu_state, 4, (uint16_t[]){ p2a03_pcm_freq0, p2a03_pcm_freq1, p2a03_pcm_freq2, p2a03_pcm_freq3 });
}

bool sim_2a03_get_pcm_loop(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_loop);
}

bool sim_2a03_get_pcm_irqen(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_irqen);
}

uint16_t sim_2a03_get_pcm_t(void) {
    return cpu_read_nodes(sim.cpu_state, 9, (uint16_t[]){ p2a03_pcm_t0, p2a03_pcm_t1, p2a03_pcm_t2, p2a03_pcm_t3, p2a03_pcm_t4, p2a03_pcm_t5, p2a03_pcm_t6, p2a03_pcm_t7, p2a03_pcm_t8 });
}

uint8_t sim_2a03_get_pcm_sa(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_pcm_sa0, p2a03_pcm_sa1, p2a03_pcm_sa2, p2a03_pcm_sa3, p2a03_pcm_sa4, p2a03_pcm_sa5, p2a03_pcm_sa6, p2a03_pcm_sa7 });
}

uint16_t sim_2a03_get_pcm_a(void) {
  return cpu_read_nodes(
      sim.cpu_state, 15,
      (uint16_t[]){p2a03_pcm_a0, p2a03_pcm_a1, p2a03_pcm_a2, p2a03_pcm_a3,
                   p2a03_pcm_a4, p2a03_pcm_a5, p2a03_pcm_a6, p2a03_pcm_a7,
                   p2a03_pcm_a8, p2a03_pcm_a9, p2a03_pcm_a10, p2a03_pcm_a11,
                   p2a03_pcm_a12, p2a03_pcm_a13, p2a03_pcm_a14});
}

uint8_t sim_2a03_get_pcm_l(void) {
    return cpu_read_nodes(sim.cpu_state, 8, (uint16_t[]){ p2a03_pcm_l0, p2a03_pcm_l1, p2a03_pcm_l2, p2a03_pcm_l3, p2a03_pcm_l4, p2a03_pcm_l5, p2a03_pcm_l6, p2a03_pcm_l7 });
}

uint16_t sim_2a03_get_pcm_lc(void) {
  return cpu_read_nodes(
      sim.cpu_state, 12,
      (uint16_t[]){p2a03_pcm_lc0, p2a03_pcm_lc1, p2a03_pcm_lc2, p2a03_pcm_lc3,
                   p2a03_pcm_lc4, p2a03_pcm_lc5, p2a03_pcm_lc6, p2a03_pcm_lc7,
                   p2a03_pcm_lc8, p2a03_pcm_lc9, p2a03_pcm_lc10,
                   p2a03_pcm_lc11});
}

bool sim_2a03_get_pcm_en(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_en);
}

bool sim_2a03_get_pcm_rd_active(void) {
    return cpu_read_node(sim.cpu_state, p2a03_pcm_rd_active);
}

#endif

static void sim_add_nodegroup(const char* name, range_t nodegroup) {
    assert(name);
    assert(sim.num_nodegroups < MAX_NODE_GROUPS);
    sim.nodegroups[sim.num_nodegroups++] = (sim_nodegroup_t){
        .group_name = name,
        .num_nodes = (int)(nodegroup.size / sizeof(uint32_t)),
        .nodes = nodegroup.ptr
    };
}

static void sim_init_nodegroups(void) {
    // setup node groups
    #if defined(CHIP_6502)
    sim_add_nodegroup("Addr", RANGE(nodegroup_ab));
    sim_add_nodegroup("Data", RANGE(nodegroup_db));
    sim_add_nodegroup("IR", RANGE(nodegroup_ir));
    sim_add_nodegroup("A", RANGE(nodegroup_a));
    sim_add_nodegroup("X", RANGE(nodegroup_x));
    sim_add_nodegroup("Y", RANGE(nodegroup_y));
    sim_add_nodegroup("S", RANGE(nodegroup_sp));
    sim_add_nodegroup("PCH", RANGE(nodegroup_pch));
    sim_add_nodegroup("PCL", RANGE(nodegroup_pcl));
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
    sim_add_nodegroup("MCycle", RANGE(nodegroup_m));
    sim_add_nodegroup("TState", RANGE(nodegroup_t));
    #elif defined(CHIP_2A03)
    sim_add_nodegroup("Addr", RANGE(nodegroup_ab));
    sim_add_nodegroup("Data", RANGE(nodegroup_db));
    sim_add_nodegroup("IR", RANGE(nodegroup_ir));
    sim_add_nodegroup("A", RANGE(nodegroup_a));
    sim_add_nodegroup("X", RANGE(nodegroup_x));
    sim_add_nodegroup("Y", RANGE(nodegroup_y));
    sim_add_nodegroup("S", RANGE(nodegroup_sp));
    sim_add_nodegroup("PCH", RANGE(nodegroup_pch));
    sim_add_nodegroup("PCL", RANGE(nodegroup_pcl));
    sim_add_nodegroup("Sq0", RANGE(nodegroup_sq0));
    sim_add_nodegroup("Sq1", RANGE(nodegroup_sq1));
    sim_add_nodegroup("Tri", RANGE(nodegroup_tri));
    sim_add_nodegroup("Noi", RANGE(nodegroup_noi));
    sim_add_nodegroup("Pcm", RANGE(nodegroup_pcm));
    #endif
}

sim_nodegroup_range_t sim_get_nodegroups(void) {
    return (sim_nodegroup_range_t) {
        .num = sim.num_nodegroups,
        .ptr = sim.nodegroups
    };
}
