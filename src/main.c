//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_args.h"
#include "util.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"
#include "pick.h"
#include "trace.h"
#include "sim.h"
#include "segdefs.h"

#if defined(CHIP_6502)
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
#elif defined(CHIP_Z80)

/*
0000                LOOP:
0000   31 00 01               LD   sp,$0100
0003   CD 09 00               CALL   sub
0006   C3 00 00               JP   loop
0009                SUB:
0009   21 0E 00               LD   hl,data
000C   34                     INC   (hl)
000D   C9                     RET
000E                DATA:
000E   00                     DB   0
*/
static uint8_t test_prog[] = {
    0x31, 0x00, 0x01,
    0xCD, 0x09, 0x00,
    0xC3, 0x00, 0x00,
    0x21, 0x0E, 0x00,
    0x34,
    0xC9,
};
#endif

static void app_init(void) {
    util_init();
    gfx_init(&(gfx_desc_t){
        .seg_vertices = {
            [0] = SG_RANGE(seg_vertices_0),
            [1] = SG_RANGE(seg_vertices_1),
            [2] = SG_RANGE(seg_vertices_2),
            [3] = SG_RANGE(seg_vertices_3),
            [4] = SG_RANGE(seg_vertices_4),
            [5] = SG_RANGE(seg_vertices_5),
        },
        .seg_min_x = seg_min_x,
        .seg_min_y = seg_min_y,
        .seg_max_x = seg_max_x,
        .seg_max_y = seg_max_y,
    });
    input_init();
    ui_init();
    pick_init(&(pick_desc_t){
        .seg_min_x = seg_min_x,
        .seg_min_y = seg_min_y,
        .seg_max_x = seg_max_x,
        .seg_max_y = seg_max_y,
        .layers = {
            [0] = {
                .verts = (const pick_vertex_t*) seg_vertices_0,
                .num_verts = sizeof(seg_vertices_0) / 8,
            },
            [1] = {
                .verts = (const pick_vertex_t*) seg_vertices_1,
                .num_verts = sizeof(seg_vertices_1) / 8,
            },
            [2] = {
                .verts = (const pick_vertex_t*) seg_vertices_2,
                .num_verts = sizeof(seg_vertices_2) / 8,
            },
            [3] = {
                .verts = (const pick_vertex_t*) seg_vertices_3,
                .num_verts = sizeof(seg_vertices_3) / 8,
            },
            [4] = {
                .verts = (const pick_vertex_t*) seg_vertices_4,
                .num_verts = sizeof(seg_vertices_4) / 8,
            },
            [5] = {
                .verts = (const pick_vertex_t*) seg_vertices_5,
                .num_verts = sizeof(seg_vertices_5) / 8,
            },
        },
        .mesh = {
            .tris = (const pick_triangle_t*) pick_tris,
            .num_tris = sizeof(pick_tris) / 8,
        },
        .grid = {
            .num_cells_x = grid_cells,
            .num_cells_y = grid_cells,
            .cells = (const pick_cell_t*) pick_grid,
            .num_cells = sizeof(pick_grid) / 8,
        }
    });
    trace_init();
    sim_init();
    sim_write(0x0000, sizeof(test_prog), test_prog);
    sim_start();
}

static void app_frame(void) {
    gfx_new_frame(sapp_widthf(), sapp_heightf());
    ui_frame();
    sim_frame();
    sim_get_node_state(gfx_get_nodestate_buffer());
    const pick_result_t pick_result = pick_dopick(
        input_get_mouse_pos(),
        gfx_get_display_size(),
        gfx_get_offset(),
        gfx_get_aspect(),
        gfx_get_scale()
    );
    for (int i = 0; i < pick_result.num_hits; i++) {
        gfx_highlight_node(pick_result.node_index[i]);
    }
    gfx_begin();
    gfx_draw();
    ui_draw();
    gfx_end();
}

static void app_input(const sapp_event* ev) {
    input_handle_event(ev);
}

static void app_shutdown(void) {
    pick_shutdown();
    sim_shutdown();
    trace_shutdown();
    ui_shutdown();
    input_shutdown();
    gfx_shutdown();
    util_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sargs_setup(&(sargs_desc){ .argc = argc, .argv = argv });
    return (sapp_desc){
        .init_cb = app_init,
        .frame_cb = app_frame,
        .event_cb = app_input,
        .cleanup_cb = app_shutdown,
        .width = 900,
        .height = 700,
        .window_title = WINDOW_TITLE,
        .enable_clipboard = true,
        .clipboard_size = 16*1024,
        .icon = {
            .sokol_default = true
        }
    };
}
