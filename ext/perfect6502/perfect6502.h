#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

extern state_t *initAndResetChip();
extern void destroyChip(state_t *state);
extern void step(state_t *state);
extern void setRDY(state_t* state, int high);
extern void setIRQ(state_t* state, int high);
extern void setNMI(state_t* state, int high);
extern void setRES(state_t* state, int high);
extern void chipStatus(state_t *state);
extern unsigned short readPC(state_t *state);
extern unsigned char readA(state_t *state);
extern unsigned char readX(state_t *state);
extern unsigned char readY(state_t *state);
extern unsigned char readSP(state_t *state);
extern unsigned char readP(state_t *state);
extern unsigned int readRW(state_t *state);
extern unsigned int readSYNC(state_t* state);
extern unsigned int readCLK0(state_t* state);
extern unsigned short readAddressBus(state_t *state);
extern void writeDataBus(state_t *state, unsigned char);
extern unsigned char readDataBus(state_t *state);
extern unsigned char readIR(state_t *state);

extern unsigned char memory[65536];
extern unsigned int cycle;
//extern unsigned int transistors;

// v6502r additions
extern int p6502_read_node_state_as_bytes(state_t* state, uint8_t* ptr, int max_nodes);
extern int p6502_read_node_values(state_t* state, uint8_t* ptr, int max_bytes);
extern int p6502_read_transistor_on(state_t* state, uint8_t* ptr, int max_bytes);
extern int p6502_write_node_values(state_t* state, const uint8_t* ptr, int max_bytes);
extern int p6502_write_transistor_on(state_t* state, const uint8_t* ptr, int max_bytes);
