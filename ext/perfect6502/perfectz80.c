#include "types.h"
#include "netlist_sim.h"
#include "netlist_z80.h"
#include <string.h> // memset

// v6502r additions
size_t cpu_get_num_nodes(void) {
    return sizeof(netlist_z80_node_is_pullup)/sizeof(*netlist_z80_node_is_pullup);
}

bool cpu_read_node_state_as_bytes(void* state, uint8_t active_value, uint8_t inactive_value, uint8_t* ptr, size_t max_nodes) {
    const size_t num_nodes = cpu_get_num_nodes();
    if (num_nodes > max_nodes) {
        return false;
    }
    for (int i = 0; i < (int)num_nodes; i++) {
        ptr[i] = (isNodeHigh(state, i) != 0) ? active_value : inactive_value;
    }
    return true;
}

bool cpu_read_node_values(void* state, uint8_t* ptr, size_t max_bytes) {
    return read_node_values(state, ptr, (int)max_bytes);
}

bool cpu_read_transistor_on(void* state, uint8_t* ptr, size_t max_bytes) {
    return read_transistor_on(state, ptr, (int)max_bytes);
}

bool cpu_write_node_values(void* state, const uint8_t* ptr, size_t max_bytes) {
    return write_node_values(state, ptr, (int)max_bytes);
}

bool cpu_write_transistor_on(void* state, const uint8_t* ptr, size_t max_bytes) {
    return write_transistor_on(state, ptr, (int)max_bytes);
}

/************************************************************
 *
 * Z80-specific Interfacing
 *
 ************************************************************/

static nodenum_t nodes_ab[16] = { ab0, ab1, ab2, ab3, ab4, ab5, ab6, ab7, ab8, ab9, ab10, ab11, ab12, ab13, ab14, ab15, };
static nodenum_t nodes_db[8] = { db0, db1, db2, db3, db4, db5, db6, db7 };
static nodenum_t nodes_ir[8] = { instr0, instr1, instr2, instr3, instr4, instr5, instr6, instr7 };
static nodenum_t nodes_reg_aa[8] = { reg_aa0, reg_aa1, reg_aa2, reg_aa3, reg_aa4, reg_aa5, reg_aa6, reg_aa7 };
static nodenum_t nodes_reg_ff[8] = { reg_ff0, reg_ff1, reg_ff2, reg_ff3, reg_ff4, reg_ff5, reg_ff6, reg_ff7 };
static nodenum_t nodes_reg_bb[8] = { reg_bb0, reg_bb1, reg_bb2, reg_bb3, reg_bb4, reg_bb5, reg_bb6, reg_bb7 };
static nodenum_t nodes_reg_cc[8] = { reg_cc0, reg_cc1, reg_cc2, reg_cc3, reg_cc4, reg_cc5, reg_cc6, reg_cc7 };
static nodenum_t nodes_reg_dd[8] = { reg_dd0, reg_dd1, reg_dd2, reg_dd3, reg_dd4, reg_dd5, reg_dd6, reg_dd7 };
static nodenum_t nodes_reg_ee[8] = { reg_ee0, reg_ee1, reg_ee2, reg_ee3, reg_ee4, reg_ee5, reg_ee6, reg_ee7 };
static nodenum_t nodes_reg_hh[8] = { reg_hh0, reg_hh1, reg_hh2, reg_hh3, reg_hh4, reg_hh5, reg_hh6, reg_hh7 };
static nodenum_t nodes_reg_ll[8] = { reg_ll0, reg_ll1, reg_ll2, reg_ll3, reg_ll4, reg_ll5, reg_ll6, reg_ll7 };

static nodenum_t nodes_reg_a[8] = { reg_a0, reg_a1, reg_a2, reg_a3, reg_a4, reg_a5, reg_a6, reg_a7 };
static nodenum_t nodes_reg_f[8] = { reg_f0, reg_f1, reg_f2, reg_f3, reg_f4, reg_f5, reg_f6, reg_f7 };
static nodenum_t nodes_reg_b[8] = { reg_b0, reg_b1, reg_b2, reg_b3, reg_b4, reg_b5, reg_b6, reg_b7 };
static nodenum_t nodes_reg_c[8] = { reg_c0, reg_c1, reg_c2, reg_c3, reg_c4, reg_c5, reg_c6, reg_c7 };
static nodenum_t nodes_reg_d[8] = { reg_d0, reg_d1, reg_d2, reg_d3, reg_d4, reg_d5, reg_d6, reg_d7 };
static nodenum_t nodes_reg_e[8] = { reg_e0, reg_e1, reg_e2, reg_e3, reg_e4, reg_e5, reg_e6, reg_e7 };
static nodenum_t nodes_reg_h[8] = { reg_h0, reg_h1, reg_h2, reg_h3, reg_h4, reg_h5, reg_h6, reg_h7 };
static nodenum_t nodes_reg_l[8] = { reg_l0, reg_l1, reg_l2, reg_l3, reg_l4, reg_l5, reg_l6, reg_l7 };

