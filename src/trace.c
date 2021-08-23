//------------------------------------------------------------------------------
//  trace.c
//  Keep a history of chip state.
//------------------------------------------------------------------------------
#include "perfect6502.h"
#include "trace.h"
#include "sim.h"
#include <string.h>

void trace_init(trace_t* trace) {
    assert(trace);
    memset(trace, 0, sizeof(trace_t));
}

void trace_shutdown(trace_t* trace) {
    (void)trace;
}

void trace_clear(trace_t* trace) {
    assert(trace);
    memset(trace, 0, sizeof(trace_t));
}

// ring buffer helper functions
static uint32_t ring_idx(uint32_t i) {
    return (i % MAX_TRACE_ITEMS);
}

static bool ring_full(const trace_t* trace) {
    return ring_idx(trace->head+1) == trace->tail;
}

static bool ring_empty(const trace_t* trace) {
    return trace->head == trace->tail;
}

static uint32_t ring_count(const trace_t* trace) {
    if (trace->head >= trace->tail) {
        return trace->head - trace->tail;
    }
    else {
        return (trace->head + MAX_TRACE_ITEMS) - trace->tail;
    }
}

static uint32_t ring_add(trace_t* trace) {
    uint32_t idx = trace->head;
    if (ring_full(trace)) {
        trace->tail = ring_idx(trace->tail+1);
    }
    trace->head = ring_idx(trace->head+1);
    return idx;
}

static void ring_pop(trace_t* trace) {
    if (!ring_empty(trace)) {
        trace->head = ring_idx(trace->head-1);
    }
}

uint32_t trace_num_items(const trace_t* trace) {
    assert(trace);
    return ring_count(trace);
}

bool trace_empty(const trace_t* trace) {
    assert(trace);
    return ring_empty(trace);
}

static uint32_t trace_to_ring_index(const trace_t* trace, uint32_t trace_index) {
    assert(trace);
    uint32_t idx = ring_idx(trace->head - 1 - trace_index);
    assert(idx != trace->head);
    return idx;
}

static int get_bitmap(const uint32_t* bm, uint32_t index) {
    return (bm[index>>5]>>(index&0x1F)) & 1;
}

static int get_nodes_value(const trace_item_t* item, uint32_t node_index) {
    return get_bitmap(item->node_values, node_index);
};

static bool is_node_high(const trace_t* trace, uint32_t trace_index, uint32_t node_index) {
    uint32_t idx = trace_to_ring_index(trace, trace_index);
    return 0 != get_nodes_value(&trace->items[idx], node_index);
}

static uint32_t read_nodes(const trace_t* trace, uint32_t trace_index, uint32_t count, uint32_t* node_indices) {
    uint32_t result = 0;
    for (int i = count - 1; i >= 0; i--) {
        result <<=  1;
        result |= is_node_high(trace, trace_index, node_indices[i]);
    }
    return result;
}

