#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

extern uint8_t cpu_memory[65536];
extern uint32_t cpu_cycle;

extern state_t* cpu_initAndResetChip(void);
extern void cpu_destroyChip(state_t *state);
extern void cpu_step(state_t *state);
extern uint16_t cpu_readPC(state_t *state);
extern uint16_t cpu_readAddressBus(state_t *state);
extern void cpu_writeDataBus(state_t *state, uint8_t val);
extern uint8_t cpu_readDataBus(state_t *state);
// FIXME

// v6502r additions
extern bool cpu_read_node_state_as_bytes(state_t* state, uint8_t* ptr, size_t max_nodes);
extern bool cpu_read_node_values(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_read_transistor_on(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_node_values(state_t* state, const uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_transistor_on(state_t* state, const uint8_t* ptr, size_t max_bytes);
