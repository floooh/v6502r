//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "sokol_args.h"
#include "sokol_app.h"
#include "gfx.h"
#include "ui.h"
#include "chipvis.h"

static struct {
    chipvis_t chipvis;
    bool dragging;
    float2_t drag_start;
    float2_t offset_start;
} state;

static void app_init(void) {
    gfx_init();
    ui_init();
    chipvis_init();
    state.chipvis = (chipvis_t){
        .aspect = 1.0f,
        .scale = 5.0f,
        .offset = { 0.0f, 0.0f },
        .layer_colors = {
            { 1.0f, 0.0f, 0.0f, 0.5f },
            { 0.0f, 1.0f, 0.0f, 0.5f },
            { 0.0f, 0.0f, 1.0f, 0.5f },
            { 1.0f, 1.0f, 0.0f, 0.5f },
            { 0.0f, 1.0f, 1.0f, 0.5f },
            { 1.0f, 0.0f, 1.0f, 0.5f },
        },
        .layer_visible = {
            true, true, true, true, true, true
        },
    };
}

static void app_input(const sapp_event* ev) {
    if (ui_input(ev)) {
        return;
    }
    float w = (float)ev->framebuffer_width * state.chipvis.scale;
    float h = (float) ev->framebuffer_height * state.chipvis.scale * state.chipvis.aspect;
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            state.chipvis.scale += ev->scroll_y * 0.25f;
            if (state.chipvis.scale < 1.0f) {
                state.chipvis.scale = 1.0f;
            }
            else if (state.chipvis.scale > 100.0f) {
                state.chipvis.scale = 100.0f;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            state.dragging = true;
            state.drag_start = (float2_t){ev->mouse_x, ev->mouse_y};
            state.offset_start = state.chipvis.offset;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            if (state.dragging) {
                float dx = ((ev->mouse_x - state.drag_start.x) * 2.0f) / w;
                float dy = ((ev->mouse_y - state.drag_start.y) * -2.0f) / h;
                state.chipvis.offset.x = state.offset_start.x + dx;
                state.chipvis.offset.y = state.offset_start.y + dy;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            state.dragging = false;
            break;
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            state.dragging = false;
            break;
        default:
            break;
    }
}

static void app_frame(void) {
    ui_new_frame();
    ui_chipvis(&state.chipvis);

    // FIXME
    static uint32_t i = 0;
    state.chipvis.node_state[(i-10) % 1725] = 0;
    state.chipvis.node_state[i] = 255;
    i = (i + 1) % 1735;

    state.chipvis.aspect = (float)sapp_width()/(float)sapp_height();
    gfx_begin();
    chipvis_draw(&state.chipvis);
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
