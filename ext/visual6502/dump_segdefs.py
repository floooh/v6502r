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
import tripy

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
        verts = []
        for i in range(3, len(tokens), 2):
            verts.append( ( int(tokens[i]), int(tokens[i+1]) ) )
        SEGMENTS[layer].append( [ node_index, len(VERTICES), len(verts) ] )
        if NODES[node_index][2] == 0:
            NODES[node_index] = [layer, len(SEGMENTS[layer]), 1]
        else:
            NODES[node_index][2] += 1
        VERTICES.extend(verts)

#-------------------------------------------------------------------------------
def gen_triangles(first, num):
    poly = VERTICES[first:first+num]
    tris = tripy.earclip(poly)

#-------------------------------------------------------------------------------
# Generate a triangulated index buffer per segment 
#
def gen_vertex_buffers():
    for l,layer in enumerate(SEGMENTS):
        for seg in layer:
            tris = tripy.earclip(VERTICES[seg[1]:seg[1]+seg[2]])
            for tri in tris:
                VERTEXBUFFERS[l].append(tri[0])
                VERTEXBUFFERS[l].append(tri[1])
                VERTEXBUFFERS[l].append(tri[2])

#-------------------------------------------------------------------------------
def write_header():
    fp = open(cur_dir + '/../../src/segdefs.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edit!\n");
    fp.write('#include <stdint.h>\n')
    for i,vb in enumerate(VERTEXBUFFERS):
        fp.write('extern uint16_t seg_vertices_{}[{}];\n'.format(i,len(vb)*2))
    fp.close()

#-------------------------------------------------------------------------------
def write_source():
    fp = open(cur_dir + '/../../src/segdefs.c', 'w')
    fp.write("// machine generated, don't edit!\n")
    fp.write('#include "segdefs.h"\n')
    for ivb,vb in enumerate(VERTEXBUFFERS):
        fp.write('uint16_t seg_vertices_{}[{}] = {{\n'.format(ivb,len(vb)*2))
        for iv,v in enumerate(vb):
            fp.write('{},{},'.format(v[0],v[1]))
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
