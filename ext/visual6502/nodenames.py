import os

MAX_NODES = 8192
NUM_NODES = 0
NODENAMES = [None for i in range(0,MAX_NODES)]

#===============================================================================
def parse_nodenames(src_dir, remap_index):
    global NUM_NODES

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
        if remap_index:
            node_index = remap_index(node_index)
        if (node_index+1) > NUM_NODES:
            NUM_NODES = node_index+1
        if (node_index != -1) and (NODENAMES[node_index] is None):
            # the .replace is a special fix for '##aluout', the '##'
            # is a special character in Dear ImGui
            NODENAMES[node_index] = str(tokens[0]).replace('##', '_#')

#-------------------------------------------------------------------------------
def write_header(dst_dir, node_prefix):
    fp = open(dst_dir + '/nodenames.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edit!\n")
    fp.write('static const int num_node_names = {};\n'.format(NUM_NODES))
    fp.write('extern const char* node_names[{}];\n\n'.format(NUM_NODES))
    fp.write('enum {\n')
    for i in range(0,NUM_NODES):
        n = NODENAMES[i]
        if n is not None and n.replace('_','').isalnum():
            fp.write('  {}_{} = {},\n'.format(node_prefix, n, i))
    fp.write('};\n')
    fp.close()

#-------------------------------------------------------------------------------
def write_source(dst_dir):
    fp = open(dst_dir + '/nodenames.c', 'w')
    fp.write("// machine generated, don't edi!\n")
    fp.write('#include "nodenames.h"\n')
    fp.write('const char* node_names[{}] = {{\n'.format(NUM_NODES))
    for i in range(0,NUM_NODES):
        n = NODENAMES[i]
        if n is None:
            fp.write('"",\n')
        else:
            fp.write('"{}",\n'.format(n))
    fp.write('};')
    fp.close()

#-------------------------------------------------------------------------------
def dump(src_dir, dst_dir, node_prefix, remap_index=None):
    parse_nodenames(src_dir, remap_index)
    write_header(dst_dir, node_prefix)
    write_source(dst_dir)

