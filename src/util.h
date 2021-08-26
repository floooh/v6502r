#pragma once
#include "common.h"

#if defined (__cplusplus)
extern "C" {
#endif

void util_init(void);
void util_shutdown(void);
const char* util_opcode_to_str(uint8_t op);
void util_html5_download_string(const char* filename, const char* content);
void util_html5_download_binary(const char* filename, range_t bytes);
void util_html5_load(void);
void util_html5_open_link(const char* url);
void util_html5_cursor_to_pointer(void);
void util_html5_cursor_to_default(void);
bool util_is_osx(void);

#if defined(__cplusplus)
} // extern "C"
#endif
