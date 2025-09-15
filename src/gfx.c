//------------------------------------------------------------------------------
//  gfx.c
//
//  Chip visualization rendering.
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "gfx.h"
#include "gfx.glsl.h"
#include <stdio.h>  // snprintf

static struct {
    bool valid;
    struct {
        sg_buffer vb;
        int num_elms;
    } layers[MAX_LAYERS];
    uint16_t seg_min_x;
    uint16_t seg_min_y;
    uint16_t seg_max_x;
    uint16_t seg_max_y;
    sg_pipeline pip_alpha;
    sg_pipeline pip_add;
    sg_image img;
    sg_view tex_view;
    sg_sampler smp;
    float2_t display_size;
    float aspect;
    float scale;
    float2_t scale_pivot;
    float2_t offset;
    bool use_additive_blend;
    gfx_palette_t palette;
    bool layer_visible[MAX_LAYERS];
    uint8_t node_state[MAX_NODES];
} gfx;

void gfx_preinit(void) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 16,
        .image_pool_size = 8,
        .view_pool_size = 8,
        .pipeline_pool_size = 8,
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });
}

void gfx_init(const gfx_desc_t* desc) {
    assert(!gfx.valid);
    assert(desc);
    assert(desc->seg_max_x > 0);
    assert(desc->seg_max_y > 0);

    // default values
    gfx.seg_min_x = desc->seg_min_x;
    gfx.seg_min_y = desc->seg_min_y;
    gfx.seg_max_x = desc->seg_max_x;
    gfx.seg_max_y = desc->seg_max_y;
    gfx.aspect = 1.0f;
    gfx.scale = 9.0f;
    gfx.palette = gfx_default_palette;
    for (int i = 0; i < MAX_LAYERS; i++) {
        gfx.layer_visible[i] = true;
    }

    // create vertex buffer from offline-baked vertex data
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (desc->seg_vertices[i].ptr && (desc->seg_vertices[i].size > 0)) {
            char label[128];
            snprintf(label, sizeof(label), "node-layer-%d", i);
            gfx.layers[i].vb = sg_make_buffer(&(sg_buffer_desc){
                .data = desc->seg_vertices[i],
                .label = label,
            });
            gfx.layers[i].num_elms = (int)(desc->seg_vertices[i].size / 8);
            assert(gfx.layers[i].num_elms > 0);
        }
    }

    // create shaders and pipeline objects
    sg_pipeline_desc pip_desc_alpha = {
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_USHORT2N },
                [1]  = { .format = SG_VERTEXFORMAT_SHORT2 }
            },
        },
        .shader = sg_make_shader(shd_alpha_shader_desc(sg_query_backend())),
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_NONE,
        .colors[0] = {
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        },
        .label = "alpha-blend-pipeline",
    };
    sg_pipeline_desc pip_desc_add = pip_desc_alpha;
    pip_desc_add.shader = sg_make_shader(shd_add_shader_desc(sg_query_backend()));
    pip_desc_add.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
    pip_desc_add.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
    pip_desc_add.label = "additive-blend-pipeline";
    gfx.pip_alpha = sg_make_pipeline(&pip_desc_alpha);
    gfx.pip_add = sg_make_pipeline(&pip_desc_add);

    // a dynamic palette texture with one pixel per netlist node, and a matching texture view and sampler
    assert((MAX_NODES & 255) == 0);
    gfx.img = sg_make_image(&(sg_image_desc){
        .width = 256,
        .height = MAX_NODES / 256,
        .pixel_format = SG_PIXELFORMAT_R8,
        .usage.stream_update = true,
        .label = "node-texture",
    });
    gfx.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = gfx.img,
        .label = "node-texture-view",
    });
    gfx.smp = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .label = "node-sampler",
    });
    gfx.valid = true;
}

void gfx_shutdown(void) {
    assert(gfx.valid);
    for (int i = 0; i < MAX_LAYERS; i++) {
        sg_destroy_buffer(gfx.layers[i].vb);
    }
    sg_destroy_pipeline(gfx.pip_alpha);
    sg_destroy_pipeline(gfx.pip_add);
    sg_destroy_view(gfx.tex_view);
    sg_destroy_image(gfx.img);
    sg_destroy_sampler(gfx.smp);
    sg_shutdown();
    memset(&gfx, 0, sizeof(gfx));
}

