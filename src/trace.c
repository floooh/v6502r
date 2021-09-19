//------------------------------------------------------------------------------
//  trace.c
//  Keep a history of chip state.
//------------------------------------------------------------------------------
#if defined(CHIP_6502)
#include "perfect6502.h"
#elif defined(CHIP_Z80)
#include "perfectz80.h"
#endif
#include "nodenames.h"
#include "trace.h"
#include "sim.h"

typedef struct {
    uint32_t cycle;
    uint32_t flip_bits;     // for grouping instruction and cycle ticks
    uint32_t node_values[64];
    uint32_t transistors_on[128];
    uint8_t mem[4096];      // only trace the first few KB of memory
} trace_item_t;

static struct {
    bool valid;
    uint32_t flip_bits;         // for visually separating instructions and cycles
    uint32_t head;
    uint32_t tail;
    trace_item_t items[MAX_TRACE_ITEMS];
    struct {
        bool scroll_to_end;
        bool selected;              // true if a trace item is currently selected in the UI
        uint32_t selected_cycle;    // selected cycle number
    } ui;
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

uint32_t trace_num_items(void) {
    assert(trace.valid);
    return ring_count();
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

static int get_nodes_value(const trace_item_t* item, uint32_t node_index) {
    return get_bitmap(item->node_values, node_index);
};

static bool is_node_high(uint32_t trace_index, uint32_t node_index) {
    uint32_t idx = trace_to_ring_index(trace_index);
    return 0 != get_nodes_value(&trace.items[idx], node_index);
}

static uint32_t read_nodes(uint32_t trace_index, uint32_t count, uint32_t* node_indices) {
    uint32_t result = 0;
    for (int i = count - 1; i >= 0; i--) {
        result <<=  1;
        result |= is_node_high(trace_index, node_indices[i]);
    }
    return result;
}

uint32_t trace_get_cycle(uint32_t index) {
    assert(trace.valid);
    uint32_t idx = trace_to_ring_index(index);
    return trace.items[idx].cycle;
}

uint32_t trace_get_flipbits(uint32_t index) {
    assert(trace.valid);
    uint32_t idx = trace_to_ring_index(index);
    return trace.items[idx].flip_bits;
}

uint16_t trace_get_addr(uint32_t index) {
    assert(trace.valid);
    #if defined(CHIP_6502)
    return read_nodes(index, 16, (uint32_t[]){ p6502_ab0, p6502_ab1, p6502_ab2, p6502_ab3, p6502_ab4, p6502_ab5, p6502_ab6, p6502_ab7, p6502_ab8, p6502_ab9, p6502_ab10, p6502_ab11, p6502_ab12, p6502_ab13, p6502_ab14, p6502_ab15 });
    #elif defined(CHIP_Z80)
    return read_nodes(index, 16, (uint32_t[]){ pz80_ab0, pz80_ab1, pz80_ab2, pz80_ab3, pz80_ab4, pz80_ab5, pz80_ab6, pz80_ab7, pz80_ab8, pz80_ab9, pz80_ab10, pz80_ab11, pz80_ab12, pz80_ab13, pz80_ab14, pz80_ab15 });
    #else
    #error "Unknown chip define!"
    #endif
}

uint8_t trace_get_data(uint32_t index) {
    assert(trace.valid);
    #if defined(CHIP_6502)
    return read_nodes(index, 8, (uint32_t[]){ p6502_db0, p6502_db1, p6502_db2, p6502_db3, p6502_db4, p6502_db5, p6502_db6, p6502_db7 });
    #elif defined(CHIP_Z80)
    return read_nodes(index, 8, (uint32_t[]){ pz80_db0, pz80_db1, pz80_db2, pz80_db3, pz80_db4, pz80_db5, pz80_db6, pz80_db7 });
    #else
    #error "Unknown chip define!"
    #endif
}


#if defined(CHIP_6502)
uint8_t trace_get_a(uint32_t index) {
    assert(trace.valid);
    return read_nodes(index, 8, (uint32_t[]){ p6502_a0,p6502_a1,p6502_a2,p6502_a3,p6502_a4,p6502_a5,p6502_a6,p6502_a7 });
}

uint8_t trace_get_x(uint32_t index) {
    assert(trace.valid);
    return read_nodes(index, 8, (uint32_t[]){ p6502_x0,p6502_x1,p6502_x2,p6502_x3,p6502_x4,p6502_x5,p6502_x6,p6502_x7 });
}

uint8_t trace_get_y(uint32_t index) {
    assert(trace.valid);
    return read_nodes(index, 8, (uint32_t[]){ p6502_y0,p6502_y1,p6502_y2,p6502_y3,p6502_y4,p6502_y5,p6502_y6,p6502_y7 });
}

uint8_t trace_get_sp(uint32_t index) {
    assert(trace.valid);
    return read_nodes(index, 8, (uint32_t[]){ p6502_s0,p6502_s1,p6502_s2,p6502_s3,p6502_s4,p6502_s5,p6502_s6,p6502_s7 });
}

static uint8_t trace_get_pcl(uint32_t index) {
    assert(trace.valid);
    return read_nodes(index, 8, (uint32_t[]){ p6502_pcl0,p6502_pcl1,p6502_pcl2,p6502_pcl3,p6502_pcl4,p6502_pcl5,p6502_pcl6,p6502_pcl7 });
}

static uint8_t trace_get_pch(uint32_t index) {
    assert(trace.valid);
    return read_nodes(index, 8, (uint32_t[]){ p6502_pch0,p6502_pch1,p6502_pch2,p6502_pch3,p6502_pch4,p6502_pch5,p6502_pch6,p6502_pch7 });
}

uint16_t trace_get_pc(uint32_t index) {
    assert(trace.valid);
    return (trace_get_pch(index)<<8)|trace_get_pcl(index);
}

uint8_t trace_get_ir(uint32_t index) {
    assert(trace.valid);
    return ~read_nodes(index, 8, (uint32_t[]){ p6502_notir0,p6502_notir1,p6502_notir2,p6502_notir3,p6502_notir4,p6502_notir5,p6502_notir6,p6502_notir7 });
}

uint8_t trace_get_p(uint32_t index) {
    assert(trace.valid);
    // missing p6502_p5 is not a typo (there is no physical bit 5 in status reg)
    return read_nodes(index, 8, (uint32_t[]){ p6502_p0,p6502_p1,p6502_p2,p6502_p3,p6502_p4,0,p6502_p6,p6502_p7 });
}

bool trace_get_rw(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_rw);
}

bool trace_get_sync(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_sync);
}

