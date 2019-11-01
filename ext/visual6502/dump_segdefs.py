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
VERTICES = []   # each vertex is a (x,y,u,v) tuple (u being the node index)
MAX_X = 0
MAX_Y = 0

# segments (== polygons) bucketed into layers, each segment is a triple
# of [node_index, start_vertex_index, num_vertices]
MAX_LAYERS = 6
SEGMENTS = [[] for i in range(0, MAX_LAYERS)]

# sparse array of nodes (== collection of related segments)
# the dictionary's key is the node index, and the value is an 
# array of [layer, start_segment, num_segments] triples
MAX_NODES = 1800
NODES = [[0,0,0] for i in range(0, MAX_NODES)]

# picking grid, each cell is a list of triangle indices
GRID_NUM_CELLS = 128    # number of cells along x/y, length is MAX_X, MAX_Y
GRID_STACK = [[] for i in range(0, MAX_LAYERS*GRID_NUM_CELLS*GRID_NUM_CELLS)]
GRID_TRIANGLES = []     # flat triangle index soup
GRID = []               # per grid-cell range types into GRID_TRIANGLES

# one vertex buffer per layer
VERTEXBUFFERS = [[] for i in range(0, MAX_LAYERS)]

#-------------------------------------------------------------------------------
# read the segdefs.js file and extract vertex-, segment- and node-data
# into VERTICES, SEGMENTS and NODES
#
def parse_segdef():
    global MAX_X, MAX_Y
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
            x = int(tokens[i])
            y = int(tokens[i+1])
            if x > MAX_X:
                MAX_X = x
            if y > MAX_Y:
                MAX_Y = y
            verts.append((x,y))
        SEGMENTS[layer].append( [ node_index, len(VERTICES), len(verts) ] )
        if NODES[node_index][2] == 0:
            NODES[node_index] = [layer, len(SEGMENTS[layer]), 1]
        else:
            NODES[node_index][2] += 1
        VERTICES.extend(verts)

#-------------------------------------------------------------------------------
# Sort triangles into picking grid.
#
def pos_to_grid(vert):
    mul_x = float(GRID_NUM_CELLS) / float(MAX_X)
    mul_y = float(GRID_NUM_CELLS) / float(MAX_Y)
    x = int(vert[0]*mul_x)
    y = int(vert[1]*mul_y)
    return (x, y)

def add_tris_to_grid(layer, triangles):
    tri_base_index = int(len(VERTEXBUFFERS[layer])/3)
    for i,tri in enumerate(triangles):
        tri_index = tri_base_index + i
        # convert triangle vertices into grid coordinates
        (x0, y0) = pos_to_grid(tri[0])
        (x1, y1) = pos_to_grid(tri[1])
        (x2, y2) = pos_to_grid(tri[2])
        min_x = min(x0, min(x1, x2))
        min_y = min(y0, min(y1, y2))
        max_x = max(x0, max(x1, x2))
        max_y = max(y0, max(y1, y2))
        for y in range(min_y, max_y+1):
            for x in range(min_x, max_x+1):
                cell_index = layer*(GRID_NUM_CELLS*GRID_NUM_CELLS)+y*GRID_NUM_CELLS+x
                GRID_STACK[cell_index].append(tri_index)

#-------------------------------------------------------------------------------
# Generate a triangulated index buffer per segment 
#
def gen_triangles():
    for l,layer in enumerate(SEGMENTS):
        for seg in layer:
            tris = tripy.earclip(VERTICES[seg[1]:seg[1]+seg[2]])
            # add triangles picking grid
            add_tris_to_grid(l, tris)
            # add triangle vertices to vertex buffer
            for tri in tris:
                VERTEXBUFFERS[l].append((tri[0][0], tri[0][1], seg[0], 0))
                VERTEXBUFFERS[l].append((tri[1][0], tri[1][1], seg[0], 0))
                VERTEXBUFFERS[l].append((tri[2][0], tri[2][1], seg[0], 0))

