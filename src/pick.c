//------------------------------------------------------------------------------
//  pick.c
//  Mouse picking. General idea is to have a offline-computed 2D grid
//  with triangle indices of picking canditates. Result of picking is 0..N
//  node indices which are picked.
//------------------------------------------------------------------------------
#include "pick.h"

static struct {
    bool valid;
    uint16_t seg_max_x;
    uint16_t seg_max_y;
    bool layer_enabled[MAX_LAYERS];
    pick_layer_t layers[MAX_LAYERS];
    pick_mesh_t mesh;
    pick_grid_t grid;
    pick_result_t result;
} pick;

void pick_init(const pick_desc_t* desc) {
    assert(!pick.valid);
    assert(desc);
    assert((desc->seg_max_x > 0) && (desc->seg_max_y > 0));
    assert(desc->mesh.tris && (desc->mesh.num_tris > 0));
    assert(desc->grid.cells && (desc->grid.num_cells > 0));
    assert(desc->grid.num_cells == (desc->grid.num_cells_x * desc->grid.num_cells_y));
    pick.seg_max_x = desc->seg_max_x;
    pick.seg_max_y = desc->seg_max_y;
    for (int i = 0; i < MAX_LAYERS; i++) {
        pick.layer_enabled[i] = true;
        pick.layers[i] = desc->layers[i];
    }
    pick.mesh = desc->mesh;
    pick.grid = desc->grid;
    pick.valid = true;
}

void pick_shutdown(void) {
    assert(pick.valid);
    memset(&pick, 0, sizeof(pick));
}

bool pick_get_layer_enabled(int layer_index) {
    assert(pick.valid);
    assert((layer_index >= 0) && (layer_index < MAX_LAYERS));
    return pick.layer_enabled[layer_index];
}

void pick_set_layer_enabled(int layer_index, bool enabled) {
    assert(pick.valid);
    assert((layer_index >= 0) && (layer_index < MAX_LAYERS));
    pick.layer_enabled[layer_index] = enabled;
}

void pick_toggle_layer_enabled(int layer_index) {
    assert(pick.valid);
    assert((layer_index >= 0) && (layer_index < MAX_LAYERS));
    pick.layer_enabled[layer_index] = !pick.layer_enabled[layer_index];
}

static float dot(float2_t v0, float2_t v1) {
    return v0.x*v1.x + v0.y*v1.y;
}

static bool point_in_triangle(float2_t p, float2_t a, float2_t b, float2_t c) {
    float2_t v0 = { c.x-a.x, c.y-a.y };
    float2_t v1 = { b.x-a.x, b.y-a.y };
    float2_t v2 = { p.x-a.x, p.y-a.y };
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d02 = dot(v0, v2);
    float d11 = dot(v1, v1);
    float d12 = dot(v1, v2);
    float invDenom = 1.0f / (d00 * d11 - d01 * d01);
    float u = (d11 * d02 - d01 * d12) * invDenom;
    float v = (d00 * d12 - d01 * d02) * invDenom;
    return (u >= 0) && (v >= 0) && (u + v < 1.0f);
}

// not super-duper optimized, but probably fast enough
static pick_result_t do_pick(float2_t mouse_pos, float2_t disp_size, float2_t offset, float2_t scale) {
    pick_result_t res = {0};

    // first convert mouse pos into screen space [-1,+1]
    float sx = ((mouse_pos.x / disp_size.x) -0.5f) * 2.0f;
    float sy = ((mouse_pos.y / disp_size.y) -0.5f) * -2.0f;

    // next convert into vertex coordinate space (see gfx.glsl)
    float half_size_x = (pick.seg_max_x>>1)/65535.0f;
    float half_size_y = (pick.seg_max_y>>1)/65535.0f;
    float vx = ((sx / scale.x) - offset.x + half_size_x) * 65535.0f;
    float vy = ((sy / scale.y) - offset.y + half_size_y) * 65535.0f;
    if (((int)vx<=0) || ((int)vy<=0) || ((int)vx>=pick.seg_max_x) || ((int)vy>=pick.seg_max_y)) {
        // out of bounds
        return res;
    }
    // get the picking grid coordinate
    float cell_width = (float)pick.seg_max_x / (float)pick.grid.num_cells_x;
    float cell_height = (float)pick.seg_max_y / (float)pick.grid.num_cells_y;
    uint32_t gx = (uint32_t)(vx / cell_width);
    uint32_t gy = (uint32_t)(vy / cell_height);
    uint32_t cell_index = gy * pick.grid.num_cells_x + gx;
    if (cell_index >= pick.grid.num_cells) {
        // this should never happen, but anyway...
        return res;
    }

    // check triangles in that grid cell, each grid cell is
    // an index,num pair into the pick_tris array
    float2_t p = { (float)vx, (float)vy };
    const pick_grid_t* pick_grid = &pick.grid;
    const pick_mesh_t* pick_mesh = &pick.mesh;
    assert(cell_index < pick.grid.num_cells);
    for (int i = 0; i < (int)pick_grid->cells[cell_index].num_triangles; i++) {
        // index into pick_tris[]
        uint32_t ii = pick_grid->cells[cell_index].first_triangle + i;
        // ...and the actual layer and triangle index
        assert(ii < pick_mesh->num_tris);
        uint32_t layer_index = pick_mesh->tris[ii].layer;
        assert(layer_index < MAX_LAYERS);
        if (!pick.layer_enabled[layer_index]) {
            continue;
        }
        uint32_t tri_index = pick_mesh->tris[ii].tri_index;
        // get the triangle corner points (each vertex is 4 uint16s)
        uint32_t vi = tri_index * 3;
        const pick_layer_t* pick_layer = &pick.layers[layer_index];
        float x0 = (float) pick_layer->verts[vi].x;
        float y0 = (float) pick_layer->verts[vi].y;
        float x1 = (float) pick_layer->verts[vi+1].x;
        float y1 = (float) pick_layer->verts[vi+1].y;
        float x2 = (float) pick_layer->verts[vi+2].x;
        float y2 = (float) pick_layer->verts[vi+2].y;
        float2_t a = { x0, y0 };
        float2_t b = { x1, y1 };
        float2_t c = { x2, y2 };
        if (point_in_triangle(p, a, b, c)) {
            if (res.num_hits < PICK_MAX_HITS) {
                // node index of first triangle corner (will be the same for all corners
                int node_index = pick_layer->verts[vi].node_index;
                // did we already hit this node?
                bool ignore = false;
                for (int ni = 0; ni < res.num_hits; ni++) {
                    if (res.node_index[ni] == node_index) {
                        ignore = true;
                        break;
                    }
                }
                if (!ignore) {
                    res.node_index[res.num_hits++] = node_index;
                }
            }
        }
    }
    return res;
}

pick_result_t pick_dopick(float2_t mouse_pos, float2_t disp_size, float2_t gfx_offset, float gfx_aspect, float gfx_scale) {
    assert(pick.valid);
    float2_t scale = { gfx_scale * gfx_aspect, gfx_scale };
    pick.result = do_pick(mouse_pos, disp_size, gfx_offset, scale);
    return pick.result;
}

pick_result_t pick_get_last_result(void) {
    assert(pick.valid);
    return pick.result;
}
