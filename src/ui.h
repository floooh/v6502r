#pragma once
#include "sokol_app.h"
#include "chipvis.h"

#ifdef __cplusplus
extern "C" {
#endif

// main UI functions
void ui_init(void);
void ui_shutdown(void);
void ui_new_frame(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);

// per-window drawing functions
void ui_chipvis(chipvis_t* params);

#ifdef __cplusplus
} // extern "C"
#endif

