//------------------------------------------------------------------------------
//  util.c
//------------------------------------------------------------------------------
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif
#include "ui_asm.h"
#include "util.h"

static struct {
    bool valid;
    bool is_osx;
} util;

#if defined(__EMSCRIPTEN__)
EM_JS_DEPS(v6502r, "$ccall");

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

EM_JS(void, emsc_js_save_async, (const char* c_key, const void* bytes, uint32_t num_bytes, util_save_callback_t completed, void* userdata), {
    console.log('emsc_js_save_async called');
    const db_name = UTF8ToString(c_key);
    const db_store_name = 'store';
    let open_request;
    try {
        open_request = window.indexedDB.open(db_name);
    } catch (err) {
        console.warn('emsc_js_save_async: failed to open IndexedDB with: ', err);
        _util_emsc_save_callback(false, completed, userdata);
        return;
    }
    open_request.onupgradeneeded = () => {
        console.log('emsc_js_save_async: onupgradeneeded');
        const db = open_request.result;
        db.createObjectStore(db_store_name);
    };
    open_request.onsuccess = () => {
        console.log('emsc_js_save_async: onsuccess');
        const db = open_request.result;
        let transaction;
        try {
            transaction = db.transaction([db_store_name], 'readwrite');
        } catch (err) {
            console.warn('emsc_js_save_async: db.transaction failed with: ', err);
            _util_emsc_save_callback(false, completed, userdata);
            return;
        }
        const file = transaction.objectStore(db_store_name);
        const blob = HEAPU8.subarray(bytes, bytes + num_bytes);
        const put_request = file.put(blob, 'imgui.ini');
        put_request.onsuccess = () => {
            console.log('emsc_js_save_async: put success');
            _util_emsc_save_callback(true, completed, userdata);
        };
        put_request.onerror = () => {
            console.warn('emsc_js_save_async: put failure');
            _util_emsc_save_callback(false, completed, userdata);
        };
        transaction.onerror = () => {
            console.warn('emsc_js_save_async: transaction failure');
            _util_emsc_save_callback(false, completed, userdata);
        };
    };
    open_request.onerror = () => {
        console.warn('emsc_js_save_async: open request failure');
        _util_emsc_save_callback(false, completed, userdata);
    };
});

EMSCRIPTEN_KEEPALIVE void util_emsc_save_callback(bool succeeded, util_save_callback_t completed, void* userdata) {
    completed(succeeded, userdata);
}

EM_JS(void, emsc_js_load_async, (const char* c_key, util_load_callback_t completed, void* userdata), {
    console.log('emsc_js_load_async called');
    const db_name = UTF8ToString(c_key);
    const db_store_name = 'store';
    let open_request;
    try {
        open_request = window.indexedDB.open(db_name);
    } catch (err) {
        console.warn('emsc_js_load_async: failed to open IndexedDB with: ', err);
        _util_emsc_load_callback(false, completed, 0, 0, userdata);
        return;
    }
    open_request.onupgradeneeded = () => {
        console.log('emsc_js_load_async: onupgradeneeded');
        const db = open_request.result;
        db.createObjectStore(db_store_name);
    };
    open_request.onsuccess = () => {
        console.log('emsc_js_load_async: open_request onsuccess');
        let db = open_request.result;
        let transaction;
        try {
            transaction = db.transaction([db_store_name], 'readwrite');
        } catch (err) {
            console.warn('emsc_js_load_async: db.transaction failed with: ', err);
            _util_emsc_load_callback(false, completed, 0, 0, userdata);
            return;
        }
        const file = transaction.objectStore(db_store_name);
        const get_request = file.get('imgui.ini');
        get_request.onsuccess = () => {
            console.log('emsc_js_load_async: get_request onsuccess');
            if (get_request.result !== undefined) {
                const num_bytes = get_request.result.length;
                console.log(`emsc_js_load_async: successfully loaded ${num_bytes} bytes`);
                const ptr = _util_emsc_alloc(num_bytes);
                HEAPU8.set(get_request.result, ptr);
                _util_emsc_load_callback(true, completed, ptr, num_bytes, userdata);
            } else {
                console.warn('emsc_js_load_async: get_request.result is undefined');
                _util_emsc_load_callback(false, completed, 0, 0, userdata);
            }
        };
        get_request.onerror = () => {
            console.warn('emsc_js_load_async: get_request onerror');
            _util_emsc_load_callback(false, completed, 0, 0, userdata);
        };
        transaction.onerror = () => {
            console.warn('emsc_js_load_async: transaction onerror');
            _util_emsc_load_callback(false, completed, 0, 0, userdata);
        };
    };
    open_request.onerror = () => {
        console.log('emsc_js_load_async: open_request onerror');
        _util_emsc_load_callback(false, completed, 0, 0, userdata);
    }
});

EMSCRIPTEN_KEEPALIVE void* util_emsc_alloc(uint32_t num_bytes) {
    return malloc(num_bytes);
}

EMSCRIPTEN_KEEPALIVE void util_emsc_load_callback(bool succeeded, util_load_callback_t completed, void* bytes, uint32_t num_bytes, void* userdata) {
    completed(succeeded, bytes, num_bytes, userdata);
    free(bytes);
}
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

void util_save_async(util_save_t args) {
    assert(args.key);
    assert(args.bytes);
    assert(args.num_bytes > 0);
    assert(args.completed);
    #if defined(__EMSCRIPTEN__)
    emsc_js_save_async(args.key, args.bytes, args.num_bytes, args.completed, args.userdata);
    #else
    args.completed(false, args.userdata);
    #endif
}

void util_load_async(util_load_t args) {
    assert(args.key);
    assert(args.completed);
    #if defined(__EMSCRIPTEN__)
    emsc_js_load_async(args.key, args.completed, args.userdata);
    #else
    args.completed(false, 0, 0, args.userdata);
    #endif
}
