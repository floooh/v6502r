//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "v6502r.h"

app_state_t app;

static void app_init(void) {
    gfx_init();
    ui_init();
    chipvis_init();
    pick_init();
}

static void app_input(const sapp_event* ev) {
    if (ui_input(ev)) {
        return;
    }
    float w = (float)ev->framebuffer_width * app.chipvis.scale;
    float h = (float) ev->framebuffer_height * app.chipvis.scale * app.chipvis.aspect;
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            app.chipvis.scale += ev->scroll_y * 0.25f;
            if (app.chipvis.scale < 1.0f) {
                app.chipvis.scale = 1.0f;
            }
            else if (app.chipvis.scale > 100.0f) {
                app.chipvis.scale = 100.0f;
            }
            app.input.mouse.x = ev->mouse_x;
            app.input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            app.input.dragging = true;
            app.input.drag_start = (float2_t){ev->mouse_x, ev->mouse_y};
            app.input.offset_start = app.chipvis.offset;
            app.input.mouse.x = ev->mouse_x;
            app.input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            if (app.input.dragging) {
                float dx = ((ev->mouse_x - app.input.drag_start.x) * 2.0f) / w;
                float dy = ((ev->mouse_y - app.input.drag_start.y) * -2.0f) / h;
                app.chipvis.offset.x = app.input.offset_start.x + dx;
                app.chipvis.offset.y = app.input.offset_start.y + dy;
            }
            app.input.mouse.x = ev->mouse_x;
            app.input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            app.input.dragging = false;
            break;
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            app.input.dragging = false;
            break;
        default:
            break;
    }
}

static void app_frame(void) {
    ui_new_frame();
    ui_chipvis();

    // FIXME: tests picking
    float2_t disp_size = { (float)sapp_width(), (float)sapp_height() };
    float2_t scale = { app.chipvis.scale, app.chipvis.scale*app.chipvis.aspect };
    pick_result_t pick_res = pick(app.input.mouse, disp_size, app.chipvis.offset, scale);
    // delete old pick results
    for (int i = 0; i < app.picking.result.num_hits; i++) {
        app.chipvis.node_state[app.picking.result.node_index[i]] = 0;
    }
    app.picking.result = pick_res;
    // highlight new pick results
    for (int i = 0; i < app.picking.result.num_hits; i++) {
        app.chipvis.node_state[app.picking.result.node_index[i]] = 255;
    }

    app.chipvis.aspect = (float)sapp_width()/(float)sapp_height();
    gfx_begin();
    chipvis_draw();
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
        .event_cb = app_input,
        .cleanup_cb = app_cleanup,
        .width = 900,
        .height = 700,
        .window_title = "Visual6502 Remix",
    };
}
