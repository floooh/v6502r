//------------------------------------------------------------------------------
//  trace.c
//  Keep a history of chip state.
//------------------------------------------------------------------------------
#if defined(CHIP_6502)
#include "perfect6502.h"
#include "chips/m6502dasm.h"
#elif defined(CHIP_Z80)
#include "perfectz80.h"
#include "chips/z80dasm.h"
#elif defined(CHIP_2A03)
#include "perfect2a03.h"
#include "chips/m6502dasm.h"
#endif
#include "nodenames.h"
#include "nodegroups.h"
#include "trace.h"
#include "sim.h"
#include "gfx.h"

#define MAX_DASM_STRING_LENGTH (32)

typedef struct {
    uint16_t op_addr;
    bool op_prefixed;   // Z80 only
    char str[MAX_DASM_STRING_LENGTH];
} trace_item_dasm_t;

typedef struct {
    uint16_t op_addr;
    uint16_t cur_addr;      // incremented by disassembler input callback
    bool op_prefixed;       // Z80 only, true if current instruction is prefixed
    int out_str_index;
    char out_str[MAX_DASM_STRING_LENGTH];
} trace_dasm_t;

static struct {
    bool valid;
    uint32_t flip_bits;         // for visually separating instructions and cycles
    struct {
        bool scroll_to_end;
    } ui;
    trace_dasm_t dasm;
    // ring buffer as SoA for faster access and better packing
    uint32_t head;
    uint32_t tail;
    struct {
        uint32_t cycle[MAX_TRACE_ITEMS];
        uint32_t flip_bits[MAX_TRACE_ITEMS];
        uint32_t node_values[MAX_TRACE_ITEMS][256];
        uint32_t transistors_on[MAX_TRACE_ITEMS][512];
        trace_item_dasm_t dasm[MAX_TRACE_ITEMS];
        uint8_t mem[MAX_TRACE_ITEMS][4096];
    } items;
} trace;

void trace_init(void) {
    assert(!trace.valid);
    trace.valid = true;
}

void trace_shutdown(void) {
    assert(trace.valid);
    memset(&trace, 0, sizeof(trace));
}

void trace_clear(void) {
    assert(trace.valid);
    memset(&trace, 0, sizeof(trace));
    trace.valid = true;
}

// ring buffer helper functions
static uint32_t ring_idx(uint32_t i) {
    return (i % MAX_TRACE_ITEMS);
}

static bool ring_full(void) {
    return ring_idx(trace.head+1) == trace.tail;
}

static bool ring_empty(void) {
    return trace.head == trace.tail;
}

static uint32_t ring_count(void) {
    if (trace.head >= trace.tail) {
        return trace.head - trace.tail;
    }
    else {
        return (trace.head + MAX_TRACE_ITEMS) - trace.tail;
    }
}

static uint32_t ring_add(void) {
    uint32_t idx = trace.head;
    if (ring_full()) {
        trace.tail = ring_idx(trace.tail+1);
    }
    trace.head = ring_idx(trace.head+1);
    return idx;
}

static void ring_pop(void) {
    if (!ring_empty()) {
        trace.head = ring_idx(trace.head-1);
    }
}

int trace_num_items(void) {
    assert(trace.valid);
    return (int)ring_count();
}

bool trace_empty(void) {
    assert(trace.valid);
    return ring_empty();
}

static uint32_t trace_to_ring_index(uint32_t trace_index) {
    uint32_t idx = ring_idx(trace.head - 1 - trace_index);
    assert(idx != trace.head);
    return idx;
}

static int get_bitmap(const uint32_t* bm, uint32_t index) {
    return (bm[index>>5]>>(index&0x1F)) & 1;
}

static int get_node_value(uint32_t idx, uint32_t node_index) {
    return get_bitmap(&trace.items.node_values[idx][0], node_index);
};

static bool is_node_high(uint32_t trace_index, uint32_t node_index) {
    uint32_t idx = trace_to_ring_index(trace_index);
    return 0 != get_node_value(idx, node_index);
}

