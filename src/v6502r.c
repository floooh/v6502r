//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "sokol_args.h"
#include "sokol_app.h"
#include "ui.h"

static void app_init(void) {
    ui_setup();
}

static void app_frame(void) {
    ui_begin();
//    ui_hello();
    ui_chip();
    ui_end();
}

static void app_cleanup(void) {
    ui_discard();
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