void gfx_new_frame(float disp_width, float disp_height) {
    assert(gfx.valid);
    gfx.display_size = (float2_t) { disp_width, disp_height };
    gfx.aspect = gfx.display_size.y / gfx.display_size.x;
}

void gfx_begin(void) {
    assert(gfx.valid);
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = {
                    .r = gfx.palette.background.x,
                    .g = gfx.palette.background.y,
                    .b = gfx.palette.background.z,
                    .a = 1.0f,
                },
            }
        },
        .swapchain = sglue_swapchain()
    });
}

void gfx_draw(void) {
    assert(gfx.valid);
    const float sx = gfx.scale * gfx.aspect;
    const float sy = gfx.scale;
    vs_params_t vs_params = {
        .half_size = (float2_t){(gfx.seg_max_x>>1)/65535.0f, (gfx.seg_max_y>>1)/65535.0f},
        .offset = gfx.offset,
        .scale = (float2_t) { sx, sy },
    };
    sg_update_image(gfx.img, &(sg_image_data){
        .mip_levels[0] = SG_RANGE(gfx.node_state)
    });
    if (gfx.use_additive_blend) {
        sg_apply_pipeline(gfx.pip_add);
    }
    else {
        sg_apply_pipeline(gfx.pip_alpha);
    }
    for (int i = 0; i < MAX_LAYERS; i++) {
        vs_params.color0 = gfx.palette.colors[i];
        if (gfx.layer_visible[i] && (gfx.layers[i].vb.id != SG_INVALID_ID)) {
            sg_apply_bindings(&(sg_bindings){
                .vertex_buffers[0] = gfx.layers[i].vb,
                .views[VIEW_palette_tex] = gfx.tex_view,
                .samplers[SMP_palette_smp] = gfx.smp,
            });
            sg_apply_uniforms(UB_vs_params, &SG_RANGE(vs_params));
            sg_draw(0, gfx.layers[i].num_elms, 1);
        }
    }
}

void gfx_end(void) {
    assert(gfx.valid);
    sg_end_pass();
    sg_commit();
}

float2_t gfx_get_display_size(void) {
    assert(gfx.valid);
    return gfx.display_size;
}

void gfx_set_offset(float2_t offset) {
    assert(gfx.valid);
    gfx.offset = offset;
}

float2_t gfx_get_offset(void) {
    assert(gfx.valid);
    return gfx.offset;
}

float gfx_min_scale(void) {
    return 1.0f;
}

float gfx_max_scale(void) {
    return 100.0f;
}

void gfx_add_scale(float scale_add) {
    assert(gfx.valid);
    gfx_set_scale(gfx.scale + scale_add);
}

void gfx_set_scale(float scale) {
    gfx.scale = scale;
    if (gfx.scale < gfx_min_scale()) {
        gfx.scale = gfx_min_scale();
    }
    else if (gfx.scale > gfx_max_scale()) {
        gfx.scale = gfx_max_scale();
    }
}

float gfx_get_scale(void) {
    assert(gfx.valid);
    return gfx.scale;
}

float gfx_get_aspect(void) {
    assert(gfx.valid);
    return gfx.aspect;
}

void gfx_highlight_node(int node_index) {
    assert(gfx.valid);
    assert((node_index >= 0) && (node_index < MAX_NODES));
    gfx.node_state[node_index] = gfx_visual_node_highlighted;
}

void gfx_toggle_layer_visibility(int layer_index) {
    assert(gfx.valid);
    assert((layer_index >= 0) && (layer_index < MAX_LAYERS));
    gfx.layer_visible[layer_index] = !gfx.layer_visible[layer_index];
}

void gfx_set_layer_visibility(int layer_index, bool visible) {
    assert(gfx.valid);
    assert((layer_index >= 0) && (layer_index < MAX_LAYERS));
    gfx.layer_visible[layer_index] = visible;
}

bool gfx_get_layer_visibility(int layer_index) {
    assert(gfx.valid);
    assert((layer_index >= 0) && (layer_index < MAX_LAYERS));
    return gfx.layer_visible[layer_index];
}

void gfx_set_layer_palette(bool use_additive_blend, gfx_palette_t palette) {
    assert(gfx.valid);
    gfx.use_additive_blend = use_additive_blend;
    gfx.palette = palette;
}

range_t gfx_get_nodestate_buffer(void) {
    assert(gfx.valid);
    return (range_t){ .ptr = gfx.node_state, .size = sizeof(gfx.node_state) };
}
