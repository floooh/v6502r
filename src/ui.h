#pragma once
#include "sokol_app.h"

#ifdef __cplusplus
extern "C" {
#endif

// main UI functions
void ui_setup(void);
void ui_discard(void);
void ui_begin(void);
void ui_end(void);
void ui_input(const sapp_event* event);

// per-window drawing functions
void ui_hello(void);

#ifdef __cplusplus
} // extern "C"
#endif

