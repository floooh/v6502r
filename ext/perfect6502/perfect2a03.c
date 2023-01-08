/*
 Copyright (c) 2010,2014 Michael Steil, Brian Silverman, Barry Silverman

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "types.h"
#include "netlist_sim.h"
/* nodes & transistors */
#include "netlist_2a03.h"

// v6502r additions
size_t cpu_get_num_nodes(void) {
    return sizeof(netlist_2a03_node_is_pullup)/sizeof(*netlist_2a03_node_is_pullup);
}

bool cpu_read_node_state_as_bytes(void* state, uint8_t active_value, uint8_t inactive_value, uint8_t* ptr, size_t max_nodes) {
    const size_t num_nodes = (int) cpu_get_num_nodes();
    if (num_nodes > max_nodes) {
        return false;
    }
    for (int i = 0; i < (int)num_nodes; i++) {
        ptr[i] = (isNodeHigh(state, i) != 0) ? active_value : inactive_value;
    }
    return true;
}

bool cpu_read_node_values(void* state, uint8_t* ptr, size_t max_bytes) {
    return read_node_values(state, ptr, (int)max_bytes);
}

bool cpu_read_transistor_on(void* state, uint8_t* ptr, size_t max_bytes) {
    return read_transistor_on(state, ptr, (int)max_bytes);
}

bool cpu_write_node_values(void* state, const uint8_t* ptr, size_t max_bytes) {
    return write_node_values(state, ptr, (int)max_bytes);
}

bool cpu_write_transistor_on(void* state, const uint8_t* ptr, size_t max_bytes) {
    return write_transistor_on(state, ptr, (int)max_bytes);
}

uint32_t cpu_read_nodes(state_t *state, size_t count, uint16_t *nodelist) {
    return readNodes(state, (int)count, nodelist);
}

/************************************************************
 *
 * 2a03-specific Interfacing
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

bool cpu_readCLK0(void* state) {
    return isNodeHigh(state, clk0) != 0;
}

bool cpu_readRW(void *state) {
    return isNodeHigh(state, rw) != 0;
}

bool cpu_readSYNC(void* state) {
    return isNodeHigh(state, sync) != 0;
}

uint8_t cpu_readA(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ a0,a1,a2,a3,a4,a5,a6,a7 });
}

uint8_t cpu_readX(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ x0,x1,x2,x3,x4,x5,x6,x7 });
}

uint8_t cpu_readY(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ y0,y1,y2,y3,y4,y5,y6,y7 });
}

uint8_t cpu_readP(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ p0,p1,p2,p3,p4,0,p6,p7 });
}

uint8_t cpu_readIR(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ notir0,notir1,notir2,notir3,notir4,notir5,notir6,notir7 }) ^ 0xFF;
}

uint8_t cpu_readSP(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ s0,s1,s2,s3,s4,s5,s6,s7 });
}

uint8_t cpu_readPCL(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ pcl0,pcl1,pcl2,pcl3,pcl4,pcl5,pcl6,pcl7 });
}

uint8_t cpu_readPCH(void *state) {
    return readNodes(state, 8, (nodenum_t[]){ pch0,pch1,pch2,pch3,pch4,pch5,pch6,pch7 });
}

uint16_t cpu_readPC(void *state) {
    return (cpu_readPCH(state) << 8) | cpu_readPCL(state);
}

void cpu_writeRDY(void* state, bool high) {
    setNode(state, rdy, high);
}

bool cpu_readRDY(void* state) {
    return isNodeHigh(state, rdy) != 0;
}

void cpu_writeIRQ(void* state, bool high) {
    setNode(state, irq, high);
}

bool cpu_readIRQ(void* state) {
    return isNodeHigh(state, irq) != 0;
}

void cpu_writeNMI(void* state, bool high) {
    setNode(state, nmi, high);
}

bool cpu_readNMI(void* state) {
    return isNodeHigh(state, nmi) != 0;
}

void cpu_writeRES(void* state, bool high) {
    setNode(state, res, high);
}

bool cpu_readRES(void* state) {
    return isNodeHigh(state, res) != 0;
}

void cpu_write_node(void* state, int node_index, bool high) {
    setNode(state, (uint16_t)node_index, high ? 1 : 0);
}

bool cpu_read_node(void* state, int node_index) {
    return isNodeHigh(state, (uint16_t)node_index) != 0;
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

static void handleBusRead(void* state) {
    if (isNodeHigh(state, rw)) {
        cpu_writeDataBus(state, mRead(cpu_readAddressBus(state)));
    }
}

static void handleBusWrite(void* state) {
    if (!isNodeHigh(state, rw)) {
        mWrite(cpu_readAddressBus(state), cpu_readDataBus(state));
    }
}

/************************************************************
 *
 * Main Clock Loop
 *
 ************************************************************/

uint32_t cpu_cycle;

void cpu_step(void *state) {
    BOOL clk = isNodeHigh(state, clk0);

    do {
      setNode(state, clk_in, 1);
      setNode(state, clk_in, 0);
    } while(clk == isNodeHigh(state, clk0));

    /* handle memory reads and writes */
    if (clk) {
        handleBusRead(state);
    } else {
        handleBusWrite(state);
    }
    cpu_cycle++;
}

void* cpu_initAndResetChip(void) {
    /* set up data structures for efficient emulation */
    nodenum_t nodes = sizeof(netlist_2a03_node_is_pullup)/sizeof(*netlist_2a03_node_is_pullup);
    nodenum_t transistors = sizeof(netlist_2a03_transdefs)/sizeof(*netlist_2a03_transdefs);
    void *state = setupNodesAndTransistors(netlist_2a03_transdefs,
        netlist_2a03_node_is_pullup,
        nodes,
        transistors,
        vss,
        vcc);

    initializePoweredTransistors(state);

    setNode(state, clk_in, 0);
    for (int i = 0; i < 6; ++i) {
      setNode(state, clk_in, 1);
      setNode(state, clk_in, 0);
    }
    setNode(state, res, 0);
    setNode(state, so, 0);
    setNode(state, irq, 1);
    setNode(state, nmi, 1);
    stabilizeChip(state);

    for (int i = 0; i < 12*8; ++i) {
      setNode(state, clk_in, 1);
      setNode(state, clk_in, 0);
    }
    setNode(state, res, 1);
    recalcNodeList(state);

    cpu_cycle = 0;

    return state;
}

void cpu_destroyChip(void *state) {
    destroyNodesAndTransistors(state);
}
