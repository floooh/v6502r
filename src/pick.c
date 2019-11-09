//------------------------------------------------------------------------------
//  pick.c
//  Mouse picking. General idea is to have a offline-computed 2D grid
//  with triangle indices of picking canditates. Result of picking is 0..N
//  node indices which are picked.
//------------------------------------------------------------------------------
#include "v6502r.h"

static float dot(float2_t v0, float2_t v1) {
    return v0.x*v1.x + v0.y*v1.y;
}

void pick_init(void) {
    app.picking.layer_enabled[0] = true;
    app.picking.layer_enabled[1] = true;
    app.picking.layer_enabled[2] = false;
    app.picking.layer_enabled[3] = false;
    app.picking.layer_enabled[4] = false;
    app.picking.layer_enabled[5] = true;
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
pick_result_t pick(float2_t mouse_pos, float2_t disp_size, float2_t offset, float2_t scale) {
    pick_result_t res = {0};

    // first convert mouse pos into screen space [-1,+1]
    float sx = ((mouse_pos.x / disp_size.x) -0.5f) * 2.0f;
    float sy = ((mouse_pos.y / disp_size.y) -0.5f) * -2.0f;

    // next convert into vertex coordinate space (see chipvis.glsl)
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
    static const uint16_t* vb[MAX_LAYERS] = {
        seg_vertices_0,
        seg_vertices_1,
        seg_vertices_2,
        seg_vertices_3,
        seg_vertices_4,
        seg_vertices_5,
    };
    float2_t p = { (float)vx, (float)vy };
    for (int i = 0; i < (int)pick_grid[cell_index][1]; i++) {
        // index into pick_tris[]
        uint32_t ii = pick_grid[cell_index][0] + i;
        // ...and the actual layer and triangle index
        uint32_t layer_index = pick_tris[ii][0];
        assert(layer_index < MAX_LAYERS);
        if (!app.picking.layer_enabled[layer_index]) {
            continue;
        }
        uint32_t tri_index = pick_tris[ii][1];
        // get the triangle corner points (each vertex is 4 uint16s)
        uint32_t vi = tri_index * 3 * 4;
        float x0 = (float) vb[layer_index][vi+0];
        float y0 = (float) vb[layer_index][vi+1];
        float x1 = (float) vb[layer_index][vi+4];
        float y1 = (float) vb[layer_index][vi+5];
        float x2 = (float) vb[layer_index][vi+8];
        float y2 = (float) vb[layer_index][vi+9];
        float2_t a = { x0, y0 };
        float2_t b = { x1, y1 };
        float2_t c = { x2, y2 };
        if (point_in_triangle(p, a, b, c)) {
            if (res.num_hits < PICK_MAX_HITS) {
                // node index of first triangle corner (will be the same for all corners
                int node_index = vb[layer_index][vi+2];
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

void pick_frame(void) {
    float2_t disp_size = { (float)sapp_width(), (float)sapp_height() };
    float2_t scale = { app.chipvis.scale, app.chipvis.scale*app.chipvis.aspect };
    app.picking.result = pick(app.input.mouse, disp_size, app.chipvis.offset, scale);
    for (int i = 0; i < app.picking.result.num_hits; i++) {
        app.chipvis.node_state[app.picking.result.node_index[i]] = 255;
    }
}

