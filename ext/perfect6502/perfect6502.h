#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

extern uint8_t cpu_memory[65536];
extern uint32_t cpu_cycle;

extern state_t *cpu_initAndResetChip(void);
extern void cpu_destroyChip(state_t *state);
extern void cpu_step(state_t *state);
extern uint16_t cpu_readAddressBus(state_t *state);
extern void cpu_writeDataBus(state_t *state, uint8_t val);
extern uint8_t cpu_readDataBus(state_t *state);
extern uint16_t cpu_readPC(state_t *state);
extern uint8_t cpu_readA(state_t *state);
extern uint8_t cpu_readX(state_t *state);
extern uint8_t cpu_readY(state_t *state);
extern uint8_t cpu_readSP(state_t *state);
extern uint8_t cpu_readP(state_t *state);
extern uint8_t cpu_readIR(state_t *state);
extern bool cpu_readRW(state_t *state);
extern bool cpu_readSYNC(state_t* state);
extern bool cpu_readCLK0(state_t* state);
extern void cpu_writeRDY(state_t* state, bool high);
extern void cpu_writeIRQ(state_t* state, bool high);
extern void cpu_writeNMI(state_t* state, bool high);
extern void cpu_writeRES(state_t* state, bool high);
extern bool cpu_readRDY(state_t* state);
extern bool cpu_readIRQ(state_t* state);
extern bool cpu_readNMI(state_t* state);
extern bool cpu_readRES(state_t* state);

// v6502r additions
extern size_t cpu_get_num_nodes(void);
extern void cpu_write_node(state_t* state, int node_index, bool high);
extern bool cpu_read_node(state_t* state, int node_index);
extern bool cpu_read_node_state_as_bytes(state_t* state, uint8_t active_value, uint8_t inactive_value, uint8_t* ptr, size_t max_nodes);
extern bool cpu_read_node_values(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_read_transistor_on(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_node_values(state_t* state, const uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_transistor_on(state_t* state, const uint8_t* ptr, size_t max_bytes);
