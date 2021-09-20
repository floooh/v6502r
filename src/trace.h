#pragma once
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void trace_init(void);
void trace_shutdown(void);
void trace_clear(void);
void trace_store(void);
bool trace_revert_to_previous(void);
bool trace_revert_to_selected(void);
uint32_t trace_num_items(void);
bool trace_empty(void);
uint32_t trace_get_cycle(uint32_t index);
uint32_t trace_get_flipbits(uint32_t index);
uint16_t trace_get_addr(uint32_t index);
uint8_t trace_get_data(uint32_t index);
uint16_t trace_get_pc(uint32_t index);
uint8_t trace_get_flags(uint32_t index);
#if defined(CHIP_6502)
uint8_t trace_6502_get_a(uint32_t index);
uint8_t trace_6502_get_x(uint32_t index);
uint8_t trace_6502_get_y(uint32_t index);
uint8_t trace_6502_get_sp(uint32_t index);
uint8_t trace_6502_get_ir(uint32_t index);
bool trace_6502_get_clk0(uint32_t index);
bool trace_6502_get_rw(uint32_t index);
bool trace_6502_get_sync(uint32_t index);
bool trace_6502_get_irq(uint32_t index);
bool trace_6502_get_nmi(uint32_t index);
bool trace_6502_get_res(uint32_t index);
bool trace_6502_get_rdy(uint32_t index);
#else
// FIXME
#endif

void trace_ui_set_selected(bool selected);
bool trace_ui_get_selected(void);
void trace_ui_set_selected_cycle(uint32_t cycle);
uint32_t trace_ui_get_selected_cycle(void);
void trace_ui_set_scroll_to_end(bool b);
bool trace_ui_get_scroll_to_end(void);

#if defined(__cplusplus)
}   // extern "C"
#endif