enum {
//    D1x1 = 827,
    a0 = 737,
    a1 = 1234,
    a2 = 978,
    a3 = 162,
    a4 = 727,
    a5 = 858,
    a6 = 1136,
    a7 = 1653,
    ab0 = 268,
    ab1 = 451,
    ab2 = 1340,
    ab3 = 211,
    ab4 = 435,
    ab5 = 736,
    ab6 = 887,
    ab7 = 1493,
    ab8 = 230,
    ab9 = 148,
    ab10 = 1443,
    ab11 = 399,
    ab12 = 1237,
    ab13 = 349,
    ab14 = 672,
    ab15 = 195,
//    adh0 = 407,
//    adh1 = 52,
//    adh2 = 1651,
//    adh3 = 315,
//    adh4 = 1160,
//    adh5 = 483,
//    adh6 = 13,
//    adh7 = 1539,
//    adl0 = 413,
//    adl1 = 1282,
//    adl2 = 1242,
//    adl3 = 684,
//    adl4 = 1437,
//    adl5 = 1630,
//    adl6 = 121,
//    adl7 = 1299,
//    alu0 = 394,
//    alu1 = 697,
//    alu2 = 276,
//    alu3 = 495,
//    alu4 = 1490,
//    alu5 = 893,
//    alu6 = 68,
//    alu7 = 1123,
//    cclk = 943,   // aka cp2
//    clearIR = 1077,
    clk0 = 1171,
//    clk1out = 1163,
//    clk2out = 421,
//    clock1 = 156,
//    clock2 = 1536,
//    cp1 = 710,
    db0 = 1005,
    db1 = 82,
    db2 = 945,
    db3 = 650,
    db4 = 1393,
    db5 = 175,
    db6 = 1591,
    db7 = 1349,
//    dor0 = 222,
//    dor1 = 527,
//    dor2 = 1288,
//    dor3 = 823,
//    dor4 = 873,
//    dor5 = 1266,
//    dor6 = 1418,
//    dor7 = 158,
//    fetch = 879,
//    h1x1 = 1042 // drive status byte onto databus
//    idb0 = 1108,
//    idb1 = 991,
//    idb2 = 1473,
//    idb3 = 1302,
//    idb4 = 892,
//    idb5 = 1503,
//    idb6 = 833,
//    idb7 = 493,
//    idl0 = 116,
//    idl1 = 576,
//    idl2 = 1485,
//    idl3 = 1284,
//    idl4 = 1516,
//    idl5 = 498,
//    idl6 = 1537,
//    idl7 = 529,
    irq = 103,
    nmi = 1297,
//    notRdy0 = 248,
    notir0 = 194,
    notir1 = 702,
    notir2 = 1182,
    notir3 = 1125,
    notir4 = 26,
    notir5 = 1394,  // OK
    notir6 = 895,   // OK
    notir7 = 1320,
//    nots0 = 418,
//    nots1 = 1064,
//    nots2 = 752,
//    nots3 = 828,
//    nots4 = 1603,
//    nots5 = 601,
//    nots6 = 1029,
//    nots7 = 181,
    p0 = 687,
    p1 = 1444,
    p2 = 1421,
    p3 = 439,
    p4 = 1119,
    p5 = 0,
    p6 = 77,
    p7 = 1370,
    pch0 = 1670,
    pch1 = 292,
    pch2 = 502,
    pch3 = 584,
    pch4 = 948,
    pch5 = 49,
    pch6 = 1551,
    pch7 = 205,
    pcl0 = 1139,
    pcl1 = 1022,
    pcl2 = 655,
    pcl3 = 1359,
    pcl4 = 900,
    pcl5 = 622,
    pcl6 = 377,
    pcl7 = 1611,
//    pd0 = 758,
//    pd1 = 361,
//    pd2 = 955,
//    pd3 = 894,
//    pd4 = 369,
//    pd5 = 829,
//    pd6 = 1669,
//    pd7 = 1690,
    rdy = 89,
    res = 159,
    rw = 1156,
    s0 = 1403,
    s1 = 183,
    s2 = 81,
    s3 = 1532,
    s4 = 1702,
    s5 = 1098,
    s6 = 1212,
    s7 = 1435,
//    sb0 = 54,
//    sb1 = 1150,
//    sb2 = 1287,
//    sb3 = 1188,
//    sb4 = 1405,
//    sb5 = 166,
//    sb6 = 1336,
//    sb7 = 1001,
    so = 1672,
    sync_ = 539,
//    t2 = 971,
//    t3 = 1567,
//    t4 = 690,
//    t5 = 909,
    vcc = 657,
    vss = 558,
    x0 = 1216,
    x1 = 98,
    x2 = 1,
    x3 = 1648,
    x4 = 85,
    x5 = 589,
    x6 = 448,
    x7 = 777,
    y0 = 64,
    y1 = 1148,
    y2 = 573,
    y3 = 305,
    y4 = 989,
    y5 = 615,
    y6 = 115,
    y7 = 843,
};

uint32_t trace_get_cycle(const trace_t* trace, uint32_t index) {
    assert(trace);
    uint32_t idx = trace_to_ring_index(trace, index);
    return trace->items[idx].cycle;
}

uint32_t trace_get_flipbits(const trace_t* trace, uint32_t index) {
    assert(trace);
    uint32_t idx = trace_to_ring_index(trace, index);
    return trace->items[idx].flip_bits;
}

uint8_t trace_get_a(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ a0,a1,a2,a3,a4,a5,a6,a7 });
}

uint8_t trace_get_x(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ x0,x1,x2,x3,x4,x5,x6,x7 });
}

uint8_t trace_get_y(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ y0,y1,y2,y3,y4,y5,y6,y7 });
}

uint8_t trace_get_sp(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ s0,s1,s2,s3,s4,s5,s6,s7 });
}

