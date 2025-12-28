//------------------------------------------------------------------------------
//  util.c
//------------------------------------------------------------------------------
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#elif defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows.h"
#endif
#include "ui.h"
#include "ui_asm.h"
#include "util.h"
#include <stdlib.h> // free()
#include <stdarg.h>
#include <stdio.h>

static struct {
    bool valid;
    bool is_osx;
} util;

#if defined(__EMSCRIPTEN__)
EM_JS_DEPS(v6502r, "$ccall,$UTF8ToString,$stringToNewUTF8");

EM_JS(void, emsc_js_init, (void), {
    Module['emsc_js_onload'] = emsc_js_onload;
});

EM_JS(int, emsc_js_is_osx, (void), {
    if (navigator.userAgent.includes('Macintosh')) {
        return 1;
    }
    else {
        return 0;
    }
});

EM_JS(void, emsc_js_download_string, (const char* c_filename, const char* c_content), {
    const filename = UTF8ToString(c_filename);
    const content = UTF8ToString(c_content);
    const elm = document.createElement('a');
    elm.setAttribute('href', 'data:text/plain;charset=utf-8,'+encodeURIComponent(content));
    elm.setAttribute('download', filename);
    elm.style.display='none';
    document.body.appendChild(elm);
    elm.click();
    document.body.removeChild(elm);
});

EM_JS(void, emsc_js_download_binary, (const char* c_filename, const uint8_t* ptr, int num_bytes), {
    const filename = UTF8ToString(c_filename);
    let binary = "";
    for (var i = 0; i < num_bytes; i++) {
        binary += String.fromCharCode(HEAPU8[ptr+i]);
    }
    console.log(btoa(binary));
    const elm = document.createElement('a');
    elm.setAttribute('href', 'data:application/octet-stream;base64,'+btoa(binary));
    elm.setAttribute('download', filename);
    elm.style.display='none';
    document.body.appendChild(elm);
    elm.click();
    document.body.removeChild(elm);
});

EM_JS(void, emsc_js_load, (void), {
    document.getElementById('picker').click();
});

EM_JS(void, emsc_js_onload, (void), {
    const picker = document.getElementById('picker');
    // reset the file picker
    const file = picker.files[0];
    picker.value = null;
    console.log('--- load file:');
    console.log('  name: ' + file.name);
    console.log('  type: ' + file.type);
    console.log('  size: ' + file.size);
    if (file.size < 256000) {
        const reader = new FileReader();
        reader.onload = (loadEvent) => {
            console.log('file loaded!');
            const content = loadEvent.target.result;
            if (content) {
                console.log('content length: ' + content.byteLength);
                const uint8Array = new Uint8Array(content);
                const res = ccall('util_emsc_loadfile',  // C function name
                    'int',
                    ['string', 'array', 'number'],  // name, data, size
                    [file.name, uint8Array, uint8Array.length]);
                if (res == 0) {
                    console.warn('util_emsc_loadfile() failed!');
                }
            }
            else {
                console.warn('load result empty!');
            }
        };
        reader.readAsArrayBuffer(file);
    }
    else {
        console.warn('ignoring file because it is too big')
    }
});

EM_JS(void, emsc_js_open_link, (const char* c_url), {
    const url = UTF8ToString(c_url);
    window.open(url);
});

EMSCRIPTEN_KEEPALIVE int util_emsc_loadfile(const char* name, uint8_t* data, int size) {
    ui_asm_put_source(name, (range_t){ .ptr=data, .size=(size_t)size });
    ui_set_window_open(UI_WINDOW_ASM, true);
    ui_asm_assemble();
    return 1;
}

EM_JS(void, emsc_js_save_settings, (const char* c_key, const char* c_payload), {
    const key = UTF8ToString(c_key);
    const payload = UTF8ToString(c_payload);
    window.localStorage.setItem(key, payload);
});

EM_JS(const char*, emsc_js_load_settings, (const char* c_key), {
    if (window.localStorage === undefined) {
        return 0;
    }
    const key = UTF8ToString(c_key);
    const payload = window.localStorage.getItem(key);
    if (payload) {
        return stringToNewUTF8(payload);
    } else {
        return 0;
    }
});
#else // win32/posix
#define UTIL_PATH_SIZE (2048)

typedef struct {
    char cstr[UTIL_PATH_SIZE];
    bool clamped;
} util_path_t;

#if defined(__GNUC__)
static util_path_t util_path_printf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#endif
static util_path_t util_path_printf(const char* fmt, ...) {
    util_path_t path = {0};
    va_list args;
    va_start(args, fmt);
    int res = vsnprintf(path.cstr, sizeof(path.cstr), fmt, args);
    va_end(args);
    path.clamped = res >= (int)sizeof(path.cstr);
    return path;
}

#if defined (WIN32)
static bool util_win32_path_to_wide(const util_path_t* path, WCHAR* out_buf, size_t out_buf_size_in_bytes) {
    if ((path->cstr[0] == 0) || (path->clamped)) {
        return false;
    }
    int out_num_chars = (int)(out_buf_size_in_bytes / sizeof(wchar_t));
    if (0 == MultiByteToWideChar(CP_UTF8, 0, path->cstr, -1, out_buf, out_num_chars)) {
        return false;
    }
    return true;
}
#endif

