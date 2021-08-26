#pragma once
#include "sokol_app.h"
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

void input_init(void);
void input_shutdown(void);
void input_handle_event(const sapp_event* ev);
float2_t input_get_mouse_pos(void);

#if defined(__cplusplus)
} // extern "C"
#endif
