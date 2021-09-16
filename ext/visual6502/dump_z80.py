import nodenames
import segdefs
import netlist
import os

cur_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = cur_dir + '/z80'
dst_dir = cur_dir + '/../../src/z80'

print("writing nodenames...")
nodenames.dump(src_dir, dst_dir, 'pz80')
print("writing netlist...")
netlist.dump(src_dir, cur_dir + '/../perfect6502/netlist_z80.h', 'z80', nodenames.NODENAMES)
print("writing segdefs...")
segdefs.dump(src_dir, dst_dir, 2)
print("done.")

