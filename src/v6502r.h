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
#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "nodenames.h"
#include "segdefs.h"

#define MAX_LAYERS (6)
#define PICK_MAX_HITS (16)
#define MAX_NODES (2048)

typedef struct {
    float x, y, z, w;
} float4_t;

typedef struct {
    float x, y;
} float2_t;

typedef struct {
    int num_hits;
    int node_index[PICK_MAX_HITS];
} pick_result_t;

typedef struct {
    struct {
        bool dragging;
        float2_t drag_start;
        float2_t offset_start;
        float2_t mouse;
        const char* open_url;
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
        sg_pipeline pip;
        sg_image img;
        float aspect;
        float scale;
        float2_t scale_pivot;
        float2_t offset;
        float4_t layer_colors[MAX_LAYERS];
        bool layer_visible[MAX_LAYERS];
        uint8_t node_state[MAX_NODES];
    } chipvis;
    struct {
        ui_memedit_t memedit;
        ui_dasm_t dasm;
    } ui;
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

void chipvis_init(void);
void chipvis_draw(void);
void chipvis_shutdown(void);

void sim_init_or_reset(void);
void sim_shutdown(void);
void sim_start(uint16_t start_addr);
void sim_step(int num_half_cycles);
void sim_update_nodestate(void);
void sim_w8(uint16_t addr, uint8_t val);
uint8_t sim_r8(uint16_t addr);
void sim_w16(uint16_t addr, uint16_t val);
uint16_t sim_r16(uint16_t addr);
void sim_write(uint16_t addr, uint16_t num_bytes, const uint8_t* ptr);

void pick_init(void);
pick_result_t pick(float2_t mouse_pos, float2_t disp_size, float2_t offset, float2_t scale);

#ifdef __cplusplus
} // extern "C"
#endif