static uint32_t read_nodes(uint32_t trace_index, uint32_t count, uint32_t* node_indices) {
    uint32_t result = 0;
    for (int i = count - 1; i >= 0; i--) {
        result <<=  1;
        result |= is_node_high(trace_index, node_indices[i]);
    }
    return result;
}

bool trace_is_node_high(uint32_t trace_index, uint32_t node_index) {
    return is_node_high(trace_index, node_index);
}

uint32_t trace_get_cycle(uint32_t trace_index) {
    assert(trace.valid);
    uint32_t idx = trace_to_ring_index(trace_index);
    return trace.items.cycle[idx];
}

uint32_t trace_get_flipbits(uint32_t trace_index) {
    assert(trace.valid);
    uint32_t idx = trace_to_ring_index(trace_index);
    return trace.items.flip_bits[idx];
}

uint16_t trace_get_addr(uint32_t index) {
    return read_nodes(index, 16, nodegroup_ab);
}

uint8_t trace_get_data(uint32_t index) {
    return read_nodes(index, 8, nodegroup_db);
}

uint16_t trace_get_pc(uint32_t index) {
    uint8_t pcl = read_nodes(index, 8, nodegroup_pcl);
    uint8_t pch = read_nodes(index, 8, nodegroup_pch);
    return (pch<<8)|pcl;
}

uint8_t trace_get_flags(uint32_t index) {
    #if defined(CHIP_6502)
    return read_nodes(index, 8, nodegroup_p);
    #elif defined(CHIP_Z80)
    return read_nodes(index, 8, is_node_high(index, pz80_ex_af) ? nodegroup_reg_f : nodegroup_reg_ff);
    #elif defined(CHIP_2A03)
    return read_nodes(index, 8, nodegroup_p);
    #endif
}

const char* trace_get_disasm(uint32_t index) {
    uint32_t idx = trace_to_ring_index(index);
    return trace.items.dasm[idx].str;
}

#if defined(CHIP_6502)
uint8_t trace_6502_get_a(uint32_t index) {
    return read_nodes(index, 8, nodegroup_a);
}

uint8_t trace_6502_get_x(uint32_t index) {
    return read_nodes(index, 8, nodegroup_x);
}

uint8_t trace_6502_get_y(uint32_t index) {
    return read_nodes(index, 8, nodegroup_y);
}

uint8_t trace_6502_get_sp(uint32_t index) {
    return read_nodes(index, 8, nodegroup_sp);
}

uint8_t trace_6502_get_op(uint32_t index) {
    return ~read_nodes(index, 8, nodegroup_ir);
}

bool trace_6502_get_rw(uint32_t index) {
    return is_node_high(index, p6502_rw);
}

bool trace_6502_get_sync(uint32_t index) {
    return is_node_high(index, p6502_sync);
}

bool trace_6502_get_clk0(uint32_t index) {
    return is_node_high(index, p6502_clk0);
}

bool trace_6502_get_irq(uint32_t index) {
    return is_node_high(index, p6502_irq);
}

bool trace_6502_get_nmi(uint32_t index) {
    return is_node_high(index, p6502_nmi);
}

bool trace_6502_get_res(uint32_t index) {
    return is_node_high(index, p6502_res);
}

bool trace_6502_get_rdy(uint32_t index) {
    return is_node_high(index, p6502_rdy);
}
#endif

#if defined(CHIP_Z80)
bool trace_z80_get_m1(uint32_t index) {
    return is_node_high(index, pz80__m1);
}

bool trace_z80_get_clk(uint32_t index) {
    return is_node_high(index, pz80_clk);
}

bool trace_z80_get_mreq(uint32_t index) {
    return is_node_high(index, pz80__mreq);
}

bool trace_z80_get_ioreq(uint32_t index) {
    return is_node_high(index, pz80__iorq);
}

bool trace_z80_get_rfsh(uint32_t index) {
    return is_node_high(index, pz80__rfsh);
}

bool trace_z80_get_rd(uint32_t index) {
    return is_node_high(index, pz80__rd);
}

bool trace_z80_get_wr(uint32_t index) {
    return is_node_high(index, pz80__wr);
}

bool trace_z80_get_int(uint32_t index) {
    return is_node_high(index, pz80__int);
}

