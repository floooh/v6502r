#===============================================================================
#   netlist.py
#
#   Write perfect6502 netlist header.
#===============================================================================

import os

MAX_NODES = 4096

NUM_NODEPULLUP = 0

NODEPULLUP = [0 for i in range(0, MAX_NODES)]
TRANSDEFS  = []

# read segdefs.js and extract the node pullup state
def parse_nodepullup(src_dir):
    global NUM_NODEPULLUP
    fp = open(src_dir + '/segdefs.js', 'r')
    lines = fp.readlines()
    fp.close()
    for line in lines:
        if not line.startswith('['):
            continue
        tokens = line.lstrip('[ ').rstrip('],\n\r').split(',')
        node_index = int(tokens[0])
        if (node_index + 1) > NUM_NODEPULLUP:
            NUM_NODEPULLUP = node_index + 1
        pullup = tokens[1]
        if pullup == "'+'":
            NODEPULLUP[node_index] = 1

# read transdefs.js and extract the transistor triples
def parse_transdefs(src_dir):
    fp = open(src_dir + '/transdefs.js', 'r')
    lines = fp.readlines()
    fp.close()
    for line in lines:
        if not line.startswith('['):
            continue
        tokens = line.lstrip('[').rstrip('],\n\r').split(',')
        if tokens[-1] == 'true':
            print("..ignoring weak transistor {}".format(tokens[0]))
        gate = tokens[1]
        c1 = tokens[2]
        c2 = tokens[3]
        TRANSDEFS.append([gate, c1, c2])

# write the perfect6502 netlist header
def write_header(dst_path, cpu_name, nodenames):
    fp = open(dst_path, 'w')
    fp.write("// machine generated, don't exit\n")
    fp.write('#include "types.h"\n')
    fp.write("enum {\n")
    for i,nodename in enumerate(nodenames):
        if nodename is not None:
            fp.write("  {} = {},\n".format(nodename.strip('"'), i))
    fp.write("};\n\n");
    fp.write("BOOL netlist_{}_node_is_pullup[{}] = {{".format(cpu_name, NUM_NODEPULLUP))
    for i in range(0, NUM_NODEPULLUP):
        if (i % 32) == 0:
            fp.write("\n  ")
        fp.write("{},".format(NODEPULLUP[i]))
    fp.write("};\n\n")
    fp.write("netlist_transdefs netlist_{}_transdefs[{}] = {{\n".format(cpu_name, len(TRANSDEFS)))
    for t in TRANSDEFS:
        fp.write("  {{ {},{},{} }},\n".format(t[0], t[1], t[2]))
    fp.write("};\n")

#-------------------------------------------------------------------------------
def dump(src_dir, dst_path, cpu_name, nodenames):
    parse_nodepullup(src_dir)
    parse_transdefs(src_dir)
    write_header(dst_path, cpu_name, nodenames)





