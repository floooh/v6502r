//------------------------------------------------------------------------------
//  v6502r.c
//  Main source file.
//------------------------------------------------------------------------------
#include "sokol_app.h"
#include "sokol_args.h"
#include "sokol_log.h"
#include "util.h"
#include "gfx.h"
#include "input.h"
#include "ui.h"
#include "ui_asm.h"
#include "pick.h"
#include "trace.h"
#include "sim.h"
#include "segdefs.h"

const char* init_src =
#if defined(CHIP_6502)
    "\tlda #0\n"
    "loop:\tjsr func\n"
    "\tjmp loop\n"
    "func:\tinx\n"
    "\tdey\n"
    "\tinc data\n"
    "\tsec\n"
    "\tadc #2\n"
    "\trts\n"
    "data:\tdb $40\n\n";
#elif defined(CHIP_Z80)
    "\tld sp,stack\n"
    "loop:\tcall func\n"
    "\tjr loop\n"
    "func:\tld hl,data\n"
    "\tinc (hl)\n"
    "\tret\n"
    "data:\tdb 40h\n"
    "stack:\torg 30h\n";
#elif defined(CHIP_2A03)
    "\tlda #0\n"
    "loop:\tjsr func\n"
    "\tjmp loop\n"
    "func:\tinx\n"
    "\tdey\n"
    "\tinc data\n"
    "\tsec\n"
    "\tadc #2\n"
    "\trts\n"
    "data:\tdb $40\n\n";
#endif

static void app_init(void) {
    util_init();
    gfx_preinit();
    ui_init();
    gfx_init(&(gfx_desc_t){
        .seg_vertices = {
            [0] = SG_RANGE(seg_vertices_0),
            [1] = SG_RANGE(seg_vertices_1),
            #if !defined(CHIP_Z80)
            [2] = SG_RANGE(seg_vertices_2),
            #endif
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
            #if !defined(CHIP_Z80)
            [2] = {
                .verts = (const pick_vertex_t*) seg_vertices_2,
                .num_verts = sizeof(seg_vertices_2) / 8,
            },
            #endif
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
    ui_asm_put_source("start", (range_t){ .ptr = (void*)init_src, strlen(init_src) });
    ui_asm_assemble();
    sim_start();
}

static void app_frame(void) {
    gfx_new_frame(sapp_widthf(), sapp_heightf());
    ui_frame();
    sim_frame();
    if (ui_is_nodeexplorer_active()) {
        ui_write_nodeexplorer_visual_state(gfx_get_nodestate_buffer());
    }
    else if (ui_is_diffview()) {
        const ui_diffview_t diffview = ui_get_diffview();
        trace_write_diff_visual_state(diffview.cycle0, diffview.cycle1, gfx_get_nodestate_buffer());
    }
    else {
        sim_write_node_visual_state(gfx_get_nodestate_buffer());
    }
    const pick_result_t pick_result = pick_dopick(
        input_get_mouse_pos(),
        gfx_get_display_size(),
        gfx_get_offset(),
        gfx_get_aspect(),
        gfx_get_scale()
    );
    for (int i = 0; i < pick_result.num_hits; i++) {
        if (!sim_is_ignore_picking_highlight_node(pick_result.node_index[i])) {
            gfx_highlight_node(pick_result.node_index[i]);
        }
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
        .sample_count = 4,
        .high_dpi = sargs_boolean("highdpi"),
        .clipboard_size = 16*1024,
        .icon = {
            .sokol_default = true
        },
        .logger = {
            .func = slog_func,
        }
    };
}
