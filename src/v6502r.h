#pragma once
// common type definitions and constants
#include "ui/ui_memedit.h"
#include "util/m6502dasm.h"
#include "ui/ui_util.h"
#define UI_DASM_USE_M6502
#include "ui/ui_dasm.h"
#include "sokol_app.h"
#include "common.h"
#include "gfx.h"
#include "pick.h"
#include "trace.h"
#include "sim.h"
#include "asm.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "nodenames.h"
#include "segdefs.h"

typedef struct {
    struct {
        bool dragging;
        float2_t drag_start;
        float2_t offset_start;
        float2_t mouse;
    } input;
    pick_t pick;
    gfx_t gfx;
    trace_t trace;
    sim_t sim;
    struct {
        ui_memedit_t memedit;
        ui_memedit_t memedit_integrated;
        ui_dasm_t dasm;
        bool cpu_controls_open;
        bool tracelog_open;
        bool asm_open;
        bool listing_open;
        bool help_asm_open;
        bool help_opcodes_open;
        bool help_about_open;
        bool open_source_hovered;
        bool save_source_hovered;
        bool save_binary_hovered;
        bool save_listing_hovered;
        bool cut_hovered;
        bool copy_hovered;
        bool link_hovered;
        char link_url[MAX_LINKURL_SIZE];
    } ui;
    struct {
        uint32_t num_bytes;
        uint8_t buf[MAX_BINARY_SIZE];
    } binary;
} app_state_t;

extern app_state_t app;

void ui_init(void);
void ui_shutdown(void);
void ui_frame(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);

void ui_asm_init(void);
void ui_asm_discard(void);
void ui_asm_draw(void);
void ui_asm_undo(void);
void ui_asm_redo(void);
void ui_asm_cut(void);
void ui_asm_copy(void);
void ui_asm_paste(void);
void ui_asm_assemble(void);
const char* ui_asm_source(void);
void ui_asm_put_source(const char* name, const uint8_t* bytes, int num_bytes);

void util_init(void);
const char* util_opcode_to_str(uint8_t op);
void util_html5_download_string(const char* filename, const char* content);
void util_html5_download_binary(const char* filename, const uint8_t* bytes, uint32_t num_bytes);
void util_html5_load(void);
void util_html5_open_link(const char* url);
void util_html5_cursor_to_pointer(void);
void util_html5_cursor_to_default(void);
bool util_is_osx(void);

#ifdef __cplusplus
} // extern "C"
#endif
