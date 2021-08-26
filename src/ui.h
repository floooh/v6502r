#pragma once
#include "sokol_app.h"
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void ui_init(void);
void ui_shutdown(void);
void ui_frame(void);
void ui_draw(void);
bool ui_input(const sapp_event* event);

#if defined(__cplusplus)
} // extern "C"
#endif