static uint8_t trace_get_pcl(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ pcl0,pcl1,pcl2,pcl3,pcl4,pcl5,pcl6,pcl7 });
}

static uint8_t trace_get_pch(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ pch0,pch1,pch2,pch3,pch4,pch5,pch6,pch7 });
}

uint16_t trace_get_pc(const trace_t* trace, uint32_t index) {
    return (trace_get_pch(trace, index)<<8)|trace_get_pcl(trace, index);
}

uint8_t trace_get_ir(const trace_t* trace, uint32_t index) {
    return ~read_nodes(trace, index, 8, (uint32_t[]){ notir0,notir1,notir2,notir3,notir4,notir5,notir6,notir7 });
}

uint8_t trace_get_p(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ p0,p1,p2,p3,p4,p5,p6,p7 });
}

bool trace_get_rw(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, rw);
}

bool trace_get_sync(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, sync_);
}

bool trace_get_clk0(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, clk0);
}

bool trace_get_irq(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, irq);
}

bool trace_get_nmi(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, nmi);
}

bool trace_get_res(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, res);
}

bool trace_get_rdy(const trace_t* trace, uint32_t index) {
    return is_node_high(trace, index, rdy);
}

uint16_t trace_get_addr(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 16, (uint32_t[]){ ab0, ab1, ab2, ab3, ab4, ab5, ab6, ab7, ab8, ab9, ab10, ab11, ab12, ab13, ab14, ab15 });
}

uint8_t trace_get_data(const trace_t* trace, uint32_t index) {
    return read_nodes(trace, index, 8, (uint32_t[]){ db0, db1, db2, db3, db4, db5, db6, db7 });
}

void trace_store(trace_t* trace, struct sim_t* sim) {
    assert(trace && sim);
    uint32_t idx = ring_add(trace);
    trace_item_t* item = &trace->items[idx];
    item->cycle = sim_get_cycle();
    memcpy(&item->mem, memory, sizeof(item->mem));
    sim_read_node_values(sim, (uint8_t*)item->node_values, sizeof(item->node_values));
    sim_read_transistor_on(sim, (uint8_t*)item->transistors_on, sizeof(item->transistors_on));
    // find start of instruction (first half tick after sync)
    if (!sim_get_sync(sim) && (trace_num_items(trace) > 1) && trace_get_sync(trace, 1)) {
        trace->flip_bits ^= TRACE_FLIPBIT_OP;
    }
    if (sim_get_clk0(sim)) {
        trace->flip_bits |= TRACE_FLIPBIT_CLK0;
    }
    else {
        trace->flip_bits &= ~TRACE_FLIPBIT_CLK0;
    }
    item->flip_bits = trace->flip_bits;
    trace->ui.log_scroll_to_end = true;
}

static void load_item(trace_t* trace, trace_item_t* item, sim_t* sim) {
    trace->flip_bits = item->flip_bits;
    sim_set_cycle(item->cycle);
    memcpy(memory, &item->mem, sizeof(item->mem));
    sim_write_node_values(sim, (const uint8_t*)item->node_values, sizeof(item->node_values));
    sim_write_transistor_on(sim, (const uint8_t*)item->transistors_on, sizeof(item->transistors_on));
}

// load the previous trace item into the simulator and pop it from the trace log
bool trace_revert_to_previous(trace_t* trace, struct sim_t* sim) {
    assert(trace);
    if (trace_num_items(trace) < 1) {
        return false;
    }
    uint32_t idx = trace_to_ring_index(trace, 1);
    trace_item_t* item = &trace->items[idx];
    load_item(trace, item, sim);
    ring_pop(trace);
    return true;
}

// load the selected trace item into the simulator, and drop following trace items
bool trace_revert_to_selected(trace_t* trace, struct sim_t* sim) {
    assert(trace);
    if (!trace->ui.selected) {
        return false;
    }
    // find the selected item by cycle
    uint32_t idx;
    for (idx = trace->tail; idx != trace->head; idx = ring_idx(idx+1)) {
        if (trace->ui.selected_cycle == trace->items[idx].cycle) {
            break;
        }
    }
    if (idx == trace->head) {
        return false;
    }
    trace_item_t* item = &trace->items[idx];
    load_item(trace, item, sim);
    trace->head = ring_idx(idx+1);
    return true;
}
