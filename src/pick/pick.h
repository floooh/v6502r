#pragma once
#include "../common.h"
#include "sokol_gfx.h"  // sg_range

// see segdefs.h seg_vertices
typedef struct {
    uint16_t x, y, node_index, reserved;
} pick_vertex_t;

// see segdefs.h: pick_tris
typedef struct {
    uint32_t layer;
    uint32_t tri_index;
} pick_triangle_t;

// see segdefs.h: pick_grid
typedef struct {
    uint32_t first_triangle;
    uint32_t num_triangles;
} pick_cell_t;

typedef struct {
    const pick_vertex_t* verts;
    uint32_t num_verts;
} pick_layer_t;

typedef struct {
    const pick_triangle_t* tris;
    uint32_t num_tris;
} pick_mesh_t;

typedef struct {
    const pick_cell_t* cells;
    uint32_t num_cells;
} pick_grid_t;

typedef struct {
    uint16_t seg_max_x;
    uint16_t seg_max_y;
    uint16_t grid_cells;
    pick_layer_t layers[MAX_LAYERS];
    pick_mesh_t mesh;
    pick_grid_t grid;
} pick_desc_t;

typedef struct {
    int num_hits;
    int node_index[PICK_MAX_HITS];
} pick_result_t;

typedef struct {
    pick_desc_t desc;
    pick_result_t result;
    bool layer_enabled[MAX_LAYERS];
} pick_t;

void pick_init(pick_t* pick, const pick_desc_t* desc);
void pick_frame(pick_t* pick, float2_t mouse_pos, float2_t gfx_size, float2_t gfx_offset, float gfx_aspect, float gfx_scale);
