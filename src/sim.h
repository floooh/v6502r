#pragma once
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void sim_init(void);
void sim_shutdown(void);
void sim_start(void);
void sim_frame(void);
void sim_step(int num_half_cycles);
void sim_step_op(void);
void sim_set_paused(bool paused);
bool sim_get_paused(void);
void sim_set_nodestate(range_t from_buffer);
void sim_w8(uint16_t addr, uint8_t val);
uint8_t sim_r8(uint16_t addr);
void sim_w16(uint16_t addr, uint16_t val);
uint16_t sim_r16(uint16_t addr);
void sim_clear(uint16_t addr, uint16_t num_bytes);
void sim_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr);
uint32_t sim_get_cycle(void);
void sim_set_cycle(uint32_t c);
uint16_t sim_get_addr(void);
uint8_t sim_get_data(void);
bool sim_set_node_values(range_t from_buffer);
bool sim_get_node_state(range_t to_buffer);
bool sim_get_node_values(range_t to_buffer);
bool sim_set_transistor_on(range_t from_buffer);
bool sim_get_transistor_on(range_t to_buffer);
bool sim_is_ignore_picking_highlight_node(int node_num);
uint16_t sim_get_pc(void);
uint8_t sim_get_flags(void);

#if defined(CHIP_6502)
uint8_t sim_6502_get_a(void);
uint8_t sim_6502_get_x(void);
uint8_t sim_6502_get_y(void);
uint8_t sim_6502_get_sp(void);
uint8_t sim_6502_get_ir(void);
bool sim_6502_get_clk0(void);
bool sim_6502_get_rw(void);
bool sim_6502_get_sync(void);
void sim_6502_set_rdy(bool high);
void sim_6502_set_irq(bool high);
void sim_6502_set_nmi(bool high);
void sim_6502_set_res(bool high);
bool sim_6502_get_irq(void);
bool sim_6502_get_nmi(void);
bool sim_6502_get_res(void);
bool sim_6502_get_rdy(void);
#endif

#if defined(CHIP_Z80)
uint16_t sim_z80_get_af(void);
uint16_t sim_z80_get_bc(void);
uint16_t sim_z80_get_de(void);
uint16_t sim_z80_get_hl(void);
uint16_t sim_z80_get_af2(void);
uint16_t sim_z80_get_bc2(void);
uint16_t sim_z80_get_de2(void);
uint16_t sim_z80_get_hl2(void);
uint16_t sim_z80_get_ix(void);
uint16_t sim_z80_get_iy(void);
uint16_t sim_z80_get_sp(void);
uint16_t sim_z80_get_pc(void);
uint16_t sim_z80_get_wz(void);
uint8_t sim_z80_get_i(void);
uint8_t sim_z80_get_r(void);
bool sim_z80_get_m1(void);
bool sim_z80_get_clk(void);
#endif

#if defined(__cplusplus)
} // extern "C"
#endif
