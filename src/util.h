#pragma once
#include "common.h"

#if defined (__cplusplus)
extern "C" {
#endif

void util_init(void);
void util_shutdown(void);
void util_html5_download_string(const char* filename, const char* content);
void util_html5_download_binary(const char* filename, range_t bytes);
void util_html5_load(void);
void util_html5_open_link(const char* url);
bool util_is_osx(void);
void util_save_settings(const char* key, const char* payload);
const char* util_load_settings(const char* key);
void util_free_settings(const char* payload);

#if defined(__cplusplus)
} // extern "C"
#endif
