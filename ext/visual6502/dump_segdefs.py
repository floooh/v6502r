#===============================================================================
#   dump_segdefs.py
#
#   Dump the segment definitions from segdefs.js into a C header.
#
#   The items in a segdefs line seem to be:
#
#   0:      node index
#   1:      pullup state
#   2:      layer
#   3..:    polygon outline vertices as x,y pairs
#===============================================================================

import os

cur_dir = os.path.dirname(os.path.abspath(__file__))

VERTICES = []   # each vertex is a [x,y] pair
SEGMENTS = []   # each segment is a triple of [layer, start_vertex, num_vertices]
NODES = {}      # each node is a range of segments [start_segment, num_seqments]

in_file = open(cur_dir + '/segdefs.js', 'r')
in_lines = in_file.readlines()
in_file.close()

# extract the actual segment data
for line in in_lines:
    if not line.startswith('['):
        continue
    tokens = line.lstrip('[ ').rstrip('],\n\r').split(',')
    node_index = tokens[0]
    pullup = tokens[1]
    layer = tokens[2]
    verts = tokens[3:]
    if not node_index in NODES:
        NODES[node_index] = [len(SEGMENTS), 0]
    SEGMENTS.append([layer, len(VERTICES), len(verts)])
    NODES[node_index][1] += 1
    VERTICES.extend(verts)
    
# write .c file
out_file = open(cur_dir + '/../../src/segdefs.c', 'w')
# copy the original copyright notice
for i in range(0, 10):
    out_file.write(in_lines[i])
out_file.write('// vertex data: x,y, ...\n')
out_file.write('int seg_vertices_num = {};\n'.format(len(VERTICES)))
out_file.write('int seg_vertices[{}] = {{\n'.format(len(VERTICES)))
for i,v in enumerate(VERTICES):
    out_file.write('{},'.format(v))
    if 0 == ((i+1) % 16):
        out_file.write('\n')
out_file.write('};\n\n')

out_file.write('// segments: layer, start_vertex, num_vertices\n')
out_file.write('int seg_segments_num = {};\n'.format(len(SEGMENTS)))
out_file.write('int seg_segments[{}][3] = {{\n'.format(len(SEGMENTS)))
for s in SEGMENTS:
    out_file.write('{{ {}, {}, {}, }},\n'.format(s[0], s[1], s[2]))
out_file.write('};\n\n')

out_file.write('// nodes: node_index, start_segment, num_segments\n')
out_file.write('int seg_nodes_num = {};\n'.format(len(NODES)))
out_file.write('int seg_nodes[{}][3] = {{\n'.format(len(NODES)))
for i,n in NODES.items():
    out_file.write('{{ {}, {}, {} }},\n'.format(i, n[0], n[1]))
out_file.write('};\n\n')
out_file.close()

# write .h file
out_file = open(cur_dir + '/../../src/segdefs.h', 'w')
out_file.write('#pragma once\n')
out_file.write('extern int seg_vertices_num;\n')
out_file.write('extern int seg_vertices[{}];\n'.format(len(VERTICES)))
out_file.write('extern int seg_segments_num;\n')
out_file.write('extern int seg_segments[{}][3];\n'.format(len(SEGMENTS)))
out_file.write('extern int seg_nodes_num;\n')
out_file.write('extern int seg_nodes[{}][3];\n'.format(len(NODES)))
out_file.close()

