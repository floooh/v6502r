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

uint16_t cpu_readAddressBus(void *state) {
    return readNodes(state, 16, (nodenum_t[]){ ab0, ab1, ab2, ab3, ab4, ab5, ab6, ab7, ab8, ab9, ab10, ab11, ab12, ab13, ab14, ab15 });
}

uint8_t cpu_readDataBus(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ db0, db1, db2, db3, db4, db5, db6, db7 });
}

void cpu_writeDataBus(void *state, uint8_t d) {
    writeNodes(state, 8, (nodenum_t[]){ db0, db1, db2, db3, db4, db5, db6, db7 }, d);
}


/************************************************************
 *
 * Address Bus and Data Bus Interface
 *
 ************************************************************/

uint8_t cpu_memory[65536];

static uint8_t mRead(uint16_t a)
{
    return cpu_memory[a];
}

static void mWrite(uint16_t a, uint8_t d)
{
    cpu_memory[a] = d;
}

static inline void handleBus(void *state) {
    if (!isNodeHigh(state, _mreq)) {
        if (!isNodeHigh(state, _rd)) {
            cpu_writeDataBus(state, mRead(cpu_readAddressBus(state)));
        }
        if (!isNodeHigh(state, _wr)) {
            mWrite(cpu_readAddressBus(state), cpu_readDataBus(state));
        }
    }
    // FIXME: IO?
}

/************************************************************
 *
 * Main Clock Loop
 *
 ************************************************************/

uint32_t cpu_cycle;

void cpu_step(void *state) {
    BOOL clock = isNodeHigh(state, clk);

    // invert clock
    setNode(state, clk, !clock);
    recalcNodeList(state);

    // handle memory reads and writes
    if (!clock) {
        handleBus(state);
    }
    cpu_cycle++;
}

void* cpu_initAndResetChip(void) {
    nodenum_t nodes = sizeof(netlist_z80_node_is_pullup)/sizeof(*netlist_z80_node_is_pullup);
    nodenum_t transistors = sizeof(netlist_z80_transdefs)/sizeof(*netlist_z80_transdefs);
    void *state = setupNodesAndTransistors(netlist_z80_transdefs,
        netlist_z80_node_is_pullup,
        nodes,
        transistors,
        vss,
        vcc);

    setNode(state, _reset, 0);
    setNode(state, clk, 1);
    setNode(state, _busrq, 1);
    setNode(state, _int, 1);
    setNode(state, _nmi, 1);
    setNode(state, _wait, 1);

    stabilizeChip(state);

    for (int i = 0; i < 31; i++) {
        cpu_step(state);
    }
    setNode(state, _reset, 1);
    recalcNodeList(state);

    cpu_cycle = 0;
    return state;
}

void cpu_destroyChip(void *state) {
    destroyNodesAndTransistors(state);
}
