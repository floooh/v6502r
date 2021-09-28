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
#endif
#include "nodenames.h"
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
        uint32_t node_values[MAX_TRACE_ITEMS][128];
        uint32_t transistors_on[MAX_TRACE_ITEMS][256];
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

#if defined(CHIP_6502)
static uint32_t nodes_ab[16] = { p6502_ab0, p6502_ab1, p6502_ab2, p6502_ab3, p6502_ab4, p6502_ab5, p6502_ab6, p6502_ab7, p6502_ab8, p6502_ab9, p6502_ab10, p6502_ab11, p6502_ab12, p6502_ab13, p6502_ab14, p6502_ab15 };
static uint32_t nodes_db[8]  = { p6502_db0, p6502_db1, p6502_db2, p6502_db3, p6502_db4, p6502_db5, p6502_db6, p6502_db7 };
static uint32_t nodes_pcl[8] = { p6502_pcl0,p6502_pcl1,p6502_pcl2,p6502_pcl3,p6502_pcl4,p6502_pcl5,p6502_pcl6,p6502_pcl7 };
static uint32_t nodes_pch[8] = { p6502_pch0,p6502_pch1,p6502_pch2,p6502_pch3,p6502_pch4,p6502_pch5,p6502_pch6,p6502_pch7 };
static uint32_t nodes_p[8]   = { p6502_p0,p6502_p1,p6502_p2,p6502_p3,p6502_p4,0,p6502_p6,p6502_p7 }; // missing p6502_p5 is not a typo (there is no physical bit 5 in status reg)
static uint32_t nodes_a[8]   = { p6502_a0,p6502_a1,p6502_a2,p6502_a3,p6502_a4,p6502_a5,p6502_a6,p6502_a7 };
static uint32_t nodes_x[8]   = { p6502_x0,p6502_x1,p6502_x2,p6502_x3,p6502_x4,p6502_x5,p6502_x6,p6502_x7 };
static uint32_t nodes_y[8]   = { p6502_y0,p6502_y1,p6502_y2,p6502_y3,p6502_y4,p6502_y5,p6502_y6,p6502_y7 };
static uint32_t nodes_sp[8]  = { p6502_s0,p6502_s1,p6502_s2,p6502_s3,p6502_s4,p6502_s5,p6502_s6,p6502_s7 };
static uint32_t nodes_ir[8]  = { p6502_notir0,p6502_notir1,p6502_notir2,p6502_notir3,p6502_notir4,p6502_notir5,p6502_notir6,p6502_notir7 };
#elif defined(CHIP_Z80)
static uint32_t nodes_ab[16] = { pz80_ab0,pz80_ab1,pz80_ab2,pz80_ab3,pz80_ab4,pz80_ab5,pz80_ab6,pz80_ab7,pz80_ab8,pz80_ab9,pz80_ab10,pz80_ab11,pz80_ab12,pz80_ab13,pz80_ab14,pz80_ab15 };
static uint32_t nodes_db[8]  = { pz80_db0,pz80_db1,pz80_db2,pz80_db3,pz80_db4,pz80_db5,pz80_db6,pz80_db7 };
static uint32_t nodes_pcl[8] = { pz80_reg_pcl0,pz80_reg_pcl1,pz80_reg_pcl2,pz80_reg_pcl3,pz80_reg_pcl4,pz80_reg_pcl5,pz80_reg_pcl6,pz80_reg_pcl7 };
static uint32_t nodes_pch[8] = { pz80_reg_pch0,pz80_reg_pch1,pz80_reg_pch2,pz80_reg_pch3,pz80_reg_pch4,pz80_reg_pch5,pz80_reg_pch6,pz80_reg_pch7 };
static uint32_t nodes_ir[8]  = { pz80_instr0,pz80_instr1,pz80_instr2,pz80_instr3,pz80_instr4,pz80_instr5,pz80_instr6,pz80_instr7 };
static uint32_t nodes_reg_ff[8] = { pz80_reg_ff0, pz80_reg_ff1, pz80_reg_ff2, pz80_reg_ff3, pz80_reg_ff4, pz80_reg_ff5, pz80_reg_ff6, pz80_reg_ff7 };
static uint32_t nodes_reg_aa[8] = { pz80_reg_aa0, pz80_reg_aa1, pz80_reg_aa2, pz80_reg_aa3, pz80_reg_aa4, pz80_reg_aa5, pz80_reg_aa6, pz80_reg_aa7 };
static uint32_t nodes_reg_bb[8] = { pz80_reg_bb0, pz80_reg_bb1, pz80_reg_bb2, pz80_reg_bb3, pz80_reg_bb4, pz80_reg_bb5, pz80_reg_bb6, pz80_reg_bb7 };
static uint32_t nodes_reg_cc[8] = { pz80_reg_cc0, pz80_reg_cc1, pz80_reg_cc2, pz80_reg_cc3, pz80_reg_cc4, pz80_reg_cc5, pz80_reg_cc6, pz80_reg_cc7 };
static uint32_t nodes_reg_dd[8] = { pz80_reg_dd0, pz80_reg_dd1, pz80_reg_dd2, pz80_reg_dd3, pz80_reg_dd4, pz80_reg_dd5, pz80_reg_dd6, pz80_reg_dd7 };
static uint32_t nodes_reg_ee[8] = { pz80_reg_ee0, pz80_reg_ee1, pz80_reg_ee2, pz80_reg_ee3, pz80_reg_ee4, pz80_reg_ee5, pz80_reg_ee6, pz80_reg_ee7 };
static uint32_t nodes_reg_hh[8] = { pz80_reg_hh0, pz80_reg_hh1, pz80_reg_hh2, pz80_reg_hh3, pz80_reg_hh4, pz80_reg_hh5, pz80_reg_hh6, pz80_reg_hh7 };
static uint32_t nodes_reg_ll[8] = { pz80_reg_ll0, pz80_reg_ll1, pz80_reg_ll2, pz80_reg_ll3, pz80_reg_ll4, pz80_reg_ll5, pz80_reg_ll6, pz80_reg_ll7 };
static uint32_t nodes_reg_f[8]  = { pz80_reg_f0, pz80_reg_f1, pz80_reg_f2, pz80_reg_f3, pz80_reg_f4, pz80_reg_f5, pz80_reg_f6, pz80_reg_f7 };
static uint32_t nodes_reg_a[8] = { pz80_reg_a0, pz80_reg_a1, pz80_reg_a2, pz80_reg_a3, pz80_reg_a4, pz80_reg_a5, pz80_reg_a6, pz80_reg_a7 };
static uint32_t nodes_reg_b[8] = { pz80_reg_b0, pz80_reg_b1, pz80_reg_b2, pz80_reg_b3, pz80_reg_b4, pz80_reg_b5, pz80_reg_b6, pz80_reg_b7 };
static uint32_t nodes_reg_c[8] = { pz80_reg_c0, pz80_reg_c1, pz80_reg_c2, pz80_reg_c3, pz80_reg_c4, pz80_reg_c5, pz80_reg_c6, pz80_reg_c7 };
static uint32_t nodes_reg_d[8] = { pz80_reg_d0, pz80_reg_d1, pz80_reg_d2, pz80_reg_d3, pz80_reg_d4, pz80_reg_d5, pz80_reg_d6, pz80_reg_d7 };
static uint32_t nodes_reg_e[8] = { pz80_reg_e0, pz80_reg_e1, pz80_reg_e2, pz80_reg_e3, pz80_reg_e4, pz80_reg_e5, pz80_reg_e6, pz80_reg_e7 };
static uint32_t nodes_reg_h[8] = { pz80_reg_h0, pz80_reg_h1, pz80_reg_h2, pz80_reg_h3, pz80_reg_h4, pz80_reg_h5, pz80_reg_h6, pz80_reg_h7 };
static uint32_t nodes_reg_l[8] = { pz80_reg_l0, pz80_reg_l1, pz80_reg_l2, pz80_reg_l3, pz80_reg_l4, pz80_reg_l5, pz80_reg_l6, pz80_reg_l7 };
static uint32_t nodes_reg_i[8] = { pz80_reg_i0, pz80_reg_i1, pz80_reg_i2, pz80_reg_i3, pz80_reg_i4, pz80_reg_i5, pz80_reg_i6, pz80_reg_i7 };
static uint32_t nodes_reg_r[8] = { pz80_reg_r0, pz80_reg_r1, pz80_reg_r2, pz80_reg_r3, pz80_reg_r4, pz80_reg_r5, pz80_reg_r6, pz80_reg_r7 };
static uint32_t nodes_reg_w[8] = { pz80_reg_w0, pz80_reg_w1, pz80_reg_w2, pz80_reg_w3, pz80_reg_w4, pz80_reg_w5, pz80_reg_w6, pz80_reg_w7 };
static uint32_t nodes_reg_z[8] = { pz80_reg_z0, pz80_reg_z1, pz80_reg_z2, pz80_reg_z3, pz80_reg_z4, pz80_reg_z5, pz80_reg_z6, pz80_reg_z7 };
static uint32_t nodes_reg_ixh[8] = { pz80_reg_ixh0, pz80_reg_ixh1, pz80_reg_ixh2, pz80_reg_ixh3, pz80_reg_ixh4, pz80_reg_ixh5, pz80_reg_ixh6, pz80_reg_ixh7 };
static uint32_t nodes_reg_ixl[8] = { pz80_reg_ixl0, pz80_reg_ixl1, pz80_reg_ixl2, pz80_reg_ixl3, pz80_reg_ixl4, pz80_reg_ixl5, pz80_reg_ixl6, pz80_reg_ixl7 };
static uint32_t nodes_reg_iyh[8] = { pz80_reg_iyh0, pz80_reg_iyh1, pz80_reg_iyh2, pz80_reg_iyh3, pz80_reg_iyh4, pz80_reg_iyh5, pz80_reg_iyh6, pz80_reg_iyh7 };
static uint32_t nodes_reg_iyl[8] = { pz80_reg_iyl0, pz80_reg_iyl1, pz80_reg_iyl2, pz80_reg_iyl3, pz80_reg_iyl4, pz80_reg_iyl5, pz80_reg_iyl6, pz80_reg_iyl7 };
static uint32_t nodes_reg_sph[8] = { pz80_reg_sph0, pz80_reg_sph1, pz80_reg_sph2, pz80_reg_sph3, pz80_reg_sph4, pz80_reg_sph5, pz80_reg_sph6, pz80_reg_sph7 };
static uint32_t nodes_reg_spl[8] = { pz80_reg_spl0, pz80_reg_spl1, pz80_reg_spl2, pz80_reg_spl3, pz80_reg_spl4, pz80_reg_spl5, pz80_reg_spl6, pz80_reg_spl7 };
#endif

