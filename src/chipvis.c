//------------------------------------------------------------------------------
//  chipvis.c
//  Chips visualization stuff.
//------------------------------------------------------------------------------
#include "chipvis.h"
#include "segdefs.h"
#include "sokol_gfx.h"
#include "chipvis.glsl.h"
#define MAX_LAYERS (6)

static struct {
    struct {
        sg_buffer vb;
        int num_elms;
    } layers[MAX_LAYERS];
    sg_pipeline pip;
    sg_image img;
} state;

void chipvis_init(void) {
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
        state.layers[i].vb = sg_make_buffer(&(sg_buffer_desc){
            .content = ptr,
            .size = size
        });
        state.layers[i].num_elms = size / 8;
    }

    // create shader and pipeline object
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs = {
                [ATTR_vs_pos] = { .format = SG_VERTEXFORMAT_USHORT2N },
                [ATTR_vs_uv]  = { .format = SG_VERTEXFORMAT_SHORT2 }
            },
        },
        .shader = sg_make_shader(chipvis_shader_desc()),
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .index_type = SG_INDEXTYPE_NONE,
        .blend = {
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
        }
    });

    // a 2048x1 dynamic palette texture
    state.img = sg_make_image(&(sg_image_desc){
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
        sg_destroy_buffer(state.layers[i].vb);
    }
    sg_destroy_pipeline(state.pip);
}

void chipvis_draw(const chipvis_t* params) {
    const float sx = params->scale;
    const float sy = params->scale * params->aspect;
    vs_params_t vs_params = {
        .half_size = (float2_t){(seg_max_x>>1)/65535.0f, (seg_max_y>>1)/65535.0f},
        .offset = params->offset,
        .scale = (float2_t) { sx, sy },
    };
    sg_update_image(state.img, &(sg_image_content){
        .subimage[0][0] = {
            .ptr = params->node_state,
            .size = sizeof(params->node_state)
        }
    });
    sg_apply_pipeline(state.pip);
    for (int i = 0; i < MAX_LAYERS; i++) {
        vs_params.color0 = params->layer_colors[i];
        if (params->layer_visible[i]) {
            sg_apply_bindings(&(sg_bindings){
                .vertex_buffers[0] = state.layers[i].vb,
                .vs_images[SLOT_palette_tex] = state.img
            });
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
            sg_draw(0, state.layers[i].num_elms, 1);
        }
    }
}

