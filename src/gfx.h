#pragma once
#include <stdint.h>
#include "sokol_gfx.h"
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    float4_t colors[MAX_LAYERS];
} gfx_palette_t;

typedef struct {
    sg_range seg_vertices[MAX_LAYERS];
    uint16_t seg_max_x;
    uint16_t seg_max_y;
} gfx_desc_t;

void gfx_init(const gfx_desc_t* desc);
void gfx_shutdown(void);

void gfx_new_frame(float disp_width, float disp_height);
void gfx_begin(void);
void gfx_draw(void);
void gfx_end(void);

float2_t gfx_get_display_size(void);
void gfx_set_offset(float2_t offset);
float2_t gfx_get_offset(void);
void gfx_add_scale(float scale_add);
float gfx_get_scale(void);
float gfx_get_aspect(void);
void gfx_highlight_node(int node_index);
void gfx_toggle_layer_visibility(int layer_index);
bool gfx_get_layer_visibility(int layer_index);
void gfx_set_layer_palette(bool use_additive_blend, gfx_palette_t palette);
range_t gfx_get_nodestate_buffer(void);

#if defined(__cplusplus)
} // extern "C"
#endif