static nodenum_t nodes_reg_i[8] = { reg_i0, reg_i1, reg_i2, reg_i3, reg_i4, reg_i5, reg_i6, reg_i7 };
static nodenum_t nodes_reg_r[8] = { reg_r0, reg_r1, reg_r2, reg_r3, reg_r4, reg_r5, reg_r6, reg_r7 };
static nodenum_t nodes_reg_w[8] = { reg_w0, reg_w1, reg_w2, reg_w3, reg_w4, reg_w5, reg_w6, reg_w7 };
static nodenum_t nodes_reg_z[8] = { reg_z0, reg_z1, reg_z2, reg_z3, reg_z4, reg_z5, reg_z6, reg_z7 };
static nodenum_t nodes_reg_ixh[8] = { reg_ixh0, reg_ixh1, reg_ixh2, reg_ixh3, reg_ixh4, reg_ixh5, reg_ixh6, reg_ixh7 };
static nodenum_t nodes_reg_ixl[8] = { reg_ixl0, reg_ixl1, reg_ixl2, reg_ixl3, reg_ixl4, reg_ixl5, reg_ixl6, reg_ixl7 };
static nodenum_t nodes_reg_iyh[8] = { reg_iyh0, reg_iyh1, reg_iyh2, reg_iyh3, reg_iyh4, reg_iyh5, reg_iyh6, reg_iyh7 };
static nodenum_t nodes_reg_iyl[8] = { reg_iyl0, reg_iyl1, reg_iyl2, reg_iyl3, reg_iyl4, reg_iyl5, reg_iyl6, reg_iyl7 };
static nodenum_t nodes_reg_sph[8] = { reg_sph0, reg_sph1, reg_sph2, reg_sph3, reg_sph4, reg_sph5, reg_sph6, reg_sph7 };
static nodenum_t nodes_reg_spl[8] = { reg_spl0, reg_spl1, reg_spl2, reg_spl3, reg_spl4, reg_spl5, reg_spl6, reg_spl7 };
static nodenum_t nodes_reg_pch[8] = { reg_pch0, reg_pch1, reg_pch2, reg_pch3, reg_pch4, reg_pch5, reg_pch6, reg_pch7 };
static nodenum_t nodes_reg_pcl[8] = { reg_pcl0, reg_pcl1, reg_pcl2, reg_pcl3, reg_pcl4, reg_pcl5, reg_pcl6, reg_pcl7 };

static nodenum_t nodes_m[5] = { m1, m2, m3, m4, m5 };
static nodenum_t nodes_t[6] = { t1, t2, t3, t4, t5, t6 };

uint16_t cpu_readAddressBus(void *state) {
    return readNodes(state, 16, nodes_ab);
}

uint8_t cpu_readDataBus(void *state) {
    return readNodes(state, 8, nodes_db);
}

void cpu_writeDataBus(void *state, uint8_t d) {
    writeNodes(state, 8, nodes_db, d);
}

uint8_t cpu_readA(state_t* state) {
    return readNodes(state, 8, isNodeHigh(state, ex_af) ? nodes_reg_a : nodes_reg_aa);
}

uint8_t cpu_readA2(state_t* state) {
    return readNodes(state, 8, !isNodeHigh(state, ex_af) ? nodes_reg_a : nodes_reg_aa);
}

uint8_t cpu_readF(state_t* state) {
    return readNodes(state, 8, isNodeHigh(state, ex_af) ? nodes_reg_f : nodes_reg_ff);
}

uint8_t cpu_readF2(state_t* state) {
    return readNodes(state, 8, !isNodeHigh(state, ex_af) ? nodes_reg_f : nodes_reg_ff);
}

uint8_t cpu_readB(state_t* state) {
    return readNodes(state, 8, isNodeHigh(state, ex_bcdehl) ? nodes_reg_bb : nodes_reg_b);
}

uint8_t cpu_readB2(state_t* state) {
    return readNodes(state, 8, !isNodeHigh(state, ex_bcdehl) ? nodes_reg_bb : nodes_reg_b);
}