uint16_t trace_get_addr(uint32_t index) {
    return read_nodes(index, 16, nodes_ab);
}

uint8_t trace_get_data(uint32_t index) {
    return read_nodes(index, 8, nodes_db);
}

uint16_t trace_get_pc(uint32_t index) {
    uint8_t pcl = read_nodes(index, 8, nodes_pcl);
    uint8_t pch = read_nodes(index, 8, nodes_pch);
    return (pch<<8)|pcl;
}

uint8_t trace_get_flags(uint32_t index) {
    #if defined(CHIP_6502)
    return read_nodes(index, 8, nodes_p);
    #else
    return read_nodes(index, 8, is_node_high(index, pz80_ex_af) ? nodes_reg_f : nodes_reg_ff);
    #endif
}

const char* trace_get_disasm(uint32_t index) {
    uint32_t idx = trace_to_ring_index(index);
    return trace.items.dasm[idx].str;
}

#if defined(CHIP_6502)
uint8_t trace_6502_get_a(uint32_t index) {
    return read_nodes(index, 8, nodes_a);
}

uint8_t trace_6502_get_x(uint32_t index) {
    return read_nodes(index, 8, nodes_x);
}

uint8_t trace_6502_get_y(uint32_t index) {
    return read_nodes(index, 8, nodes_y);
}

