//------------------------------------------------------------------------------
//  pick.c
//  Mouse picking. General idea is to have a offline-computed 2D grid
//  with triangle indices of picking canditates. Result of picking is 0..N
//  node indices which are picked.
//------------------------------------------------------------------------------
#include "pick.h"

static float dot(float2_t v0, float2_t v1) {
    return v0.x*v1.x + v0.y*v1.y;
}

void pick_init(pick_t* pick, const pick_desc_t* desc) {
    assert(pick && desc);
    assert((desc->seg_max_x > 0) && (desc->seg_max_y > 0) && (desc->grid_cells > 0));
    assert(desc->mesh.tris && (desc->mesh.num_tris > 0));
    assert(desc->grid.cells && (desc->grid.num_cells > 0));
    assert(desc->grid.num_cells == (desc->grid_cells * desc->grid_cells));
    pick->desc = *desc;
    pick->layer_enabled[0] = true;
    pick->layer_enabled[1] = true;
    pick->layer_enabled[2] = false;
    pick->layer_enabled[3] = false;
    pick->layer_enabled[4] = false;
    pick->layer_enabled[5] = true;
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
static pick_result_t do_pick(pick_t* pick, float2_t mouse_pos, float2_t disp_size, float2_t offset, float2_t scale) {
    assert(pick);

    pick_result_t res = {0};
    const uint16_t seg_max_x = pick->desc.seg_max_x;
    const uint16_t seg_max_y = pick->desc.seg_max_y;
    const uint16_t grid_cells = pick->desc.grid_cells;

    // first convert mouse pos into screen space [-1,+1]
    float sx = ((mouse_pos.x / disp_size.x) -0.5f) * 2.0f;
    float sy = ((mouse_pos.y / disp_size.y) -0.5f) * -2.0f;

    // next convert into vertex coordinate space (see gfx.glsl)
    float half_size_x = (seg_max_x>>1)/65535.0f;
    float half_size_y = (seg_max_y>>1)/65535.0f;
    float vx = ((sx / scale.x) - offset.x + half_size_x) * 65535.0f;
    float vy = ((sy / scale.y) - offset.y + half_size_y) * 65535.0f;
    if (((int)vx<=0) || ((int)vy<=0) || ((int)vx>=seg_max_x) || ((int)vy>=seg_max_y)) {
        // out of bounds
        return res;
    }
    // get the picking grid coordinate
    float cell_width = (float)seg_max_x / (float)grid_cells;
    float cell_height = (float)seg_max_y / (float)grid_cells;
    int gx = (int)(vx / cell_width);
    int gy = (int)(vy / cell_height);
    int cell_index = gy * grid_cells + gx;
    if ((cell_index < 0) || (cell_index >= (grid_cells*grid_cells))) {
        // this should never happen, but anyway...
        return res;
    }

    // check triangles in that grid cell, each grid cell is
    // an index,num pair into the pick_tris array
    float2_t p = { (float)vx, (float)vy };
    const pick_grid_t* pick_grid = &pick->desc.grid;
    const pick_mesh_t* pick_mesh = &pick->desc.mesh;
    assert(cell_index < (int)pick->desc.grid.num_cells);
    for (int i = 0; i < (int)pick_grid->cells[cell_index].num_triangles; i++) {
        // index into pick_tris[]
        uint32_t ii = pick_grid->cells[cell_index].first_triangle + i;
        // ...and the actual layer and triangle index
        assert(ii < pick_mesh->num_tris);
        uint32_t layer_index = pick_mesh->tris[ii].layer;
        assert(layer_index < MAX_LAYERS);
        if (!pick->layer_enabled[layer_index]) {
            continue;
        }
        uint32_t tri_index = pick_mesh->tris[ii].tri_index;
        // get the triangle corner points (each vertex is 4 uint16s)
        uint32_t vi = tri_index * 3;
        const pick_layer_t* pick_layer = &pick->desc.layers[layer_index];
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

void pick_frame(pick_t* pick, float2_t mouse_pos, float2_t disp_size, float2_t gfx_offset, float gfx_aspect, float gfx_scale) {
    float2_t scale = { gfx_scale, gfx_scale * gfx_aspect };
    pick->result = do_pick(pick, mouse_pos, disp_size, gfx_offset, scale);
}

