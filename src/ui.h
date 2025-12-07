#pragma once
#include "sokol_app.h"
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    uint32_t cycle0;
    uint32_t cycle1;
} ui_diffview_t;

typedef enum {
    UI_WINDOW_INVALID = 0,
    UI_WINDOW_FLOATING_CONTROLS,    // keep at start
    UI_WINDOW_CPU_CONTROLS,
    UI_WINDOW_TRACELOG,
    UI_WINDOW_TIMING_DIAGRAM,
    UI_WINDOW_NODE_EXPLORER,
    UI_WINDOW_LISTING,
    UI_WINDOW_ASM,
    UI_WINDOW_MEMEDIT,
    UI_WINDOW_DASM,
    UI_WINDOW_HELP_ASM,
    UI_WINDOW_HELP_OPCODES,
    UI_WINDOW_HELP_ABOUT,
    #if defined(CHIP_Z80)
    UI_WINDOW_IOEDIT,
    #endif
    #if defined(CHIP_2A03)
    UI_WINDOW_AUDIO,
    #endif
    UI_WINDOW_NUM,
} ui_window_t;

void ui_init(void);
void ui_shutdown(void);
void ui_init_window_open(ui_window_t win, bool state);
void ui_set_window_open(ui_window_t win, bool state);
void ui_toggle_window(ui_window_t win);
bool ui_is_window_open(ui_window_t win);
void ui_set_keyboard_target(ui_window_t win);
const char* ui_window_id(ui_window_t win);
ui_window_t ui_window_by_id(const char* id);
void ui_check_dirty(ui_window_t win);
bool* ui_window_open_ptr(ui_window_t win);
void ui_frame(void);
void ui_draw(void);
bool ui_handle_input(const sapp_event* event);
bool ui_is_diffview(void);
ui_diffview_t ui_get_diffview(void);
bool ui_is_nodeexplorer_active(void);
void ui_write_nodeexplorer_visual_state(range_t to_buffer);
const char* ui_trace_get_dump(void);

#if defined(__cplusplus)
} // extern "C"
#endif
