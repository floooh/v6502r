#===============================================================================
#   dump_segdefs.py
#
#   Dump the segment definitions from segdefs.js into a C header.
#
#   The items in a segdefs line are:
#
#   0:      node index
#   1:      pullup state
#   2:      layer
#   3..:    polygon outline vertices as x,y pairs
#===============================================================================

import os

cur_dir = os.path.dirname(os.path.abspath(__file__))

# this is the original extracted data
VERTICES = []   # each vertex is a [x,y] pair

# segments (== polygons) bucketed into layers, each segment is a triple
# of [node_index, start_vertex_index, num_vertices]
MAX_LAYERS = 6
SEGMENTS = [[] for i in range(0, MAX_LAYERS)]

# sparse array of nodes (== collection of related segments)
# the dictionary's key is the node index, and the value is an 
# array of [layer, start_segment, num_segments] triples
MAX_NODES = 1800
NODES = [[0,0,0] for i in range(0, MAX_NODES)]

# one vertex buffer per layer
VERTEXBUFFERS = [[] for i in range(0, MAX_LAYERS)]

#-------------------------------------------------------------------------------
# read the segdefs.js file and extract vertex-, segment- and node-data
# into VERTICES, SEGMENTS and NODES
#
def parse_segdef():
    fp = open(cur_dir + '/segdefs.js', 'r')
    lines = fp.readlines()
    fp.close
    for line in lines:
        if not line.startswith('['):
            continue
        tokens = line.lstrip('[ ').rstrip('],\n\r').split(',')
        node_index = int(tokens[0])
        pullup = tokens[1]
        layer = int(tokens[2])
        verts = tokens[3:]
        SEGMENTS[layer].append([node_index, len(VERTICES), len(verts)])
        if NODES[node_index][2] == 0:
            NODES[node_index] = [layer, len(SEGMENTS[layer]), 1]
        else:
            NODES[node_index][2] += 1
        VERTICES.extend(verts)

#-------------------------------------------------------------------------------
# Generate a vertex buffer per segment with polygon outlines
#
def gen_vertex_buffers():
    for l,layer in enumerate(SEGMENTS):
        for seg in layer:
            assert (seg[2] > 1)
            vi = seg[1]
            x0 = VERTICES[vi]
            y0 = VERTICES[vi+1]
            VERTEXBUFFERS[l].extend([x0,y0])
            for vi in range(seg[1]+2, seg[1]+seg[2], 2):
                x = VERTICES[vi]
                y = VERTICES[vi+1]
                VERTEXBUFFERS[l].extend([x,y])
                VERTEXBUFFERS[l].extend([x,y])
            VERTEXBUFFERS[l].extend([x0,y0])

#-------------------------------------------------------------------------------
def write_header():
    fp = open(cur_dir + '/../../src/segdefs.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edit!\n");
    fp.write('#include <stdint.h>\n')
    for i,vb in enumerate(VERTEXBUFFERS):
        fp.write('extern uint16_t seg_vertices_{}[{}];\n'.format(i,len(vb)))
    fp.close()

#-------------------------------------------------------------------------------
def write_source():
    fp = open(cur_dir + '/../../src/segdefs.c', 'w')
    fp.write("// machine generated, don't edit!\n")
    fp.write('#include "segdefs.h"\n')
    for ivb,vb in enumerate(VERTEXBUFFERS):
        fp.write('uint16_t seg_vertices_{}[{}] = {{\n'.format(ivb,len(vb)))
        for iv,v in enumerate(vb):
            fp.write('{},'.format(v))
            if 0 == ((iv+1) % 16):
                fp.write('\n')
        fp.write('};\n')
    fp.close()

#-------------------------------------------------------------------------------
#   main
#
parse_segdef()
gen_vertex_buffers()
write_header()
write_source()

    
# # write .c file
# out_file = open(cur_dir + '/../../src/segdefs.c', 'w')
# # copy the original copyright notice
# for i in range(0, 10):
#     out_file.write(in_lines[i])
# out_file.write('// vertex data: x,y, ...\n')
# out_file.write('int seg_vertices_num = {};\n'.format(len(VERTICES)))
# out_file.write('int seg_vertices[{}] = {{\n'.format(len(VERTICES)))
# for i,v in enumerate(VERTICES):
#     out_file.write('{},'.format(v))
#     if 0 == ((i+1) % 16):
#         out_file.write('\n')
# out_file.write('};\n\n')
# 
# out_file.write('// segments: layer, start_vertex, num_vertices\n')
# out_file.write('int seg_segments_num = {};\n'.format(len(SEGMENTS)))
# out_file.write('int seg_segments[{}][3] = {{\n'.format(len(SEGMENTS)))
# for s in SEGMENTS:
#     out_file.write('{{ {}, {}, {}, }},\n'.format(s[0], s[1], s[2]))
# out_file.write('};\n\n')
# 
# out_file.write('// nodes: node_index, start_segment, num_segments\n')
# out_file.write('int seg_nodes_num = {};\n'.format(len(NODES)))
# out_file.write('int seg_nodes[{}][3] = {{\n'.format(len(NODES)))
# for i,n in NODES.items():
#     out_file.write('{{ {}, {}, {} }},\n'.format(i, n[0], n[1]))
# out_file.write('};\n\n')
# out_file.close()
# 
# # write .h file
# out_file = open(cur_dir + '/../../src/segdefs.h', 'w')
# out_file.write('#pragma once\n')
# out_file.write('extern int seg_vertices_num;\n')
# out_file.write('extern int seg_vertices[{}];\n'.format(len(VERTICES)))
# out_file.write('extern int seg_segments_num;\n')
# out_file.write('extern int seg_segments[{}][3];\n'.format(len(SEGMENTS)))
# out_file.write('extern int seg_nodes_num;\n')
# out_file.write('extern int seg_nodes[{}][3];\n'.format(len(NODES)))
# out_file.close()