uint8_t trace_6502_get_sp(uint32_t index) {
    return read_nodes(index, 8, nodes_sp);
}

uint8_t trace_6502_get_ir(uint32_t index) {
    return ~read_nodes(index, 8, nodes_ir);
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

uint8_t trace_z80_get_ir(uint32_t index) {
    return read_nodes(index, 8, nodes_ir);
}

uint8_t trace_z80_get_a(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_af) ? nodes_reg_a : nodes_reg_aa);
}

uint8_t trace_z80_get_a2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_af) ? nodes_reg_a : nodes_reg_aa);
}

uint8_t trace_z80_get_f(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_af) ? nodes_reg_f : nodes_reg_ff);
}

uint8_t trace_z80_get_f2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_af) ? nodes_reg_f : nodes_reg_ff);
}

uint8_t trace_z80_get_b(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_bcdehl) ? nodes_reg_bb : nodes_reg_b);
}

uint8_t trace_z80_get_b2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_bcdehl) ? nodes_reg_bb : nodes_reg_b);
}

uint8_t trace_z80_get_c(uint32_t index) {
    return read_nodes(index, 8, is_node_high(index, pz80_ex_bcdehl) ? nodes_reg_cc : nodes_reg_c);
}

uint8_t trace_z80_get_c2(uint32_t index) {
    return read_nodes(index, 8, !is_node_high(index, pz80_ex_bcdehl) ? nodes_reg_cc : nodes_reg_c);
}

