#ifndef INCLUDED_FROM_NETLIST_SIM_C
#define state_t void
#endif

state_t *setupNodesAndTransistors(netlist_transdefs *transdefs, BOOL *node_is_pullup, nodenum_t nodes, nodenum_t transistors, nodenum_t vss, nodenum_t vcc);
void destroyNodesAndTransistors(state_t *state);
void setNode(state_t *state, nodenum_t nn, BOOL s);
BOOL isNodeHigh(state_t *state, nodenum_t nn);
unsigned int readNodes(state_t *state, int count, nodenum_t *nodelist);
void writeNodes(state_t *state, int count, nodenum_t *nodelist, int v);

void recalcNodeList(state_t *state);
void stabilizeChip(state_t *state);

// Z80 specific extension (see https://stardot.org.uk/forums/viewtopic.php?p=349760#p349760)
void modelChargeSharing(state_t *state, BOOL enabled);

// 2A03 specific extension
void initializePoweredTransistors(state_t *state);

// v6502r additions
int read_node_values(state_t* state, uint8_t* ptr, int max_bytes);
int read_transistor_on(state_t* state, uint8_t* ptr, int max_bytes);
int write_node_values(state_t* state, const uint8_t* ptr, int max_bytes);
int write_transistor_on(state_t* state, const uint8_t* ptr, int max_bytes);
