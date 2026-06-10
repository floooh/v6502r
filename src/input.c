//------------------------------------------------------------------------------
//  input.c
//
//  Process sokol_app.h events.
//------------------------------------------------------------------------------
#include "input.h"
#include "ui.h"
#include "gfx.h"
#include <math.h>

static struct {
    bool valid;
    bool dragging;
    float2_t drag_world_anchor;
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
    const float2_t mouse = (float2_t){ ev->mouse_x, ev->mouse_y };
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_SCROLL: {
                float2_t pivot = gfx_transform_mouse(mouse, gfx_get_offset());
                gfx_set_scale(gfx_get_scale() * powf(1.05f, ev->scroll_y));
                gfx_set_offset(gfx_transform_mouse(mouse, pivot));
                input.mouse.x = ev->mouse_x;
                input.mouse.y = ev->mouse_y;
            }
            break;
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            input.dragging = true;
            input.drag_world_anchor = gfx_transform_mouse(mouse, gfx_get_offset());
            input.mouse.x = ev->mouse_x;
            input.mouse.y = ev->mouse_y;
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE:
            if (input.dragging) {
                gfx_set_offset(gfx_transform_mouse(mouse, input.drag_world_anchor));
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