uint8_t cpu_readC(state_t* state) {
    return readNodes(state, 8, isNodeHigh(state, ex_bcdehl) ? nodes_reg_cc : nodes_reg_c);
}

uint8_t cpu_readC2(state_t* state) {
    return readNodes(state, 8, !isNodeHigh(state, ex_bcdehl) ? nodes_reg_cc : nodes_reg_c);
}

uint8_t cpu_readD(state_t* state) {
    if (isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_hh : nodes_reg_dd);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_h : nodes_reg_d);
    }
}

uint8_t cpu_readD2(state_t* state) {
    if (!isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_hh : nodes_reg_dd);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_h : nodes_reg_d);
    }
}
uint8_t cpu_readE(state_t* state) {
    if (isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_ll : nodes_reg_ee);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_l : nodes_reg_e);
    }
}

uint8_t cpu_readE2(state_t* state) {
    if (!isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_ll : nodes_reg_ee);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_l : nodes_reg_e);
    }
}

uint8_t cpu_readH(state_t* state) {
    if (isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_dd : nodes_reg_hh);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_d : nodes_reg_h);
    }
}

uint8_t cpu_readH2(state_t* state) {
    if (!isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_dd : nodes_reg_hh);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_d : nodes_reg_h);
    }
}

uint8_t cpu_readL(state_t* state) {
    if (isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_ee : nodes_reg_ll);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_e : nodes_reg_l);
    }
}

uint8_t cpu_readL2(state_t* state) {
    if (!isNodeHigh(state, ex_bcdehl)) {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl1) ? nodes_reg_ee : nodes_reg_ll);
    }
    else {
        return readNodes(state, 8, isNodeHigh(state, ex_dehl0) ? nodes_reg_e : nodes_reg_l);
    }
}

uint8_t cpu_readI(state_t* state) {
    return readNodes(state, 8, nodes_reg_i);
}

uint8_t cpu_readR(state_t* state) {
    return readNodes(state, 8, nodes_reg_r);
}

uint8_t cpu_readW(state_t* state) {
    return readNodes(state, 8, nodes_reg_w);
}

uint8_t cpu_readZ(state_t* state) {
    return readNodes(state, 8, nodes_reg_z);
}

uint8_t cpu_readIR(state_t* state) {
    return readNodes(state, 8, nodes_ir);
}

uint16_t cpu_readIX(state_t* state) {
    return (readNodes(state, 8, nodes_reg_ixh) << 8) | readNodes(state, 8, nodes_reg_ixl);
}

uint16_t cpu_readIY(state_t* state) {
    return (readNodes(state, 8, nodes_reg_iyh) << 8) | readNodes(state, 8, nodes_reg_iyl);
}

uint16_t cpu_readSP(state_t* state) {
    return (readNodes(state, 8, nodes_reg_sph) << 8) | readNodes(state, 8, nodes_reg_spl);
}

uint16_t cpu_readPC(state_t* state) {
    return (readNodes(state, 8, nodes_reg_pch) << 8) | readNodes(state, 8, nodes_reg_pcl);
}

bool cpu_readM1(state_t* state) {
    return isNodeHigh(state, _m1) != 0;
}

bool cpu_readCLK(state_t* state) {
    return isNodeHigh(state, clk) != 0;
}

bool cpu_readMREQ(state_t* state) {
    return isNodeHigh(state, _mreq) != 0;
}

bool cpu_readIORQ(state_t* state) {
    return isNodeHigh(state, _iorq) != 0;
}

bool cpu_readRD(state_t* state) {
    return isNodeHigh(state, _rd) != 0;
}

bool cpu_readWR(state_t* state) {
    return isNodeHigh(state, _wr) != 0;
}

bool cpu_readHALT(state_t* state) {
    return isNodeHigh(state, _halt) != 0;
}

bool cpu_readWAIT(state_t* state) {
    return isNodeHigh(state, _wait) != 0;
}

bool cpu_readRFSH(state_t* state) {
    return isNodeHigh(state, _rfsh) != 0;
}

bool cpu_readINT(state_t* state) {
    return isNodeHigh(state, _int) != 0;
}

bool cpu_readNMI(state_t* state) {
    return isNodeHigh(state, _nmi) != 0;
}

bool cpu_readRESET(state_t* state) {
    return isNodeHigh(state, _reset) != 0;
}

bool cpu_readBUSRQ(state_t* state) {
    return isNodeHigh(state, _busrq) != 0;
}

