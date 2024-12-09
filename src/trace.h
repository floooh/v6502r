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
bool trace_revert_to_cycle(uint32_t cycle);
bool trace_write_diff_visual_state(uint32_t cycle0, uint32_t cycle1, range_t to_buf);
int trace_num_items(void);
bool trace_empty(void);
uint32_t trace_get_cycle(uint32_t index);
bool trace_is_node_high(uint32_t index, uint32_t node_index);
uint32_t trace_get_flipbits(uint32_t index);
uint16_t trace_get_addr(uint32_t index);
uint8_t trace_get_data(uint32_t index);
uint16_t trace_get_pc(uint32_t index);
uint8_t trace_get_flags(uint32_t index);
const char* trace_get_disasm(uint32_t index);
#if defined(CHIP_6502)
uint8_t trace_6502_get_a(uint32_t index);
uint8_t trace_6502_get_x(uint32_t index);
uint8_t trace_6502_get_y(uint32_t index);
uint8_t trace_6502_get_sp(uint32_t index);
uint8_t trace_6502_get_op(uint32_t index);
bool trace_6502_get_clk0(uint32_t index);
bool trace_6502_get_rw(uint32_t index);
bool trace_6502_get_sync(uint32_t index);
bool trace_6502_get_irq(uint32_t index);
bool trace_6502_get_nmi(uint32_t index);
bool trace_6502_get_res(uint32_t index);
bool trace_6502_get_rdy(uint32_t index);
#elif defined(CHIP_Z80)
bool trace_z80_get_clk(uint32_t index);
bool trace_z80_get_m1(uint32_t index);
bool trace_z80_get_mreq(uint32_t index);
bool trace_z80_get_ioreq(uint32_t index);
bool trace_z80_get_rfsh(uint32_t index);
bool trace_z80_get_rd(uint32_t index);
bool trace_z80_get_wr(uint32_t index);
bool trace_z80_get_int(uint32_t index);
bool trace_z80_get_nmi(uint32_t index);
bool trace_z80_get_wait(uint32_t index);
bool trace_z80_get_halt(uint32_t index);
bool trace_z80_get_iff1(uint32_t index);
uint8_t trace_z80_get_im(uint32_t index);
uint8_t trace_z80_get_op(uint32_t index);
uint8_t trace_z80_get_a(uint32_t index);
uint8_t trace_z80_get_a2(uint32_t index);
uint8_t trace_z80_get_f(uint32_t index);
uint8_t trace_z80_get_f2(uint32_t index);
uint8_t trace_z80_get_b(uint32_t index);
uint8_t trace_z80_get_b2(uint32_t index);
uint8_t trace_z80_get_c(uint32_t index);
uint8_t trace_z80_get_c2(uint32_t index);
uint8_t trace_z80_get_d(uint32_t index);
uint8_t trace_z80_get_d2(uint32_t index);
uint8_t trace_z80_get_e(uint32_t index);
uint8_t trace_z80_get_e2(uint32_t index);
uint8_t trace_z80_get_h(uint32_t index);
uint8_t trace_z80_get_h2(uint32_t index);
uint8_t trace_z80_get_l(uint32_t index);
uint8_t trace_z80_get_l2(uint32_t index);
uint8_t trace_z80_get_i(uint32_t index);
uint8_t trace_z80_get_r(uint32_t index);
uint8_t trace_z80_get_w(uint32_t index);
uint8_t trace_z80_get_z(uint32_t index);
uint8_t trace_z80_get_mcycle(uint32_t index);
uint8_t trace_z80_get_tstate(uint32_t index);
uint16_t trace_z80_get_af(uint32_t index);
uint16_t trace_z80_get_af2(uint32_t index);
uint16_t trace_z80_get_bc(uint32_t index);
uint16_t trace_z80_get_bc2(uint32_t index);
uint16_t trace_z80_get_de(uint32_t index);
uint16_t trace_z80_get_de2(uint32_t index);
uint16_t trace_z80_get_hl(uint32_t index);
uint16_t trace_z80_get_hl2(uint32_t index);
uint16_t trace_z80_get_ix(uint32_t index);
uint16_t trace_z80_get_iy(uint32_t index);
uint16_t trace_z80_get_sp(uint32_t index);
uint16_t trace_z80_get_wz(uint32_t index);
#elif defined(CHIP_2A03)
uint8_t trace_2a03_get_a(uint32_t index);
uint8_t trace_2a03_get_x(uint32_t index);
uint8_t trace_2a03_get_y(uint32_t index);
uint8_t trace_2a03_get_sp(uint32_t index);
uint8_t trace_2a03_get_op(uint32_t index);
bool trace_2a03_get_clk0(uint32_t index);
bool trace_2a03_get_rw(uint32_t index);
bool trace_2a03_get_sync(uint32_t index);
bool trace_2a03_get_irq(uint32_t index);
bool trace_2a03_get_nmi(uint32_t index);
bool trace_2a03_get_res(uint32_t index);
bool trace_2a03_get_rdy(uint32_t index);
uint8_t trace_2a03_get_sq0_out(uint32_t index);
uint8_t trace_2a03_get_sq1_out(uint32_t index);
uint8_t trace_2a03_get_tri_out(uint32_t index);
uint8_t trace_2a03_get_noi_out(uint32_t index);
uint8_t trace_2a03_get_pcm_out(uint32_t index);
#endif

void trace_ui_set_scroll_to_end(bool b);
bool trace_ui_get_scroll_to_end(void);

#if defined(__cplusplus)
}   // extern "C"
#endif
