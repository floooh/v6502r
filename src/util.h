#pragma once
#include "common.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef void (*util_load_callback_t)(bool succeeded, const void* data, uint32_t num_bytes, void* userdata);
typedef void (*util_save_callback_t)(bool succeeded, void* userdata);

typedef struct {
    const char* key;
    util_load_callback_t completed;
    void* userdata;
} util_load_t;

typedef struct {
    const char* key;
    const void* bytes;
    size_t num_bytes;
    util_save_callback_t completed;
    void* userdata;
} util_save_t;

void util_init(void);
void util_shutdown(void);
void util_html5_download_string(const char* filename, const char* content);
void util_html5_download_binary(const char* filename, range_t bytes);
void util_html5_load(void);
void util_html5_open_link(const char* url);
bool util_is_osx(void);
void util_save_async(util_save_t args);
void util_load_async(util_load_t args);

#if defined(__cplusplus)
} // extern "C"
#endif
