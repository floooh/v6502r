#pragma once
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

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
void ui_asm_put_source(const char* name, range_t src);
bool ui_asm_is_window_focused(void);
range_t ui_asm_get_binary(void);

#if defined(__cplusplus)
} // extern "C"
#endif
