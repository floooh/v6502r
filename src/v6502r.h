#pragma once
// common type definitions and constants
#include <assert.h>
#include <stdbool.h>
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_args.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "segdefs.h"

#define MAX_LAYERS (6)
#define PICK_MAX_HITS (16)

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
        uint8_t node_state[2048];
    } chipvis;
} app_state_t;

extern app_state_t app;

void gfx_init(void);
void gfx_shutdown(void);
void gfx_begin(void);
void gfx_end(void);

void ui_init(void);
void ui_shutdown(void);
void ui_new_frame(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);
void ui_chipvis(void);

void chipvis_init(void);
void chipvis_draw(void);
void chipvis_shutdown(void);

void pick_init(void);
pick_result_t pick(float2_t mouse_pos, float2_t disp_size, float2_t offset, float2_t scale);

#ifdef __cplusplus
} // extern "C"
#endif
