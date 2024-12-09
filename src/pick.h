#pragma once
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    uint16_t x, y, node_index, reserved;
} pick_vertex_t;

typedef struct {
    uint32_t first_triangle;
    uint32_t num_triangles;
} pick_cell_t;

typedef struct {
    const pick_vertex_t* verts;
    uint32_t num_verts;
} pick_layer_t;

typedef struct {
    uint32_t layer;
    uint32_t tri_index;
} pick_triangle_t;

typedef struct {
    const pick_triangle_t* tris;
    uint32_t num_tris;
} pick_mesh_t;

typedef struct {
    uint16_t num_cells_x;
    uint16_t num_cells_y;
    const pick_cell_t* cells;
    uint32_t num_cells;
} pick_grid_t;

typedef struct {
    uint16_t seg_min_x;
    uint16_t seg_min_y;
    uint16_t seg_max_x;
    uint16_t seg_max_y;
    pick_layer_t layers[MAX_LAYERS];
    pick_mesh_t mesh;
    pick_grid_t grid;
} pick_desc_t;

typedef struct {
    int num_hits;
    int node_index[PICK_MAX_HITS];
} pick_result_t;

void pick_init(const pick_desc_t* desc);
void pick_shutdown(void);
void pick_set_layer_enabled(int layer_index, bool enabled);
bool pick_get_layer_enabled(int layer_index);
void pick_toggle_layer_enabled(int layer_index);
pick_result_t pick_dopick(float2_t mouse_pos, float2_t gfx_size, float2_t gfx_offset, float gfx_aspect, float gfx_scale);
pick_result_t pick_get_last_result(void);

#if defined(__cplusplus)
} // extern "C"
#endif
