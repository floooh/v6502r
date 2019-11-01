//------------------------------------------------------------------------------
//  gfx.c
//------------------------------------------------------------------------------
#include "v6502r.h"

void gfx_init(void) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 16,
        .image_pool_size = 8,
        .pipeline_pool_size = 8,
        .mtl_device = sapp_metal_get_device(),
        .mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor,
        .mtl_drawable_cb = sapp_metal_get_drawable,
        .d3d11_device = sapp_d3d11_get_device(),
        .d3d11_device_context = sapp_d3d11_get_device_context(),
        .d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view,
        .d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view
    });
}

void gfx_shutdown(void) {
    sg_shutdown();
}

void gfx_begin(void) {
    sg_pass_action pass_action = {
        .colors[0] = { .action = SG_ACTION_CLEAR, .val = { 0.0f, 0.0f, 0.0f, 1.0f }}
    };
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
}

void gfx_end(void) {
    sg_end_pass();
    sg_commit();
}