bool cpu_readBUSAK(state_t* state) {
    return isNodeHigh(state, _busak) != 0;
}

void cpu_writeWAIT(state_t* state, bool high) {
    setNode(state, _wait, high);
}

void cpu_writeINT(state_t* state, bool high) {
    setNode(state, _int, high);
}

void cpu_writeNMI(state_t* state, bool high) {
    setNode(state, _nmi, high);
}

void cpu_writeRESET(state_t* state, bool high) {
    setNode(state, _reset, high);
}

void cpu_writeBUSRQ(state_t* state, bool high) {
    setNode(state, _busrq, high);
}

void cpu_write_node(void* state, int node_index, bool high) {
    setNode(state, (uint16_t)node_index, high ? 1 : 0);
}

bool cpu_read_node(void* state, int node_index) {
    return isNodeHigh(state, (uint16_t)node_index) != 0;
}

uint8_t cpu_readM(state_t* state) {
    return readNodes(state, 5, nodes_m);
}

extern uint8_t cpu_readT(state_t* state) {
    return readNodes(state, 6, nodes_t);
}

/************************************************************
 *
 * Address Bus and Data Bus Interface
 *
 ************************************************************/

uint8_t cpu_memory[0x10000];
uint8_t cpu_io[0x10000];
uint8_t int_vector = 0xE0;

void cpu_setIntVec(uint8_t val) {
    int_vector = val;
}

uint8_t cpu_getIntVec(void) {
    return int_vector;
}

static uint8_t memRead(uint16_t a)
{
    return cpu_memory[a];
}

static void memWrite(uint16_t a, uint8_t d)
{
    cpu_memory[a] = d;
}

static uint8_t ioRead(uint16_t a) {
    return cpu_io[a];
}

static void ioWrite(uint16_t a, uint8_t d) {
    cpu_io[a] = d;
}

static inline void handleBus(void *state) {
    if (!isNodeHigh(state, _mreq)) {
        if (!isNodeHigh(state, _rd)) {
            cpu_writeDataBus(state, memRead(cpu_readAddressBus(state)));
        }
        if (!isNodeHigh(state, _wr)) {
            memWrite(cpu_readAddressBus(state), cpu_readDataBus(state));
        }
    }
    if (!isNodeHigh(state, _iorq)) {
        if (!isNodeHigh(state, _m1)) {
            // interrupt acknowledge cycle, put interrupt vector on data bus
            cpu_writeDataBus(state, int_vector);
        }
        if (!isNodeHigh(state, _rd)) {
            cpu_writeDataBus(state, ioRead(cpu_readAddressBus(state)));
        }
        if (!isNodeHigh(state, _wr)) {
            ioWrite(cpu_readAddressBus(state), cpu_readDataBus(state));
        }
    }
    // FIXME: integer request cycle (put interrupt vector on data bus)
}

/************************************************************
 *
 * Main Clock Loop
 *
 ************************************************************/

uint32_t cpu_cycle;

void cpu_step(void *state) {
    BOOL clock = isNodeHigh(state, clk);

    // invert clock
    setNode(state, clk, !clock);
    recalcNodeList(state);

    // handle memory reads and writes
    if (!clock) {
        handleBus(state);
    }
    cpu_cycle++;
}

void* cpu_initAndResetChip(void) {
    memset(cpu_io, 0xFF, sizeof(cpu_io));

    nodenum_t nodes = sizeof(netlist_z80_node_is_pullup)/sizeof(*netlist_z80_node_is_pullup);
    nodenum_t transistors = sizeof(netlist_z80_transdefs)/sizeof(*netlist_z80_transdefs);
    void *state = setupNodesAndTransistors(netlist_z80_transdefs,
        netlist_z80_node_is_pullup,
        nodes,
        transistors,
        vss,
        vcc);
    modelChargeSharing(state, YES);

    setNode(state, _reset, 0);
    setNode(state, clk, 1);
    setNode(state, _busrq, 1);
    setNode(state, _int, 1);
    setNode(state, _nmi, 1);
    setNode(state, _wait, 1);

    stabilizeChip(state);

    for (int i = 0; i < 31; i++) {
        cpu_step(state);
    }
    setNode(state, _reset, 1);
    recalcNodeList(state);

    // 4 half cycles before M1 toggles
    for (int i = 0; i < 4; i++) {
        cpu_step(state);
    }

    cpu_cycle = 0;
    return state;
}

void cpu_destroyChip(void *state) {
    destroyNodesAndTransistors(state);
}
