//------------------------------------------------------------------------------
//  util.c
//------------------------------------------------------------------------------
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include "ui_asm.h"
#include "util.h"
#include <stdlib.h> // free()

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
    ui_asm_set_window_open(true);
    ui_asm_assemble();
    return 1;
}

EM_JS(void, emsc_js_save_string, (const char* c_key, const char* c_payload), {
    const key = UTF8ToString(c_key);
    const payload = UTF8ToString(c_payload);
    window.localStorage.setItem(key, payload);
});

EM_JS(const char*, emsc_js_load_string, (const char* c_key), {
    const key = UTF8ToString(c_key);
    const payload = window.localStorage.getItem(key);
    if (payload) {
        return stringToNewUTF8(payload);
    } else {
        return 0;
    }
});
#endif // EMSCRIPTEN

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

void util_save_string(const char* key, const char* payload) {
    assert(key && payload);
    #if defined(__EMSCRIPTEN__)
    emsc_js_save_string(key, payload);
    #endif
}

// NOTE: may return 0
const char* util_load_string(const char* key) {
    assert(key);
    #if defined(__EMSCRIPTEN__)
    return emsc_js_load_string(key);
    #else
    return 0;
    #endif
}

void util_free_loaded_string(const char* payload) {
    if (payload) {
        free((void*)payload);
    }
}