bool trace_z80_get_nmi(uint32_t index) {
    return is_node_high(index, pz80__nmi);
}

bool trace_z80_get_wait(uint32_t index) {
    return is_node_high(index, pz80__wait);
}

bool trace_z80_get_halt(uint32_t index) {
    return is_node_high(index, pz80__halt);
}

bool trace_z80_get_iff1(uint32_t index) {
    return is_node_high(index, 231);
}

uint8_t trace_z80_get_op(uint32_t index) {
    return read_nodes(index, 8, nodegroup_ir);
}

// see https://github.com/floooh/v6502r/issues/3
uint8_t trace_z80_get_im(uint32_t index) {
    uint8_t im = (is_node_high(index, 205) ? 1:0) | (is_node_high(index, 179) ? 2:0);
    if (im > 0) {
        im -= 1;
    }
    return im;
}

uint8_t trace_z80_get_a(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_af) ? nodegroup_reg_a : nodegroup_reg_aa);
}

uint8_t trace_z80_get_a2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_af) ? nodegroup_reg_a : nodegroup_reg_aa);
}

uint8_t trace_z80_get_f(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_af) ? nodegroup_reg_f : nodegroup_reg_ff);
}

uint8_t trace_z80_get_f2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_af) ? nodegroup_reg_f : nodegroup_reg_ff);
}

uint8_t trace_z80_get_b(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_bcdehl) ? nodegroup_reg_bb : nodegroup_reg_b);
}

uint8_t trace_z80_get_b2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_bcdehl) ? nodegroup_reg_bb : nodegroup_reg_b);
}

uint8_t trace_z80_get_c(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_bcdehl) ? nodegroup_reg_cc : nodegroup_reg_c);
}

uint8_t trace_z80_get_c2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_bcdehl) ? nodegroup_reg_cc : nodegroup_reg_c);
}

uint8_t trace_z80_get_d(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_hh : nodegroup_reg_dd);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_h : nodegroup_reg_d);
    }
}

uint8_t trace_z80_get_d2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_hh : nodegroup_reg_dd);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_h : nodegroup_reg_d);
    }
}

uint8_t trace_z80_get_e(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_ll : nodegroup_reg_ee);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_l : nodegroup_reg_e);
    }
}

uint8_t trace_z80_get_e2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_ll : nodegroup_reg_ee);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_l : nodegroup_reg_e);
    }
}

uint8_t trace_z80_get_h(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_dd : nodegroup_reg_hh);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_d : nodegroup_reg_h);
    }
}

uint8_t trace_z80_get_h2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_dd : nodegroup_reg_hh);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_d : nodegroup_reg_h);
    }
}

uint8_t trace_z80_get_l(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_ee : nodegroup_reg_ll);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_e : nodegroup_reg_l);
    }
}

uint8_t trace_z80_get_l2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodegroup_reg_ee : nodegroup_reg_ll);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodegroup_reg_e : nodegroup_reg_l);
    }
}

uint8_t trace_z80_get_i(uint32_t index) {
    return read_nodes(index, 8, nodegroup_reg_i);
}

uint8_t trace_z80_get_r(uint32_t index) {
    return read_nodes(index, 8, nodegroup_reg_r);
}

uint8_t trace_z80_get_w(uint32_t index) {
    return read_nodes(index, 8, nodegroup_reg_w);
}

uint8_t trace_z80_get_z(uint32_t index) {
    return read_nodes(index, 8, nodegroup_reg_z);
}

uint8_t trace_z80_get_mcycle(uint32_t index) {
    switch (read_nodes(index, 5, nodegroup_m)) {
        case (1<<0): return 1;
        case (1<<1): return 2;
        case (1<<2): return 3;
        case (1<<3): return 4;
        case (1<<4): return 5;
        default: return 0;
    }
}

uint8_t trace_z80_get_tstate(uint32_t index) {
    switch (read_nodes(index, 6, nodegroup_t)) {
        case (1<<0): return 1;
        case (1<<1): return 2;
        case (1<<2): return 3;
        case (1<<3): return 4;
        case (1<<4): return 5;
        case (1<<5): return 6;
        default: return 0;
    }
}

