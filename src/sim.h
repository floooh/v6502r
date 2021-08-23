#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "trace.h"
#include "gfx.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct sim_t {
    bool paused;
    void* p6502_state;
} sim_t;

void sim_init_or_reset(sim_t* sim);
void sim_shutdown(sim_t* sim);
void sim_start(sim_t* sim, trace_t* trace);
void sim_frame(sim_t* sim, trace_t* trace, gfx_t* gfx);
void sim_step(sim_t* sim, trace_t* trace, int num_half_cycles);
void sim_step_op(sim_t* sim, trace_t* trace);
void sim_pause(sim_t* sim, bool paused);
bool sim_paused(const sim_t* sim);
void sim_update_nodestate(sim_t* sim, uint8_t* buf_ptr, size_t buf_size);
void sim_w8(uint16_t addr, uint8_t val);
uint8_t sim_r8(uint16_t addr);
void sim_w16(uint16_t addr, uint16_t val);
uint16_t sim_r16(uint16_t addr);
void sim_clear(uint16_t addr, uint16_t num_bytes);
void sim_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr);
uint32_t sim_get_cycle(void);
void sim_set_cycle(uint32_t c);
uint8_t sim_get_a(const sim_t* sim);
uint8_t sim_get_x(const sim_t* sim);
uint8_t sim_get_y(const sim_t* sim);
uint8_t sim_get_sp(const sim_t* sim);
uint16_t sim_get_pc(const sim_t* sim);
uint8_t sim_get_ir(const sim_t* sim);
uint8_t sim_get_p(const sim_t* sim);
bool sim_get_clk0(const sim_t* sim);
bool sim_get_rw(const sim_t* sim);
bool sim_get_sync(const sim_t* sim);
uint16_t sim_get_addr(const sim_t* sim);
uint8_t sim_get_data(const sim_t* sim);
void sim_set_rdy(sim_t* sim, bool high);
void sim_set_irq(sim_t* sim, bool high);
void sim_set_nmi(sim_t* sim, bool high);
void sim_set_res(sim_t* sim, bool high);
bool sim_get_irq(const sim_t* sim);
bool sim_get_nmi(const sim_t* sim);
bool sim_get_res(const sim_t* sim);
bool sim_get_rdy(const sim_t* sim);
bool sim_read_node_values(const sim_t* sim, uint8_t* ptr, int max_bytes);
bool sim_read_transistor_on(const sim_t* sim, uint8_t* ptr, int max_bytes);
void sim_write_node_values(sim_t* sim, const uint8_t* ptr, int max_bytes);
void sim_write_transistor_on(sim_t* sim, const uint8_t* ptr, int max_bytes);

#if defined(__cplusplus)
} // extern "C"
#endif
