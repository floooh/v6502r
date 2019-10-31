//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "sokol_args.h"
#include "sokol_app.h"
#include "gfx.h"
#include "ui.h"
#include "chipvis.h"

static chipvis_t chipvis_params = {
    .aspect = 1.0f,
    .scale = 5.0f,
    .offset = { -0.5f, -0.5f },
    .layer_colors = {
        { 1.0f, 0.0f, 0.0f, 0.25f },
        { 0.0f, 1.0f, 0.0f, 0.25f },
        { 0.0f, 0.0f, 1.0f, 0.25f },
        { 1.0f, 1.0f, 0.0f, 0.25f },
        { 0.0f, 1.0f, 1.0f, 0.25f },
        { 1.0f, 0.0f, 1.0f, 0.25f },
    },
    .layer_visible = {
        true, true, true, true, true, true
    },
};

static void app_init(void) {
    gfx_init();
    ui_init();
    chipvis_init();
}

static void app_frame(void) {
    ui_new_frame();
    ui_chipvis(&chipvis_params);
    chipvis_params.aspect = (float)sapp_width()/(float)sapp_height();
    gfx_begin();
    chipvis_draw(&chipvis_params);
    ui_draw();
    gfx_end();
}

static void app_cleanup(void) {
    chipvis_shutdown();
    ui_shutdown();
    gfx_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc = argc, .argv = argv });
    return (sapp_desc){
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = ui_input,
        .cleanup_cb = app_cleanup,
        .width = 900,
        .height = 700,
        .window_title = "Visual6502",
    };
}
