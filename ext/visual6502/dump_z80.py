import nodenames
import segdefs
import os

cur_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = cur_dir + '/z80'
dst_dir = cur_dir + '/../../src/z80'

print("writing nodenames...")
nodenames.dump(src_dir, dst_dir)
print("writing segdefs...")
segdefs.dump(src_dir, dst_dir)
print("done.")