#-------------------------------------------------------------------------------
def flatten_picking_grid():
    cells_per_layer = GRID_NUM_CELLS*GRID_NUM_CELLS
    for i in range(0, cells_per_layer):
        start_index = len(GRID_TRIANGLES)
        for l in range(0, MAX_LAYERS):
            stack_index = l*cells_per_layer + i
            for t in GRID_STACK[stack_index]:
                GRID_TRIANGLES.append((l, t))
        GRID.append((start_index, len(GRID_TRIANGLES)-start_index))

#-------------------------------------------------------------------------------
def write_header():
    fp = open(cur_dir + '/../../src/segdefs.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edit!\n");
    fp.write('#include <stdint.h>\n')
    fp.write('static const uint16_t seg_max_x = {}; // max x coordinate (min is 0)\n'.format(MAX_X))
    fp.write('static const uint16_t seg_max_y = {}; // max y coordinate (min is 0)\n'.format(MAX_Y))
    fp.write('static const uint16_t grid_cells = {}; // length of picking grid in one dimension\n'.format(GRID_NUM_CELLS))
    for i,vb in enumerate(VERTEXBUFFERS):
        fp.write('extern uint16_t seg_vertices_{}[{}]; // (x,y,u=node_index,v=0) as triangle list\n'.format(i,len(vb)*4))
    fp.write('extern uint32_t pick_tris[{}][2]; // (layer,tri_index) pairs for picking check\n'.format(len(GRID_TRIANGLES)))
    fp.write('extern uint32_t pick_grid[{}][2]; // [y*grid_cells+x](start,num) pairs into pick_tris\n'.format(len(GRID)))
    fp.close()

#-------------------------------------------------------------------------------
def write_source():
    fp = open(cur_dir + '/../../src/segdefs.c', 'w')
    fp.write("// machine generated, don't edit!\n")
    fp.write('#include "segdefs.h"\n')
    for ivb,vb in enumerate(VERTEXBUFFERS):
        fp.write('uint16_t seg_vertices_{}[{}] = {{\n'.format(ivb,len(vb)*4))
        for iv,v in enumerate(vb):
            fp.write('{},{},{},{},'.format(v[0],v[1],v[2],v[3]))
            if 0 == ((iv+1) % 8):
                fp.write('\n')
        fp.write('};\n')
    fp.write('uint32_t pick_tris[{}][2] = {{\n'.format(len(GRID_TRIANGLES)))
    for itri,tri in enumerate(GRID_TRIANGLES):
        fp.write('{{{},{}}},'.format(tri[0],tri[1]))    # layer+triindex
        if 0 == ((itri+1) % 16):
            fp.write('\n')
    fp.write('};\n')
    fp.write('uint32_t pick_grid[{}][2] = {{\n'.format(len(GRID)))
    for ir,r in enumerate(GRID):
        fp.write('{{{},{}}},'.format(r[0],r[1])) # start+num in pick_tris array
        if 0 == ((ir+1) % 16):
            fp.write('\n')
    fp.write('};\n')
    fp.close()

#-------------------------------------------------------------------------------
#   main
#
parse_segdef()
gen_triangles()
flatten_picking_grid()
write_header()
write_source()

num_empty_cells = 0
max_tris_per_cell = 0
avg_tris_per_cell = 0
all_tris = 0
cells_per_layer = GRID_NUM_CELLS*GRID_NUM_CELLS
for i in range(0,cells_per_layer):
    tris_per_cell = 0
    for j in range(0, MAX_LAYERS):
        tris_per_cell += len(GRID_STACK[j*cells_per_layer + i])
    if tris_per_cell == 0:
        num_empty_cells += 1
    avg_tris_per_cell += tris_per_cell
    all_tris += tris_per_cell
    max_tris_per_cell = max(max_tris_per_cell, tris_per_cell)
avg_tris_per_cell /= cells_per_layer
print('num_empty_cells: {}\nmax_tris_per_cell: {}\navg_tris_per_cell: {}\n'.format(num_empty_cells, max_tris_per_cell, avg_tris_per_cell))