uint16_t trace_z80_get_af(uint32_t index) {
    return (trace_z80_get_a(index)<<8) | trace_z80_get_f(index);
}

uint16_t trace_z80_get_af2(uint32_t index) {
    return (trace_z80_get_a2(index)<<8) | trace_z80_get_f2(index);
}

uint16_t trace_z80_get_bc(uint32_t index) {
    return (trace_z80_get_b(index)<<8) | trace_z80_get_c(index);
}

uint16_t trace_z80_get_bc2(uint32_t index) {
    return (trace_z80_get_b2(index)<<8) | trace_z80_get_c2(index);
}

uint16_t trace_z80_get_de(uint32_t index) {
    return (trace_z80_get_d(index)<<8) | trace_z80_get_e(index);
}

uint16_t trace_z80_get_de2(uint32_t index) {
    return (trace_z80_get_d2(index)<<8) | trace_z80_get_e2(index);
}

uint16_t trace_z80_get_hl(uint32_t index) {
    return (trace_z80_get_h(index)<<8) | trace_z80_get_l(index);
}

uint16_t trace_z80_get_hl2(uint32_t index) {
    return (trace_z80_get_h2(index)<<8) | trace_z80_get_l2(index);
}

uint16_t trace_z80_get_ix(uint32_t index) {
    return (read_nodes(index, 8, nodegroup_reg_ixh) << 8) | read_nodes(index, 8, nodegroup_reg_ixl);
}

uint16_t trace_z80_get_iy(uint32_t index) {
    return (read_nodes(index, 8, nodegroup_reg_iyh) << 8) | read_nodes(index, 8, nodegroup_reg_iyl);
}

uint16_t trace_z80_get_sp(uint32_t index) {
    return (read_nodes(index, 8, nodegroup_reg_sph) << 8) | read_nodes(index, 8, nodegroup_reg_spl);
}

uint16_t trace_z80_get_wz(uint32_t index) {
    return (trace_z80_get_w(index)<<8) | trace_z80_get_z(index);
}
#endif

#if defined(CHIP_2A03)
uint8_t trace_2a03_get_a(uint32_t index) {
    return read_nodes(index, 8, nodegroup_a);
}

uint8_t trace_2a03_get_x(uint32_t index) {
    return read_nodes(index, 8, nodegroup_x);
}

uint8_t trace_2a03_get_y(uint32_t index) {
    return read_nodes(index, 8, nodegroup_y);
}

uint8_t trace_2a03_get_sp(uint32_t index) {
    return read_nodes(index, 8, nodegroup_sp);
}

uint8_t trace_2a03_get_op(uint32_t index) {
    return ~read_nodes(index, 8, nodegroup_ir);
}

bool trace_2a03_get_rw(uint32_t index) {
    return is_node_high(index, p2a03_rw);
}

bool trace_2a03_get_sync(uint32_t index) {
    return is_node_high(index, p2a03_sync);
}

bool trace_2a03_get_clk0(uint32_t index) {
    return is_node_high(index, p2a03_clk0);
}

bool trace_2a03_get_irq(uint32_t index) {
    return is_node_high(index, p2a03_irq);
}

bool trace_2a03_get_nmi(uint32_t index) {
    return is_node_high(index, p2a03_nmi);
}

bool trace_2a03_get_res(uint32_t index) {
    return is_node_high(index, p2a03_res);
}

bool trace_2a03_get_rdy(uint32_t index) {
    return is_node_high(index, p2a03_rdy);
}

uint8_t trace_2a03_get_sq0_out(uint32_t index) {
    return read_nodes(index, 4, nodegroup_sq0);
}

uint8_t trace_2a03_get_sq1_out(uint32_t index) {
    return read_nodes(index, 4, nodegroup_sq1);
}

uint8_t trace_2a03_get_tri_out(uint32_t index) {
    return read_nodes(index, 4, nodegroup_tri);
}

uint8_t trace_2a03_get_noi_out(uint32_t index) {
    return read_nodes(index, 4, nodegroup_noi);
}

uint8_t trace_2a03_get_pcm_out(uint32_t index) {
    return read_nodes(index, 7, nodegroup_pcm);
}
#endif


