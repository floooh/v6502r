#pragma once
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    const char* node_name;
    int node_index;
} sim_named_node_t;

typedef struct {
    const sim_named_node_t* ptr;
    int num;
} sim_named_node_range_t;

typedef struct {
    const char* group_name;
    int num_nodes;
    uint32_t* nodes;
} sim_nodegroup_t;

typedef struct {
    int num;
    const sim_nodegroup_t* ptr;
} sim_nodegroup_range_t;

void sim_init(void);
void sim_shutdown(void);
void sim_start(void);
void sim_frame(void);
void sim_step(int num_half_cycles);
void sim_step_op(void);
void sim_set_paused(bool paused);
bool sim_get_paused(void);
void sim_set_nodestate(range_t from_buffer);
sim_named_node_range_t sim_get_sorted_nodes(void);
sim_nodegroup_range_t sim_get_nodegroups(void);
int sim_find_node(const char* name);    // accepts name or '#1234', return -1 if not found
const char* sim_get_node_name_by_index(int node_index);
void sim_mem_w8(uint16_t addr, uint8_t val);
uint8_t sim_mem_r8(uint16_t addr);
void sim_mem_w16(uint16_t addr, uint16_t val);
uint16_t sim_mem_r16(uint16_t addr);
void sim_mem_clear(uint16_t addr, uint16_t num_bytes);
void sim_mem_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr);
uint32_t sim_get_cycle(void);
void sim_set_cycle(uint32_t c);
uint16_t sim_get_addr(void);
uint8_t sim_get_data(void);
bool sim_write_node_visual_state(range_t to_buffer);
bool sim_read_node_values(range_t from_buffer);
bool sim_write_node_values(range_t to_buffer);
bool sim_read_transistor_on(range_t from_buffer);
bool sim_write_transistor_on(range_t to_buffer);
bool sim_is_ignore_picking_highlight_node(int node_index);
uint16_t sim_get_pc(void);
uint8_t sim_get_flags(void);
int sim_get_num_nodes(void);
bool sim_get_node_state(int node_index);
void sim_set_node_state(int node_index, bool high);

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
void sim_io_w8(uint16_t addr, uint8_t val);
uint8_t sim_io_r8(uint16_t addr);
void sim_io_w16(uint16_t addr, uint16_t val);
uint16_t sim_io_r16(uint16_t addr);
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
uint8_t sim_z80_get_f(void);
uint8_t sim_z80_get_i(void);
uint8_t sim_z80_get_r(void);
uint8_t sim_z80_get_ir(void);
bool sim_z80_get_m1(void);
bool sim_z80_get_clk(void);
bool sim_z80_get_mreq(void);
bool sim_z80_get_iorq(void);
bool sim_z80_get_rd(void);
bool sim_z80_get_wr(void);
void sim_z80_set_wait(bool high);
void sim_z80_set_int(bool high);
void sim_z80_set_nmi(bool high);
void sim_z80_set_reset(bool high);
void sim_z80_set_busrq(bool high);
bool sim_z80_get_halt(void);
bool sim_z80_get_wait(void);
bool sim_z80_get_rfsh(void);
bool sim_z80_get_int(void);
bool sim_z80_get_nmi(void);
bool sim_z80_get_reset(void);
bool sim_z80_get_busrq(void);
bool sim_z80_get_busak(void);
bool sim_z80_get_iff1(void);

uint8_t sim_z80_get_m(void);
uint8_t sim_z80_get_t(void);

void sim_z80_set_intvec(uint8_t val);
uint8_t sim_z80_get_intvec(void);
#endif

#if defined(__cplusplus)
} // extern "C"
#endif
