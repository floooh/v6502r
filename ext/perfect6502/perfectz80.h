#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

extern uint8_t cpu_memory[0x10000];
extern uint8_t cpu_io[0x10000];
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
extern bool cpu_readMREQ(state_t* state);
extern bool cpu_readIORQ(state_t* state);
extern bool cpu_readRD(state_t* state);
extern bool cpu_readWR(state_t* state);

extern bool cpu_readHALT(state_t* state);
extern bool cpu_readWAIT(state_t* state);
extern bool cpu_readRFSH(state_t* state);
extern bool cpu_readINT(state_t* state);
extern bool cpu_readNMI(state_t* state);
extern bool cpu_readRESET(state_t* state);
extern bool cpu_readBUSRQ(state_t* state);
extern bool cpu_readBUSAK(state_t* state);

extern void cpu_writeWAIT(state_t* state, bool high);
extern void cpu_writeINT(state_t* state, bool high);
extern void cpu_writeNMI(state_t* state, bool high);
extern void cpu_writeRESET(state_t* state, bool high);
extern void cpu_writeBUSRQ(state_t* state, bool high);

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
extern uint8_t cpu_readIR(state_t* state);

extern uint16_t cpu_readIX(state_t* state);
extern uint16_t cpu_readIY(state_t* state);
extern uint16_t cpu_readSP(state_t* state);
extern uint16_t cpu_readPC(state_t* state);

extern uint8_t cpu_readM(state_t* state);
extern uint8_t cpu_readT(state_t* state);

extern void cpu_setIntVec(uint8_t val);
extern uint8_t cpu_getIntVec(void);

// v6502r additions
extern size_t cpu_get_num_nodes(void);
extern void cpu_write_node(state_t* state, int node_index, bool high);
extern bool cpu_read_node(state_t* state, int node_index);
extern bool cpu_read_node_state_as_bytes(state_t* state, uint8_t active_value, uint8_t inactive_value, uint8_t* ptr, size_t max_nodes);
extern bool cpu_read_node_values(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_read_transistor_on(state_t* state, uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_node_values(state_t* state, const uint8_t* ptr, size_t max_bytes);
extern bool cpu_write_transistor_on(state_t* state, const uint8_t* ptr, size_t max_bytes);