// disassembler callbacks
static uint8_t dasm_inp_cb(void* user_data) {
    (void)user_data;
    return sim_mem_r8(trace.dasm.cur_addr++);
}

static void dasm_outp_cb(char c, void* user_data) {
    (void)user_data;
    if (trace.dasm.out_str_index < MAX_DASM_STRING_LENGTH) {
        trace.dasm.out_str[trace.dasm.out_str_index++] = c;
    }
    trace.dasm.out_str[MAX_DASM_STRING_LENGTH-1] = 0;
}

static void trace_disassemble(void) {
    trace.dasm.cur_addr = trace.dasm.op_addr;
    trace.dasm.out_str_index = 0;
    #if defined(CHIP_6502) || defined(CHIP_2A03)
        memset(&trace.dasm.out_str, 0, sizeof(trace.dasm.out_str));
        m6502dasm_op(trace.dasm.op_addr, dasm_inp_cb, dasm_outp_cb, 0);
    #elif defined(CHIP_Z80)
        // skip disassembly if last byte was a prefix
        if (!trace.dasm.op_prefixed) {
            memset(&trace.dasm.out_str, 0, sizeof(trace.dasm.out_str));
            z80dasm_op(trace.dasm.op_addr, dasm_inp_cb, dasm_outp_cb, 0);
        }
        switch (sim_mem_r8(trace.dasm.op_addr)) {
            case 0xCB: case 0xDD: case 0xED: case 0xFD:
                trace.dasm.op_prefixed = true;
                break;
            default:
                trace.dasm.op_prefixed = false;
                break;
        }
    #endif
}

void trace_store(void) {
    assert(trace.valid);
    int success = 0;
    uint32_t idx = ring_add();
    trace.items.cycle[idx] = sim_get_cycle();
    memcpy(&trace.items.mem[idx], cpu_memory, sizeof(trace.items.mem[idx]));
    success = sim_write_node_values((range_t){ .ptr=trace.items.node_values[idx], .size=sizeof(trace.items.node_values[idx]) });
    assert(success == 1); (void)success;
    success = sim_write_transistor_on((range_t){ .ptr=trace.items.transistors_on[idx], .size=sizeof(trace.items.transistors_on[idx]) });
    assert(success == 1); (void)success;

    // find start of instruction (first half tick after sync)
    bool disasm = false;
    #if defined(CHIP_6502)
        if (!sim_6502_get_sync() && (trace_num_items() > 1) && trace_6502_get_sync(1)) {
            trace.flip_bits ^= TRACE_FLIPBIT_OP;
            disasm = true;
        }
        if (sim_6502_get_clk0()) {
            trace.flip_bits |= TRACE_FLIPBIT_CLK;
        }
        else {
            trace.flip_bits &= ~TRACE_FLIPBIT_CLK;
        }
        // we need to catch the next instruction address while sync is active
        // (at the end of the current instruction)
        if (sim_6502_get_sync()) {
            trace.dasm.op_addr = sim_get_addr();
        }
    #elif defined(CHIP_Z80)
        if ((trace_num_items() <= 1) || (!sim_z80_get_m1() && (trace_num_items() > 1) && trace_z80_get_m1(1))) {
            trace.flip_bits ^= TRACE_FLIPBIT_OP;
            disasm = true;
            // important, need to catch address bus for proper instruction start, not read PC register!
            trace.dasm.op_addr = sim_get_addr();
        }
        if (sim_z80_get_clk()) {
            trace.flip_bits |= TRACE_FLIPBIT_CLK;
        }
        else {
            trace.flip_bits &= ~TRACE_FLIPBIT_CLK;
        }
    #elif defined(CHIP_2A03)
        if (!sim_2a03_get_sync() && (trace_num_items() > 1) && trace_2a03_get_sync(1)) {
            trace.flip_bits ^= TRACE_FLIPBIT_OP;
            disasm = true;
        }
        if (sim_2a03_get_clk0()) {
            trace.flip_bits |= TRACE_FLIPBIT_CLK;
        }
        else {
            trace.flip_bits &= ~TRACE_FLIPBIT_CLK;
        }
        // we need to catch the next instruction address while sync is active
        // (at the end of the current instruction)
        if (sim_2a03_get_sync()) {
            trace.dasm.op_addr = sim_get_addr();
        }
    #endif
    trace.items.flip_bits[idx] = trace.flip_bits;

    // disassemble current instruction
    if (disasm) {
        trace_disassemble();
    }
    assert(sizeof(trace.items.dasm[idx].str) == sizeof(trace.dasm.out_str));
    memcpy(&trace.items.dasm[idx].str, &trace.dasm.out_str, sizeof(trace.dasm.out_str));
    trace.items.dasm[idx].op_prefixed = trace.dasm.op_prefixed;
    trace.items.dasm[idx].op_addr = trace.dasm.op_addr;
    trace.ui.scroll_to_end = true;
}

