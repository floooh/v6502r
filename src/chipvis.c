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
        state.layers[i].num_elms = size / 4;
    }

    // create shader and pipeline object
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .layout = {
            .attrs = {
                [ATTR_vs_pos] = { .format = SG_VERTEXFORMAT_USHORT2N },
            },
        },
        .shader = sg_make_shader(chipvis_shader_desc()),
        .primitive_type = SG_PRIMITIVETYPE_LINES,
        .index_type = SG_INDEXTYPE_NONE,
        .blend = {
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
        }
    });
}

void chipvis_shutdown(void) {
    for (int i = 0; i < MAX_LAYERS; i++) {
        sg_destroy_buffer(state.layers[i].vb);
    }
    sg_destroy_pipeline(state.pip);
}

void chipvis_draw(const chipvis_t* params) {
    sg_apply_pipeline(state.pip);
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (params->layer_visible[i]) {
            sg_apply_bindings(&(sg_bindings){
                .vertex_buffers[0] = state.layers[i].vb
            });
            vs_params_t vs_params = {
                .color0 = params->layer_colors[i],
                .offset = (float2_t) { params->offset.x, params->offset.y },
                .scale = (float2_t) { params->scale, params->scale * params->aspect }
            };
            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params, sizeof(vs_params));
            sg_draw(0, state.layers[i].num_elms, 1);
        }
    }
}

