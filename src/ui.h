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

void ui_init(void);
void ui_shutdown(void);
void ui_frame(void);
void ui_draw(void);
bool ui_handle_input(const sapp_event* event);
bool ui_is_diffview(void);
ui_diffview_t ui_get_diffview(void);

#if defined(__cplusplus)
} // extern "C"
#endif