static void load_item(uint32_t idx) {
    trace.flip_bits = trace.items.flip_bits[idx];
    trace.dasm.op_prefixed = trace.items.dasm[idx].op_prefixed;
    trace.dasm.op_addr = trace.items.dasm[idx].op_addr;
    assert(sizeof(trace.items.dasm[idx].str) == sizeof(trace.dasm.out_str));
    memcpy(&trace.dasm.out_str, &trace.items.dasm[idx].str, sizeof(trace.dasm.out_str));
    sim_set_cycle(trace.items.cycle[idx]);
    memcpy(cpu_memory, &trace.items.mem[idx][0], sizeof(trace.items.mem[idx]));
    sim_read_node_values((range_t){ trace.items.node_values[idx], sizeof(trace.items.node_values[idx]) });
    sim_read_transistor_on((range_t){ trace.items.transistors_on[idx], sizeof(trace.items.transistors_on[idx]) });
}

// load the previous trace item into the simulator and pop it from the trace log
bool trace_revert_to_previous(void) {
    assert(trace.valid);
    if (trace_num_items() < 1) {
        return false;
    }
    load_item(trace_to_ring_index(1));
    ring_pop();
    return true;
}

// load the selected cycle item into the simulator, and drop following trace items
bool trace_revert_to_cycle(uint32_t cycle) {
    assert(trace.valid);
    // find the selected item by cycle
    uint32_t idx;
    for (idx = trace.tail; idx != trace.head; idx = ring_idx(idx+1)) {
        if (cycle == trace.items.cycle[idx]) {
            break;
        }
    }
    if (idx == trace.head) {
        return false;
    }
    load_item(idx);
    trace.head = ring_idx(idx+1);
    return true;
}

// returns the node-state difference in a 1-byte-per-node array
bool trace_write_diff_visual_state(uint32_t cycle0, uint32_t cycle1, range_t to_buf) {
    assert(trace.valid);
    const int num_nodes = (int)cpu_get_num_nodes();
    if ((int)to_buf.size < num_nodes) {
        return false;
    }
    if (cycle0 > cycle1) {
        uint32_t tmp = cycle1;
        cycle1 = cycle0;
        cycle0 = tmp;
    }
    uint32_t idx0, idx1;
    for (idx0 = trace.tail; idx0 != trace.head; idx0 = ring_idx(idx0+1)) {
        if (cycle0 == trace.items.cycle[idx0]) {
            break;
        }
    }
    if (idx0 == trace.head) {
        return false;
    }
    idx1 = idx0;
    if (cycle0 != cycle1) {
        for (idx1 = trace.tail; idx1 != trace.head; idx1 = ring_idx(idx1+1)) {
            if (cycle1 == trace.items.cycle[idx1]) {
                break;
            }
        }
        if (idx1 == trace.head) {
            return false;
        }
    }
    uint8_t* ptr = to_buf.ptr;
    for (int i = 0; i < num_nodes; i++) {
        bool diff = get_node_value(idx0, i) != get_node_value(idx1, i);
        ptr[i] = diff ? gfx_visual_node_active : gfx_visual_node_inactive;
    }
    return true;
}

void trace_ui_set_scroll_to_end(bool b) {
    assert(trace.valid);
    trace.ui.scroll_to_end = b;
}

bool trace_ui_get_scroll_to_end(void) {
    assert(trace.valid);
    return trace.ui.scroll_to_end;
}
