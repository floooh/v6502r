#pragma once
// common type definitions and constants
#include "common.h"
#include "gfx.h"
#include "pick.h"
#include "trace.h"
#include "sim.h"
#include "asm.h"
#include "ui.h"
#include "util.h"

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
} app_state_t;

extern app_state_t app;

#ifdef __cplusplus
} // extern "C"
#endif