static bool util_win32_posix_write_file(util_path_t path, range_t data) {
    if (path.clamped) {
        return false;
    }
    #if defined(WIN32)
        WCHAR wc_path[UTIL_PATH_SIZE];
        if (!util_win32_path_to_wide(&path, wc_path, sizeof(wc_path))) {
            return false;
        }
        HANDLE fp = CreateFileW(wc_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fp == INVALID_HANDLE_VALUE) {
            return false;
        }
        if (!WriteFile(fp, data.ptr, (DWORD)data.size, NULL, NULL)) {
            CloseHandle(fp);
            return false;
        }
        CloseHandle(fp);
    #else
        FILE* fp = fopen(path.cstr, "wb");
        if (!fp) {
            return false;
        }
        fwrite(data.ptr, data.size, 1, fp);
        fclose(fp);
    #endif
    return true;
}

// NOTE: free the returned range.ptr with free(ptr)
static range_t util_win32_posix_read_file(util_path_t path, bool null_terminated) {
    if (path.clamped) {
        return (range_t){0};
    }
    #if defined(WIN32)
        WCHAR wc_path[UTIL_PATH_SIZE];
        if (!util_win32_path_to_wide(&path, wc_path, sizeof(wc_path))) {
            return (range_t){0};
        }
        HANDLE fp = CreateFileW(wc_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fp == INVALID_HANDLE_VALUE) {
            return (range_t){0};
        }
        size_t file_size = GetFileSize(fp, NULL);
        size_t alloc_size = null_terminated ? file_size + 1 : file_size;
        void* ptr = calloc(1, alloc_size);
        DWORD read_bytes = 0;
        BOOL read_res = ReadFile(fp, ptr, (DWORD)file_size, &read_bytes, NULL);
        CloseHandle(fp);
        if (read_res && read_bytes == file_size) {
            return (range_t){ .ptr = ptr, .size = alloc_size };
        } else {
            free(ptr);
            return (range_t){0};
        }
    #else
        FILE* fp = fopen(path.cstr, "rb");
        if (!fp) {
            return (range_t){0};
        }
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        size_t alloc_size = null_terminated ? file_size + 1 : file_size;
        void* ptr = calloc(1, alloc_size);
        size_t bytes_read = fread(ptr, 1, file_size, fp);
        fclose(fp);
        if (bytes_read == file_size) {
            return (range_t){ .ptr = ptr, .size = alloc_size };
        } else {
            free(ptr);
            return (range_t){0};
        }
    #endif
}

static util_path_t util_win32_posix_tmp_dir(void) {
    #if defined(WIN32)
    WCHAR wc_tmp_path[UTIL_PATH_SIZE];
    if (0 == GetTempPathW(sizeof(wc_tmp_path) / sizeof(WCHAR), wc_tmp_path)) {
        return (util_path_t){0};
    }
    char utf8_tmp_path[UTIL_PATH_SIZE];
    if (0 == WideCharToMultiByte(CP_UTF8, 0, wc_tmp_path, -1, utf8_tmp_path, sizeof(utf8_tmp_path), NULL, NULL)) {
        return (util_path_t){0};
    }
    return util_path_printf("%s", utf8_tmp_path);
    #else
    return util_path_printf("%s", "/tmp");
    #endif
}

util_path_t util_win32_posix_make_snapshot_path(const char* system_name, size_t snapshot_index) {
    util_path_t tmp_dir = util_win32_posix_tmp_dir();
    return util_path_printf("%s/chips_%s_snapshot_%zu", tmp_dir.cstr, system_name, snapshot_index);
}

util_path_t util_win32_posix_make_ini_path(const char* key) {
    util_path_t tmp_dir = util_win32_posix_tmp_dir();
    return util_path_printf("%s/%s_imgui.ini", tmp_dir.cstr, key);
}
#endif

void util_init(void) {
    assert(!util.valid);
    #if defined(__EMSCRIPTEN__)
    emsc_js_init();
    util.is_osx = emsc_js_is_osx();
    #elif defined(__APPLE__)
    util.is_osx = true;
    #else
    util.is_osx = false;
    #endif
    util.valid = true;
}

void util_shutdown(void) {
    assert(util.valid);
    util.valid = false;
}

void util_html5_download_string(const char* filename, const char* content) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_download_string(filename, content);
    #else
    (void)filename; (void)content;
    #endif
}

void util_html5_download_binary(const char* filename, range_t bytes) {
    if (bytes.size == 0) {
        return;
    }
    assert(bytes.ptr);
    #if defined(__EMSCRIPTEN__)
    emsc_js_download_binary(filename, bytes.ptr, bytes.size);
    #else
    (void)filename; (void)bytes;
    #endif
}

void util_html5_load(void) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_load();
    #endif
}

void util_html5_open_link(const char* url) {
    #if defined(__EMSCRIPTEN__)
    emsc_js_open_link(url);
    #else
    (void)url;
    #endif
}

bool util_is_osx(void) {
    assert(util.valid);
    return util.is_osx;
}

void util_save_settings(const char* key, const char* payload) {
    assert(key && payload);
    #if defined(__EMSCRIPTEN__)
        emsc_js_save_settings(key, payload);
    #else
        util_path_t path = util_win32_posix_make_ini_path(key);
        range_t data = { .ptr = (void*)payload, .size = strlen(payload) };
        util_win32_posix_write_file(path, data);
    #endif
}

// NOTE: may return 0
const char* util_load_settings(const char* key) {
    assert(key);
    #if defined(__EMSCRIPTEN__)
        return emsc_js_load_settings(key);
    #else
        util_path_t path = util_win32_posix_make_ini_path(key);
        range_t data = util_win32_posix_read_file(path, true);
        return data.ptr;
    #endif
}

void util_free_settings(const char* payload) {
    if (payload) {
        free((void*)payload);
    }
}
