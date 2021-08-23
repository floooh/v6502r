#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    uint32_t cycle;
    uint32_t flip_bits;     // for grouping instruction and cycle ticks
    uint32_t node_values[64];
    uint32_t transistors_on[128];
    uint8_t mem[4096];      // only trace the first few KB of memory
} trace_item_t;

typedef struct {
    uint32_t flip_bits;         // for visually separating instructions and cycles
    uint32_t head;
    uint32_t tail;
    trace_item_t items[MAX_TRACE_ITEMS];
    struct {
        bool log_scroll_to_end;
        bool hovered;               // true if mouse is currently hovering a UI trace item
        bool selected;              // true if a trace item is currently selected in the UI
        uint32_t hovered_cycle;     // mouse-hovered cycle number
        uint32_t selected_cycle;    // selected cycle number
    } ui;
} trace_t;

struct sim_t;

void trace_init(trace_t* trace);
void trace_shutdown(trace_t* trace);
void trace_clear(trace_t* trace);
void trace_store(trace_t* trace, struct sim_t* sim);
bool trace_revert_to_previous(trace_t* trace, struct sim_t* sim);
bool trace_revert_to_selected(trace_t* trace, struct sim_t* sim);
uint32_t trace_num_items(const trace_t* trace);  // number of stored trace items
bool trace_empty(const trace_t* trace);
uint32_t trace_get_cycle(const trace_t* trace, uint32_t index);
uint32_t trace_get_flipbits(const trace_t* trace, uint32_t index);
uint8_t trace_get_a(const trace_t* trace, uint32_t index);
uint8_t trace_get_x(const trace_t* trace, uint32_t index);
uint8_t trace_get_y(const trace_t* trace, uint32_t index);
uint8_t trace_get_sp(const trace_t* trace, uint32_t index);
uint16_t trace_get_pc(const trace_t* trace, uint32_t index);
uint8_t trace_get_ir(const trace_t* trace, uint32_t index);
uint8_t trace_get_p(const trace_t* trace, uint32_t index);
bool trace_get_clk0(const trace_t* trace, uint32_t index);
bool trace_get_rw(const trace_t* trace, uint32_t index);
bool trace_get_sync(const trace_t* trace, uint32_t index);
bool trace_get_irq(const trace_t* trace, uint32_t index);
bool trace_get_nmi(const trace_t* trace, uint32_t index);
bool trace_get_res(const trace_t* trace, uint32_t index);
bool trace_get_rdy(const trace_t* trace, uint32_t index);
uint16_t trace_get_addr(const trace_t* trace, uint32_t index);
uint8_t trace_get_data(const trace_t* trace, uint32_t index);

#if defined(__cplusplus)
}   // extern "C"
#endif