uint8_t trace_z80_get_d(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_hh : nodes_reg_dd);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_h : nodes_reg_d);
    }
}

uint8_t trace_z80_get_d2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_hh : nodes_reg_dd);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_h : nodes_reg_d);
    }
}

uint8_t trace_z80_get_e(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_ll : nodes_reg_ee);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_l : nodes_reg_e);
    }
}

uint8_t trace_z80_get_e2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_ll : nodes_reg_ee);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_l : nodes_reg_e);
    }
}

uint8_t trace_z80_get_h(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_dd : nodes_reg_hh);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_d : nodes_reg_h);
    }
}

uint8_t trace_z80_get_h2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_dd : nodes_reg_hh);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_d : nodes_reg_h);
    }
}

uint8_t trace_z80_get_l(uint32_t index) {
    if (is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_ee : nodes_reg_ll);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_e : nodes_reg_l);
    }
}

uint8_t trace_z80_get_l2(uint32_t index) {
    if (!is_node_high(index, pz80_ex_bcdehl)) {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl1) ? nodes_reg_ee : nodes_reg_ll);
    }
    else {
        return read_nodes(index, 8, is_node_high(index, pz80_ex_dehl0) ? nodes_reg_e : nodes_reg_l);
    }
}

uint8_t trace_z80_get_i(uint32_t index) {
    return read_nodes(index, 8, nodes_reg_i);
}

uint8_t trace_z80_get_r(uint32_t index) {
    return read_nodes(index, 8, nodes_reg_r);
}

uint8_t trace_z80_get_w(uint32_t index) {
    return read_nodes(index, 8, nodes_reg_w);
}

uint8_t trace_z80_get_z(uint32_t index) {
    return read_nodes(index, 8, nodes_reg_z);
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
    return (read_nodes(index, 8, nodes_reg_ixh) << 8) | read_nodes(index, 8, nodes_reg_ixl);
}

uint16_t trace_z80_get_iy(uint32_t index) {
    return (read_nodes(index, 8, nodes_reg_iyh) << 8) | read_nodes(index, 8, nodes_reg_iyl);
}

uint16_t trace_z80_get_sp(uint32_t index) {
    return (read_nodes(index, 8, nodes_reg_sph) << 8) | read_nodes(index, 8, nodes_reg_spl);
}

uint16_t trace_z80_get_wz(uint32_t index) {
    return (trace_z80_get_w(index)<<8) | trace_z80_get_z(index);
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
    #if defined(CHIP_6502)
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
    #else
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
