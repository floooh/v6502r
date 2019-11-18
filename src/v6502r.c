//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "v6502r.h"

app_state_t app;

/* example program from visual6502.org:
                   * = 0000
 0000   A9 00      LDA #$00
 0002   20 10 00   JSR $0010
 0005   4C 02 00   JMP $0002
 0008   00         BRK
 0009   00         BRK
 000A   00         BRK
 000B   00         BRK
 000C   00         BRK
 000D   00         BRK
 000E   00         BRK
 000F   43         ???
 0010   E8         INX
 0011   88         DEY
 0012   E6 0F      INC $0F
 0014   38         SEC
 0015   69 02      ADC #$02
 0017   60         RTS
 0018              .END
*/
static uint8_t test_prog[] = {
    0xa9, 0x00, 0x20, 0x10, 0x00, 0x4c, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43,
    0xe8, 0x88, 0xe6, 0x0f, 0x38, 0x69, 0x02, 0x60
};

static void app_init(void) {
    util_init();
    gfx_init();
    ui_init();
    chipvis_init();
    pick_init();
    trace_init();
    sim_init_or_reset();
    sim_write(0x0000, sizeof(test_prog), test_prog);
    sim_start();
}

static void app_frame(void) {
    app.chipvis.aspect = (float)sapp_width()/(float)sapp_height();
    ui_frame();
    sim_frame();
    pick_frame();
    gfx_begin();
    chipvis_draw();
    ui_draw();
    gfx_end();
}

static void app_input(const sapp_event* ev) {
    if (ui_input(ev)) {
        return;
    }
    float w = (float) ev->framebuffer_width * app.chipvis.scale;
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

static void app_cleanup(void) {
    sim_shutdown();
    trace_shutdown();
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
        .enable_clipboard = true,
        .clipboard_size = 16*1024
    };
}
