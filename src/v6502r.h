#pragma once
// common type definitions and constants
#include <assert.h>
#include <stdbool.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_args.h"
#include "ui/ui_memedit.h"
#include "util/m6502dasm.h"
#include "ui/ui_util.h"
#define UI_DASM_USE_M6502
#include "ui/ui_dasm.h"
#include "sokol_gfx_imgui.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "nodenames.h"
#include "segdefs.h"

#define MAX_LAYERS (6)
#define PICK_MAX_HITS (16)
#define MAX_NODES (2048)
#define MAX_TRACE_ITEMS (256)
#define TRACE_FLIPBIT_CLK0 (1<<0)
#define TRACE_FLIPBIT_OP (1<<1)
#define MAX_BINARY_SIZE ((1<<16)+2)

typedef struct {
    float x, y, z, w;
} float4_t;

typedef struct {
    float x, y;
} float2_t;

typedef struct {
    float4_t colors[MAX_LAYERS];
} palette_t;

typedef struct {
    int num_hits;
    int node_index[PICK_MAX_HITS];
} pick_result_t;

typedef struct {
    uint32_t cycle;
    uint32_t flip_bits;     // for grouping instruction and cycle ticks
    uint32_t node_values[64];
    uint32_t transistors_on[128];
    uint8_t mem[4096];      // only trace the first few KB of memory
} trace_item_t;

typedef struct {
    struct {
        bool paused;
    } sim;
    struct {
        bool dragging;
        float2_t drag_start;
        float2_t offset_start;
        float2_t mouse;
    } input;
    struct {
        pick_result_t result;
        bool layer_enabled[MAX_LAYERS];
    } picking;
    struct {
        struct {
            sg_buffer vb;
            int num_elms;
        } layers[MAX_LAYERS];
        sg_pipeline pip_alpha;
        sg_pipeline pip_add;
        sg_image img;
        float aspect;
        float scale;
        float2_t scale_pivot;
        float2_t offset;
        bool use_additive_blend;
        palette_t layer_palette;
        bool layer_visible[MAX_LAYERS];
        uint8_t node_state[MAX_NODES];
    } chipvis;
    struct {
        ui_memedit_t memedit;
        ui_memedit_t memedit_integrated;
        ui_dasm_t dasm;
        sg_imgui_t sg_imgui;
        bool cpu_controls_open;
        bool tracelog_open;
        bool asm_open;
        bool tracelog_scroll_to_end;
        bool open_source_hovered;
        bool save_source_hovered;
        bool save_binary_hovered;
        bool save_listing_hovered;
    } ui;
    struct {
        bool hovered;               // true if mouse is currently hovering a UI trace item
        bool selected;              // true if a trace item is currently selected in the UI
        uint32_t hovered_cycle;     // mouse-hovered cycle number
        uint32_t selected_cycle;    // selected cycle number
        uint32_t flip_bits;         // for visually separating instructions and cycles
        uint32_t head;
        uint32_t tail;
        trace_item_t items[MAX_TRACE_ITEMS];
    } trace;
    struct {
        uint32_t num_bytes;
        uint8_t buf[MAX_BINARY_SIZE];
    } binary;
} app_state_t;

extern app_state_t app;

void gfx_init(void);
void gfx_shutdown(void);
void gfx_begin(void);
void gfx_end(void);

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

void chipvis_init(void);
void chipvis_draw(void);
void chipvis_shutdown(void);

void sim_init_or_reset(void);
void sim_shutdown(void);
void sim_start(uint16_t start_addr);
void sim_frame(void);
void sim_pause(bool paused);
bool sim_paused(void);
void sim_step(int num_half_cycles);
void sim_step_op(void);
void sim_update_nodestate(void);
void sim_w8(uint16_t addr, uint8_t val);
uint8_t sim_r8(uint16_t addr);
void sim_w16(uint16_t addr, uint16_t val);
uint16_t sim_r16(uint16_t addr);
void sim_clear(uint16_t addr, uint16_t num_bytes);
void sim_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr);
uint32_t sim_get_cycle(void);
void sim_set_cycle(uint32_t c);
uint8_t sim_get_a(void);
uint8_t sim_get_x(void);
uint8_t sim_get_y(void);
uint8_t sim_get_sp(void);
uint16_t sim_get_pc(void);
uint8_t sim_get_ir(void);
uint8_t sim_get_p(void);
bool sim_get_clk0(void);
bool sim_get_rw(void);
bool sim_get_sync(void);
uint16_t sim_get_addr(void);
uint8_t sim_get_data(void);
bool sim_read_node_values(uint8_t* ptr, int max_bytes);
bool sim_read_transistor_on(uint8_t* ptr, int max_bytes);
void sim_write_node_values(const uint8_t* ptr, int max_bytes);
void sim_write_transistor_on(const uint8_t* ptr, int max_bytes);

void trace_init(void);
void trace_shutdown(void);
void trace_clear(void);
void trace_store(void);
bool trace_revert_to_previous(void);
bool trace_revert_to_selected(void);
uint32_t trace_num_items(void);  // number of stored trace items
bool trace_empty(void);
uint32_t trace_get_cycle(uint32_t index);
uint32_t trace_get_flipbits(uint32_t index);
uint8_t trace_get_a(uint32_t index);
uint8_t trace_get_x(uint32_t index);
uint8_t trace_get_y(uint32_t index);
uint8_t trace_get_sp(uint32_t index);
uint16_t trace_get_pc(uint32_t index);
uint8_t trace_get_ir(uint32_t index);
uint8_t trace_get_p(uint32_t index);
bool trace_get_clk0(uint32_t index);
bool trace_get_rw(uint32_t index);
bool trace_get_sync(uint32_t index);
bool trace_get_irq(uint32_t index);
bool trace_get_nmi(uint32_t index);
bool trace_get_res(uint32_t index);
uint16_t trace_get_addr(uint32_t index);
uint8_t trace_get_data(uint32_t index);

void pick_init(void);
void pick_frame(void);
pick_result_t pick(float2_t mouse_pos, float2_t disp_size, float2_t offset, float2_t scale);

typedef struct {
    const char* filename;
    int line_nr;                // 1-based!
    bool warning;               // true: warning, false: error
    const char* msg;
} asm_error_t;

typedef struct {
    bool errors;
    uint16_t addr;
    uint16_t len;
    const uint8_t* bytes;
} asm_result_t;

void asm_init(void);
void asm_source_open(void);
void asm_source_write(const char* src, uint32_t tab_width);
void asm_source_close(void);
asm_result_t asm_assemble(void);
int asm_num_errors(void);
const asm_error_t* asm_error(int index);
const char* asm_stderr(void);
const char* asm_listing(void);
const char* asm_source(void);

void util_init(void);
const char* util_opcode_to_str(uint8_t op);
bool util_is_mac(void);
void util_html5_download_string(const char* filename, const char* content);
void util_html5_download_binary(const char* filename, const uint8_t* bytes, uint32_t num_bytes);
void util_html5_load(void);

#ifdef __cplusplus
} // extern "C"
#endif
