#===============================================================================
#   dump_nodenames.py
#
#   Dump the nodename definitions from nodenames.js into a C header/src pair.
#
#===============================================================================

import os

cur_dir = os.path.dirname(os.path.abspath(__file__))

MAX_NODES = 2048
NODENAMES = [None for i in range(0,MAX_NODES)]

#===============================================================================
def parse_nodenames():
    fp = open(cur_dir + '/nodenames.js', 'r')
    lines = fp.readlines()
    fp.close()

    for i,line in enumerate(lines):
        if (i < 23) or (i >= 940):
            continue
        if line[0] in [' ','\t','\n','\r','/']:
            continue
        tokens = line.split(',')[0].split(':')
        tokens[0] = tokens[0].strip(' \"')
        tokens[1] = tokens[1].strip(' \"')
        node_index = int(tokens[1])
        if (node_index != -1) and (NODENAMES[node_index] is None):
            NODENAMES[node_index] = str(tokens[0])

#-------------------------------------------------------------------------------
def write_header():
    fp = open(cur_dir + '/../../src/nodenames.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edi!\n")
    fp.write('static const int max_node_names = {};\n'.format(MAX_NODES))
    fp.write('extern const char* node_names[{}];'.format(MAX_NODES))
    fp.close()

#-------------------------------------------------------------------------------
def write_source():
    fp = open(cur_dir + '/../../src/nodenames.c', 'w')
    fp.write("// machine generated, don't edi!\n")
    fp.write('#include "nodenames.h"\n')
    fp.write('const char* node_names[{}] = {{\n'.format(MAX_NODES))
    for n in NODENAMES:
        if n is None:
            fp.write('"",\n')
        else:
            fp.write('"{}",\n'.format(n))
    fp.write('};')
    fp.close()


#-------------------------------------------------------------------------------
#   main
#
parse_nodenames()
write_header()
write_source()

