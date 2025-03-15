//------------------------------------------------------------------------------
//  input.c
//
//  Process sokol_app.h events.
//------------------------------------------------------------------------------
#include "input.h"
#include "ui.h"
#include "gfx.h"

static struct {
    bool valid;
    bool dragging;
    float2_t drag_start;
    float2_t offset_start;
    float2_t mouse;
} input;

void input_init(void) {
    assert(!input.valid);
    input.valid = true;
}

void input_shutdown(void) {
    assert(input.valid);
    input.valid = false;
}

float2_t input_get_mouse_pos(void) {
    assert(input.valid);
    return input.mouse;
}

void input_handle_event(const sapp_event* ev) {
    assert(input.valid);
    if (ui_handle_input(ev)) {
        // cancel any chip visualization mouse input if Dear ImGui captured the mouse
        input.mouse.x = 0;
        input.mouse.y = 0;
        input.dragging = false;
        return;
    }
    float w = (float) ev->framebuffer_width * gfx_get_scale() * gfx_get_aspect();
    float h = (float) ev->framebuffer_height * gfx_get_scale();
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_SCROLL:
            gfx_add_scale(ev->scroll_y * 0.25f);
            input.mouse.x = ev->mouse_x;
            input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            input.dragging = true;
            input.drag_start = (float2_t){ev->mouse_x, ev->mouse_y};
            input.offset_start = gfx_get_offset();
            input.mouse.x = ev->mouse_x;
            input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            if (input.dragging) {
                float dx = ((ev->mouse_x - input.drag_start.x) * 2.0f) / w;
                float dy = ((ev->mouse_y - input.drag_start.y) * -2.0f) / h;
                gfx_set_offset((float2_t){
                    .x = input.offset_start.x + dx,
                    .y = input.offset_start.y + dy,
                });
            }
            input.mouse.x = ev->mouse_x;
            input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            input.dragging = false;
            break;
        case SAPP_EVENTTYPE_MOUSE_LEAVE:
            input.dragging = false;
            break;
        default:
            break;
    }
}
