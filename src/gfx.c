//------------------------------------------------------------------------------
//  gfx.c
//  sokol-gfx init/shutdown, frame start/end.
//------------------------------------------------------------------------------
#include "v6502r.h"

void gfx_init(void) {
    sg_setup(&(sg_desc){
        .buffer_pool_size = 16,
        .image_pool_size = 8,
        .pipeline_pool_size = 8,
        .context = sapp_sgcontext()
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
