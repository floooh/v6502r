#pragma once
#include "sokol_gfx.h"
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define GFX_CSSRGB(c,a) {((c>>16)&0xFF)/255.0f, ((c>>8)&0xFF)/255.0f, (c&0xFF)/255.0f, a}

static const uint8_t gfx_visual_node_inactive = 100;
static const uint8_t gfx_visual_node_active = 190;
static const uint8_t gfx_visual_node_highlighted = 255;

typedef struct {
    float4_t colors[MAX_LAYERS];
    float4_t background;
} gfx_palette_t;

static const gfx_palette_t gfx_default_palette = {
    {
        GFX_CSSRGB(0xf50057, 1.0f),
        GFX_CSSRGB(0xffeb3b, 1.0f),
        GFX_CSSRGB(0xff5252, 1.0f),   // unused?
        GFX_CSSRGB(0x7e57c2, 0.7f),
        GFX_CSSRGB(0xfb8c00, 0.7f),
        GFX_CSSRGB(0x00b0ff, 1.0f),
    },
    { 0.1f, 0.1f, 0.15f, 1.0f }
};

typedef struct {
    sg_range seg_vertices[MAX_LAYERS];
    uint16_t seg_min_x;
    uint16_t seg_min_y;
    uint16_t seg_max_x;
    uint16_t seg_max_y;
} gfx_desc_t;

void gfx_preinit(void);
void gfx_init(const gfx_desc_t* desc);
void gfx_shutdown(void);

void gfx_new_frame(float disp_width, float disp_height);
void gfx_begin(void);
void gfx_draw(void);
void gfx_end(void);

float2_t gfx_get_display_size(void);
void gfx_set_offset(float2_t offset);
float2_t gfx_get_offset(void);
float gfx_min_scale(void);
float gfx_max_scale(void);
void gfx_add_scale(float scale_add);
void gfx_set_scale(float scale);
float gfx_get_scale(void);
float gfx_get_aspect(void);
void gfx_highlight_node(int node_index);
void gfx_toggle_layer_visibility(int layer_index);
void gfx_set_layer_visibility(int layer_index, bool visible);
bool gfx_get_layer_visibility(int layer_index);
void gfx_set_layer_palette(bool use_additive_blend, gfx_palette_t palette);
range_t gfx_get_nodestate_buffer(void);

#if defined(__cplusplus)
} // extern "C"
#endif