bool trace_get_clk0(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_clk0);
}

bool trace_get_irq(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_irq);
}

bool trace_get_nmi(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_nmi);
}

bool trace_get_res(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_res);
}

bool trace_get_rdy(uint32_t index) {
    assert(trace.valid);
    return is_node_high(index, p6502_rdy);
}
#endif

void trace_store(void) {
    assert(trace.valid);
    uint32_t idx = ring_add();
    trace_item_t* item = &trace.items[idx];
    item->cycle = sim_get_cycle();
    memcpy(&item->mem, cpu_memory, sizeof(item->mem));
    sim_get_node_values((range_t){ .ptr=item->node_values, .size=sizeof(item->node_values) });
    sim_get_transistor_on((range_t){ .ptr=item->transistors_on, .size=sizeof(item->transistors_on) });
    // find start of instruction (first half tick after sync)
    #if defined(CHIP_6502)
        if (!sim_get_sync() && (trace_num_items() > 1) && trace_get_sync(1)) {
            trace.flip_bits ^= TRACE_FLIPBIT_OP;
        }
        if (sim_get_clk0()) {
            trace.flip_bits |= TRACE_FLIPBIT_CLK0;
        }
        else {
            trace.flip_bits &= ~TRACE_FLIPBIT_CLK0;
        }
    #else
        // FIXME
    #endif
    item->flip_bits = trace.flip_bits;
    trace.ui.scroll_to_end = true;
}

static void load_item(trace_item_t* item) {
    trace.flip_bits = item->flip_bits;
    sim_set_cycle(item->cycle);
    memcpy(cpu_memory, &item->mem, sizeof(item->mem));
    sim_set_node_values((range_t){ item->node_values, sizeof(item->node_values) });
    sim_set_transistor_on((range_t){ item->transistors_on, sizeof(item->transistors_on) });
}

// load the previous trace item into the simulator and pop it from the trace log
bool trace_revert_to_previous(void) {
    assert(trace.valid);
    if (trace_num_items() < 1) {
        return false;
    }
    uint32_t idx = trace_to_ring_index(1);
    trace_item_t* item = &trace.items[idx];
    load_item(item);
    ring_pop();
    return true;
}

// load the selected trace item into the simulator, and drop following trace items
bool trace_revert_to_selected(void) {
    assert(trace.valid);
    if (!trace.ui.selected) {
        return false;
    }
    // find the selected item by cycle
    uint32_t idx;
    for (idx = trace.tail; idx != trace.head; idx = ring_idx(idx+1)) {
        if (trace.ui.selected_cycle == trace.items[idx].cycle) {
            break;
        }
    }
    if (idx == trace.head) {
        return false;
    }
    trace_item_t* item = &trace.items[idx];
    load_item(item);
    trace.head = ring_idx(idx+1);
    return true;
}

void trace_ui_set_selected(bool selected) {
    assert(trace.valid);
    trace.ui.selected = selected;
}

bool trace_ui_get_selected(void) {
    assert(trace.valid);
    return trace.ui.selected;
}

void trace_ui_set_selected_cycle(uint32_t cycle) {
    assert(trace.valid);
    trace.ui.selected_cycle = cycle;
}

uint32_t trace_ui_get_selected_cycle(void) {
    assert(trace.valid);
    return trace.ui.selected_cycle;
}

void trace_ui_set_scroll_to_end(bool b) {
    assert(trace.valid);
    trace.ui.scroll_to_end = b;
}

bool trace_ui_get_scroll_to_end(void) {
    assert(trace.valid);
    return trace.ui.scroll_to_end;
}
