//------------------------------------------------------------------------------
//  chipvis.c
//  Chips visualization stuff.
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "chipvis.glsl.h"

static const palette_t default_palette = { {
    { 1.0f, 0.0f, 0.0f, 0.5f },
    { 0.0f, 1.0f, 0.0f, 0.5f },
    { 0.0f, 0.0f, 1.0f, 0.5f },
    { 1.0f, 1.0f, 0.0f, 0.5f },
    { 0.0f, 1.0f, 1.0f, 0.5f },
    { 1.0f, 0.0f, 1.0f, 0.5f },
} };

void chipvis_init(void) {
    // default values
    app.chipvis.aspect = 1.0f;
    app.chipvis.scale = 7.0f;
    app.chipvis.offset = (float2_t) { -0.05f, 0.0f };
    app.chipvis.layer_palette = default_palette;
    for (int i = 0; i < MAX_LAYERS; i++) {
        app.chipvis.layer_visible[i] = true;
    }

    // create vertex buffer from offline-baked vertex data
    for (int i = 0; i < MAX_LAYERS; i++) {
        void* ptr = 0;
        int size = 0;
        switch (i) {
            case 0: ptr = seg_vertices_0; size = sizeof(seg_vertices_0); break;
            case 1: ptr = seg_vertices_1; size = sizeof(seg_vertices_1); break;
            case 2: ptr = seg_vertices_2; size = sizeof(seg_vertices_2); break;
            case 3: ptr = seg_vertices_3; size = sizeof(seg_vertices_3); break;
            case 4: ptr = seg_vertices_4; size = sizeof(seg_vertices_4); break;
            case 5: ptr = seg_vertices_5; size = sizeof(seg_vertices_5); break;
        }
        app.chipvis.layers[i].vb = sg_make_buffer(&(sg_buffer_desc){
            .data = { .ptr = ptr, .size = size }
        });
        app.chipvis.layers[i].num_elms = size / 8;
    }

    // create shaders and pipeline objects
    sg_pipeline_desc pip_desc_alpha = {
        .layout = {
            .attrs = {
                [ATTR_vs_pos] = { .format = SG_VERTEXFORMAT_USHORT2N },
                [ATTR_vs_uv]  = { .format = SG_VERTEXFORMAT_USHORT2N }
            },
        },
        .shader = sg_make_shader(chipvis_alpha_shader_desc(sg_query_backend())),
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
    pip_desc_add.shader = sg_make_shader(chipvis_add_shader_desc(sg_query_backend()));
    pip_desc_add.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_ONE;
    pip_desc_add.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE;
    app.chipvis.pip_alpha = sg_make_pipeline(&pip_desc_alpha);
    app.chipvis.pip_add = sg_make_pipeline(&pip_desc_add);

    // a 2048x1 dynamic palette texture
    app.chipvis.img = sg_make_image(&(sg_image_desc){
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

void chipvis_shutdown(void) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        sg_destroy_buffer(app.chipvis.layers[i].vb);
    }
    sg_destroy_pipeline(app.chipvis.pip_alpha);
    sg_destroy_pipeline(app.chipvis.pip_add);
    sg_destroy_image(app.chipvis.img);
}

void chipvis_draw(void) {
    const float sx = app.chipvis.scale;
    const float sy = app.chipvis.scale * app.chipvis.aspect;
    vs_params_t vs_params = {
        .half_size = (float2_t){(seg_max_x>>1)/65535.0f, (seg_max_y>>1)/65535.0f},
        .offset = app.chipvis.offset,
        .scale = (float2_t) { sx, sy },
    };
    sg_update_image(app.chipvis.img, &(sg_image_data){
        .subimage[0][0] = {
            .ptr = app.chipvis.node_state,
            .size = sizeof(app.chipvis.node_state)
        }
    });
    if (app.chipvis.use_additive_blend) {
        sg_apply_pipeline(app.chipvis.pip_add);
    }
    else {
        sg_apply_pipeline(app.chipvis.pip_alpha);
    }
    for (int i = 0; i < MAX_LAYERS; i++) {
        vs_params.color0 = app.chipvis.layer_palette.colors[i];
        if (app.chipvis.layer_visible[i]) {
            sg_apply_bindings(&(sg_bindings){
                .vertex_buffers[0] = app.chipvis.layers[i].vb,
                .vs_images[SLOT_palette_tex] = app.chipvis.img
            });
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &SG_RANGE(vs_params));
            sg_draw(0, app.chipvis.layers[i].num_elms, 1);
        }
    }
}

