//------------------------------------------------------------------------------
//  gfx.c
//
//  Chip visualization rendering.
//------------------------------------------------------------------------------
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_glue.h"
#include "gfx.h"
#include "gfx.glsl.h"
#include <assert.h>

static const palette_t default_palette = { {
    { 1.0f, 0.0f, 0.0f, 0.5f },
    { 0.0f, 1.0f, 0.0f, 0.5f },
    { 0.0f, 0.0f, 1.0f, 0.5f },
    { 1.0f, 1.0f, 0.0f, 0.5f },
    { 0.0f, 1.0f, 1.0f, 0.5f },
    { 1.0f, 0.0f, 1.0f, 0.5f },
} };

void gfx_init(gfx_t* gfx, const gfx_desc_t* desc) {
    assert(gfx && desc);
    assert(desc->seg_max_x > 0);
    assert(desc->seg_max_y > 0);

    // setup sokol-gfx
    sg_setup(&(sg_desc){
        .buffer_pool_size = 16,
        .image_pool_size = 8,
        .pipeline_pool_size = 8,
        .context = sapp_sgcontext()
    });

    // default values
    gfx->seg_max_x = desc->seg_max_x;
    gfx->seg_max_y = desc->seg_max_y;
    gfx->aspect = 1.0f;
    gfx->scale = 7.0f;
    gfx->offset = (float2_t) { -0.05f, 0.0f };
    gfx->layer_palette = default_palette;
    for (int i = 0; i < MAX_LAYERS; i++) {
        gfx->layer_visible[i] = true;
    }

    // create vertex buffer from offline-baked vertex data
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (desc->seg_vertices[i].ptr) {
            gfx->layers[i].vb = sg_make_buffer(&(sg_buffer_desc){
                .data = desc->seg_vertices[i]
            });
            gfx->layers[i].num_elms = desc->seg_vertices[i].size / 8;
            assert(gfx->layers[i].num_elms > 0);
        }
    }

    // create shaders and pipeline objects
    sg_pipeline_desc pip_desc_alpha = {
        .layout = {
            .attrs = {
                [ATTR_vs_pos] = { .format = SG_VERTEXFORMAT_USHORT2N },
                [ATTR_vs_uv]  = { .format = SG_VERTEXFORMAT_USHORT2N }
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
        }
    };
    sg_pipeline_desc pip_desc_add = pip_desc_alpha;
    pip_desc_add.shader = sg_make_shader(shd_add_shader_desc(sg_query_backend()));
    pip_desc_add.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
    pip_desc_add.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
    gfx->pip_alpha = sg_make_pipeline(&pip_desc_alpha);
    gfx->pip_add = sg_make_pipeline(&pip_desc_add);

    // a 2048x1 dynamic palette texture
    gfx->img = sg_make_image(&(sg_image_desc){
        .width = 2048,
        .height = 1,
        .pixel_format = SG_PIXELFORMAT_R8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
        .usage = SG_USAGE_STREAM
    });
}

void gfx_shutdown(gfx_t* gfx) {
    assert(gfx);
    for (int i = 0; i < MAX_LAYERS; i++) {
        sg_destroy_buffer(gfx->layers[i].vb);
    }
    sg_destroy_pipeline(gfx->pip_alpha);
    sg_destroy_pipeline(gfx->pip_add);
    sg_destroy_image(gfx->img);
    sg_shutdown();
}

void gfx_new_frame(gfx_t* gfx, float disp_width, float disp_height) {
    assert(gfx);
    gfx->display_size = (float2_t) { disp_width, disp_height };
    gfx->aspect = gfx->display_size.x / gfx->display_size.y;
}

void gfx_begin() {
    sg_pass_action pass_action = {
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = { 0.0f, 0.0f, 0.0f, 1.0f }}
    };
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
}

void gfx_draw(gfx_t* gfx) {
    const float sx = gfx->scale;
    const float sy = gfx->scale * gfx->aspect;
    vs_params_t vs_params = {
        .half_size = (float2_t){(gfx->seg_max_x>>1)/65535.0f, (gfx->seg_max_y>>1)/65535.0f},
        .offset = gfx->offset,
        .scale = (float2_t) { sx, sy },
    };
    sg_update_image(gfx->img, &(sg_image_data){
        .subimage[0][0] = SG_RANGE(gfx->node_state)
    });
    if (gfx->use_additive_blend) {
        sg_apply_pipeline(gfx->pip_add);
    }
    else {
        sg_apply_pipeline(gfx->pip_alpha);
    }
    for (int i = 0; i < MAX_LAYERS; i++) {
        vs_params.color0 = gfx->layer_palette.colors[i];
        if (gfx->layer_visible[i]) {
            sg_apply_bindings(&(sg_bindings){
                .vertex_buffers[0] = gfx->layers[i].vb,
                .vs_images[SLOT_palette_tex] = gfx->img
            });
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
            sg_draw(0, gfx->layers[i].num_elms, 1);
        }
    }
}

void gfx_end() {
    sg_end_pass();
    sg_commit();
}
