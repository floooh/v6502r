#pragma once
#include <stdint.h>
#include "sokol_gfx.h"
#include "common.h"

typedef struct {
    float4_t colors[MAX_LAYERS];
} palette_t;

typedef struct {
    sg_range seg_vertices[MAX_LAYERS];
    uint16_t seg_max_x;
    uint16_t seg_max_y;
} gfx_desc_t;

typedef struct {
    struct {
        sg_buffer vb;
        int num_elms;
    } layers[MAX_LAYERS];
    uint16_t seg_max_x;
    uint16_t seg_max_y;
    sg_pipeline pip_alpha;
    sg_pipeline pip_add;
    sg_image img;
    float2_t display_size;
    float aspect;
    float scale;
    float2_t scale_pivot;
    float2_t offset;
    bool use_additive_blend;
    palette_t layer_palette;
    bool layer_visible[MAX_LAYERS];
    uint8_t node_state[MAX_NODES];
} gfx_t;

void gfx_init(gfx_t* gfx, const gfx_desc_t* desc);
void gfx_shutdown(gfx_t* gfx);
void gfx_new_frame(gfx_t* gfx, float disp_width, float disp_height);
void gfx_begin();
void gfx_draw(gfx_t* gfx);
void gfx_end();
