#include "types.h"
#include "netlist_sim.h"
#include "netlist_z80.h"

// v6502r additions
bool cpu_read_node_state_as_bytes(void* state, uint8_t* ptr, size_t max_nodes) {
    size_t num_nodes = sizeof(netlist_z80_node_is_pullup)/sizeof(*netlist_z80_node_is_pullup);
    if (num_nodes > max_nodes) {
        return false;
    }
    for (size_t i = 0; i < num_nodes; i++) {
        ptr[i] = (isNodeHigh(state, i) != 0) ? 160 : 48;
    }
    return true;
}

bool cpu_read_node_values(void* state, uint8_t* ptr, size_t max_bytes) {
    return read_node_values(state, ptr, max_bytes);
}

bool cpu_read_transistor_on(void* state, uint8_t* ptr, size_t max_bytes) {
    return read_transistor_on(state, ptr, max_bytes);
}

bool cpu_write_node_values(void* state, const uint8_t* ptr, size_t max_bytes) {
    return write_node_values(state, ptr, max_bytes);
}

bool cpu_write_transistor_on(void* state, const uint8_t* ptr, size_t max_bytes) {
    return write_transistor_on(state, ptr, max_bytes);
}

/************************************************************
 *
 * Z80-specific Interfacing
 *
 ************************************************************/

// FIXME

/************************************************************
 *
 * Address Bus and Data Bus Interface
 *
 ************************************************************/

uint8_t cpu_memory[65536];

// FIXME

/************************************************************
 *
 * Main Clock Loop
 *
 ************************************************************/

uint32_t cpu_cycle;

void cpu_step(void *state) {
    /* FIXME
    bool clk = isNodeHigh(state, clk0) != 0;

    // invert clock
    setNode(state, clk0, !clk);
    recalcNodeList(state);

    / handle memory reads and writes
    if (!clk) {
        handleMemory(state);
    }
    */
    cpu_cycle++;
}

void* cpu_initAndResetChip(void) {
    // FIXME
    nodenum_t nodes = sizeof(netlist_z80_node_is_pullup)/sizeof(*netlist_z80_node_is_pullup);
    nodenum_t transistors = sizeof(netlist_z80_transdefs)/sizeof(*netlist_z80_transdefs);
    void *state = setupNodesAndTransistors(netlist_z80_transdefs,
        netlist_z80_node_is_pullup,
        nodes,
        transistors,
        vss,
        vcc);

    cpu_cycle = 0;
    return state;
}

void cpu_destroyChip(void *state) {
    destroyNodesAndTransistors(state);
}
