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

extern bool cpu_readM1(state_t* state);
extern bool cpu_readCLK(state_t* state);

extern uint8_t cpu_readA(state_t* state);
extern uint8_t cpu_readF(state_t* state);
extern uint8_t cpu_readB(state_t* state);
extern uint8_t cpu_readC(state_t* state);
extern uint8_t cpu_readD(state_t* state);
extern uint8_t cpu_readE(state_t* state);
extern uint8_t cpu_readH(state_t* state);
extern uint8_t cpu_readL(state_t* state);

extern uint8_t cpu_readA2(state_t* state);
extern uint8_t cpu_readF2(state_t* state);
extern uint8_t cpu_readB2(state_t* state);
extern uint8_t cpu_readC2(state_t* state);
extern uint8_t cpu_readD2(state_t* state);
extern uint8_t cpu_readE2(state_t* state);
extern uint8_t cpu_readH2(state_t* state);
extern uint8_t cpu_readL2(state_t* state);

extern uint8_t cpu_readI(state_t* state);
extern uint8_t cpu_readR(state_t* state);
extern uint8_t cpu_readW(state_t* state);
extern uint8_t cpu_readZ(state_t* state);

extern uint16_t cpu_readIX(state_t* state);
extern uint16_t cpu_readIY(state_t* state);
extern uint16_t cpu_readSP(state_t* state);
extern uint16_t cpu_readPC(state_t* state);

// v6502r additions
extern bool cpu_read_node_state_as_bytes(state_t* state, uint8_t* ptr, size_t max_nodes);
extern bool cpu_read_node_values(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_read_transistor_on(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_node_values(state_t* state, const uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_transistor_on(state_t* state, const uint8_t* ptr, size_t max_bytes);
