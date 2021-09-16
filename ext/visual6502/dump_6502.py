import nodenames
import segdefs
import os

cur_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = cur_dir + '/m6502'
dst_dir = cur_dir + '/../../src/m6502'

print("writing nodenames...")
nodenames.dump(src_dir, dst_dir, 'p6502')
print("writing segdefs...")
segdefs.dump(src_dir, dst_dir, 1)
print("done.")
