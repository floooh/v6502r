import os

MAX_NODES = 4096
NODENAMES = [None for i in range(0,MAX_NODES)]

#===============================================================================
def parse_nodenames(src_dir):
    fp = open(src_dir + '/nodenames.js', 'r')
    lines = fp.readlines()
    fp.close()

    for i,line in enumerate(lines):
        if line[0] in [' ','\t','\n','\r','/','*']:
            continue
        tokens = line.split(',')[0].split(':')
        if (len(tokens) != 2):
            continue
        tokens[0] = tokens[0].strip(' \"')
        tokens[1] = tokens[1].strip(' \"')
        node_index = int(tokens[1])
        if (node_index != -1) and (NODENAMES[node_index] is None):
            NODENAMES[node_index] = str(tokens[0])

#-------------------------------------------------------------------------------
def write_header(dst_dir):
    fp = open(dst_dir + '/nodenames.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edi!\n")
    fp.write('static const int max_node_names = {};\n'.format(MAX_NODES))
    fp.write('extern const char* node_names[{}];'.format(MAX_NODES))
    fp.close()

#-------------------------------------------------------------------------------
def write_source(dst_dir):
    fp = open(dst_dir + '/nodenames.c', 'w')
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
def dump(src_dir, dst_dir):
    parse_nodenames(src_dir)
    write_header(dst_dir)
    write_source(dst_dir)

