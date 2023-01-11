import netlist
import nodenames
import segdefs
import os

FIRST_6502_NODE   = 0
NUM_6502_NODES    = 1725
LAST_6502_NODE    = FIRST_6502_NODE + NUM_6502_NODES
FIRST_2A03_NODE   = 10000
NUM_2A03_NODES    = 5815
LAST_2A03_NODE    = FIRST_2A03_NODE + NUM_2A03_NODES
FIRST_EXTRA_NODE  = 20000

UNUSED_2A03_NODES = FIRST_EXTRA_NODE - LAST_2A03_NODE
UNUSED_6502_NODES = FIRST_2A03_NODE - LAST_6502_NODE

cur_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = cur_dir + '/2a03'
dst_dir = cur_dir + '/../../src/2a03'

indexes = set()

# Remap to remove unused nodes
def remap_index(index):
    indexes.add(index)
    if index >= FIRST_EXTRA_NODE:
        return index - UNUSED_2A03_NODES - UNUSED_6502_NODES
    if index >= FIRST_2A03_NODE:
        return index - UNUSED_6502_NODES
    return index

print("writing nodenames...")
nodenames.dump(src_dir, dst_dir, 'p2a03', remap_index)
print("writing netlist...")
netlist.dump(src_dir, cur_dir + '/../perfect6502/netlist_2a03.h', '2a03', nodenames.NODENAMES, remap_index)
print("writing segdefs...")
segdefs.dump(src_dir, dst_dir, 1, remap_index)
print("done.")
