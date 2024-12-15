#===============================================================================
#   Dump the segment definitions from segdefs.js into a C header, convert
#   into triangles, precompute mouse picking helper structs.
#
#   The items in a segdefs line are:
#
#   0:      node index
#   1:      pullup state
#   2:      layer
#   3..:    polygon outline vertices as x,y pairs
#===============================================================================

import earcut

# this is the original extracted data
VERTICES = []   # each vertex is a (x,y,u,v) tuple (u being the node index)
MAX_X = 0
MAX_Y = 0
MIN_X = 65536
MIN_Y = 65536

# segments (== polygons) bucketed into layers, each segment is a triple
# of [node_index, start_vertex_index, num_vertices]
MAX_LAYERS = 6
SEGMENTS = [[] for i in range(0, MAX_LAYERS)]

# sparse array of nodes (== collection of related segments)
# the dictionary's key is the node index, and the value is an
# array of [layer, start_segment, num_segments] triples
MAX_NODES = 8192
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
def parse_segdef(src_dir, scale, remap_index):
    global MAX_X, MAX_Y, MIN_X, MIN_Y
    fp = open(src_dir + '/segdefs.js', 'r')
    lines = fp.readlines()
    fp.close()
    for line in lines:
        if not line.startswith('['):
            continue
        tokens = line.lstrip('[ ').rstrip('],\n\r').split(',')
        node_index = int(tokens[0])
        if remap_index:
            node_index = remap_index(node_index)
        pullup = tokens[1]
        layer = int(tokens[2])
        verts = []
        for i in range(3, len(tokens), 2):
            # NOTE: the +1 is because the Z80 has some coords -0.5
            x = int(float(tokens[i]) * scale) + 1
            y = int(float(tokens[i+1]) * scale) + 1
            if x > MAX_X:
                MAX_X = x
            if y > MAX_Y:
                MAX_Y = y
            if x < MIN_X:
                MIN_X = x
            if y < MIN_Y:
                MIN_Y = y
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
# Some segments consist of multiple polygons, split them accordingly, the
# returned array consists of types (node_index, start_vertex_index, num_vertices).
#
# Such multi-path segments only exists on the Z80 and define polygons with holes
# (the first path is the actual poly, and all following paths are holes)
#
def split_segment(seg):
    res = []
    i0 = seg[1]
    i1 = i0 + seg[2]
    i_start = i0
    # special case, v0 == v1
    # (this is why the original renderer moves the first vertex to the end:
    # https://github.com/trebonian/visual6502/blob/d8ecc129b34e0eaf320e0400fcf33329475bdb1e/wires.js#L232-L233)
    v0 = VERTICES[i0]
    v1 = VERTICES[i0 + 1]
    if v0[0] == v1[0] and v0[1] == v1[1]:
        print(f'skip initial duplicate vertex in segment: {seg[0]}')
        i0 += 1
    for i in range(i0, i1):
        if i == i_start:
            continue
        v = VERTICES[i]
        x = v[0]
        y = v[1]
        v0 = VERTICES[i_start]
        x0 = v0[0]
        y0 = v0[1]
        last = i == i1 - 1
        if (x == x0 and y == y0) or last:
            # found end of current polygon
            num_verts = i - i_start
            if last:
                num_verts += 1
            if num_verts > 2:
                new_seg = (seg[0], i_start, num_verts)
                res.append(new_seg)
            else:
                print(f'ignoring path with less than 3 vertices in segment: {seg[0]}')
            # skip the loop-back vertex
            i_start = i + 1
    return res

#-------------------------------------------------------------------------------
# Generate a triangulated index buffer per segment
#
def gen_triangles():
    for l,layer in enumerate(SEGMENTS):
        for seg in layer:
            # some segments are made of multiple polygons
            split_segs = split_segment(seg)
            if len(split_segs) == 0:
                print(f'skipping invalid segment (no paths with at least 3 vertices): {seg[0]}')
                continue
            if len(split_segs) > 1:
                print(f'found multi-poly-segment: {seg[0]} num={len(split_segs)}')
            # convert to 'first sequence is poly and following sequences are holes'
            poly_and_holes = []
            for s in split_segs:
                poly_and_holes.append(VERTICES[s[1]:s[1]+s[2]])
            data = earcut.flatten(poly_and_holes);
            vertices = data['vertices']
            holes = data['holes']
            dims = data['dimensions']
            tri_indices = earcut.earcut(vertices, holes, dims)
            if 0 != earcut.deviation(vertices, holes, dims, tri_indices):
                print(f'incorrect triangulation in segment: {seg[0]}')
            tris = []
            for i in range(int(len(tri_indices) / 3)):
                i0 = tri_indices[i * 3 + 0]
                i1 = tri_indices[i * 3 + 1]
                i2 = tri_indices[i * 3 + 2]
                v0 = (vertices[i0 * 2 + 0], vertices[i0 * 2 + 1])
                v1 = (vertices[i1 * 2 + 0], vertices[i1 * 2 + 1])
                v2 = (vertices[i2 * 2 + 0], vertices[i2 * 2 + 1])
                tris.append([v0, v1, v2])

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
def write_header(dst_dir):
    fp = open(dst_dir + '/segdefs.h', 'w')
    fp.write('#pragma once\n')
    fp.write("// machine generated, don't edit!\n");
    fp.write('#include <stdint.h>\n')
    fp.write('static const uint16_t seg_max_x = {}; // max x coordinate\n'.format(MAX_X))
    fp.write('static const uint16_t seg_max_y = {}; // max y coordinate\n'.format(MAX_Y))
    fp.write('static const uint16_t seg_min_x = {}; // min x coordinate\n'.format(MIN_X))
    fp.write('static const uint16_t seg_min_y = {}; // min y coordinate\n'.format(MIN_Y))
    fp.write('static const uint16_t grid_cells = {}; // length of picking grid in one dimension\n'.format(GRID_NUM_CELLS))
    for i,vb in enumerate(VERTEXBUFFERS):
        if len(vb) > 0:
            fp.write('extern uint16_t seg_vertices_{}[{}]; // (x,y,u=node_index,v=0) as triangle list\n'.format(i,len(vb)*4))
    fp.write('extern uint32_t pick_tris[{}][2]; // (layer,tri_index) pairs for picking check\n'.format(len(GRID_TRIANGLES)))
    fp.write('extern uint32_t pick_grid[{}][2]; // [y*grid_cells+x](start,num) pairs into pick_tris\n'.format(len(GRID)))
    fp.close()

#-------------------------------------------------------------------------------
def write_source(dst_dir):
    fp = open(dst_dir + '/segdefs.c', 'w')
    fp.write("// machine generated, don't edit!\n")
    fp.write('#include "segdefs.h"\n')
    for ivb,vb in enumerate(VERTEXBUFFERS):
        if len(vb) > 0:
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
def dump(src_dir, dst_dir, scale, remap_index=None):
    parse_segdef(src_dir, scale, remap_index)
    gen_triangles()
    flatten_picking_grid()
    write_header(dst_dir)
    write_source(dst_dir)

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
