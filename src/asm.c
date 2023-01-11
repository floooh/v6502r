//------------------------------------------------------------------------------
//  asm.c
//  Wrapper and glue code for asmx.
//------------------------------------------------------------------------------
#include "asmx.h"
#include "asm.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_MALLOC_SIZE (512 * 1024)
#define MAX_IOBUF_SIZE  (512 * 1024)
#define MAX_ERRORS (32)

#define IOSTREAM_STDERR     (0)
#define IOSTREAM_SRC        (1)
#define IOSTREAM_OBJ        (2)
#define IOSTREAM_LST        (3)
#define NUM_IOSTREAMS       (4)
#define IOSTREAM_INVALID    (0)

asmx_FILE* asmx_stderr = (asmx_FILE*) (IOSTREAM_STDERR+1);

typedef struct {
    uint32_t pos;
    uint32_t size;
    uint8_t buf[MAX_IOBUF_SIZE];
} iostream_t;

static struct {
    uint32_t src_line_pos;          // for untabbify during asm_source_write()
    iostream_t io[NUM_IOSTREAMS];
    uint32_t alloc_pos;
    uint8_t alloc_buf[MAX_MALLOC_SIZE];
    uint8_t parse_buf[MAX_IOBUF_SIZE];
    int num_errors;
    asm_error_t errors[MAX_ERRORS];
} state;

// return asmx_FILE* for IOSTREAM constant
asmx_FILE* asmx_file_ptr(uintptr_t io_stream) {
    if (io_stream < NUM_IOSTREAMS) {
        return (asmx_FILE*)(io_stream+1);
    }
    else {
        return (asmx_FILE*)IOSTREAM_INVALID;
    }
}

// return IOSTREAM constant from asmx_FILE*
iostream_t* asmx_io_stream(asmx_FILE* fp) {
    uintptr_t io_index = ((uintptr_t)fp)-1;
    assert(io_index < NUM_IOSTREAMS);
    return &state.io[io_index];
}

// C runtime wrapper functions
asmx_FILE* asmx_fopen(const char* filename, const char* mode) {
    assert(filename && mode);
    asmx_FILE* fp = 0;
    if (0 == strcmp(filename, "src.asm")) {
        fp = asmx_file_ptr(IOSTREAM_SRC);
    }
    else if (0 == strcmp(filename, "out.obj")) {
        fp = asmx_file_ptr(IOSTREAM_OBJ);
    }
    else if (0 == strcmp(filename, "list.lst")) {
        fp = asmx_file_ptr(IOSTREAM_LST);
    }
    if (fp != 0) {
        iostream_t* io = asmx_io_stream(fp);
        if (0 == strchr(mode, 'r')) {
            io->size = 0;
        }
        io->pos = 0;
    }
    return fp;
}

int asmx_fclose(asmx_FILE* stream) {
    (void)stream;
    return 0;
}

asmx_size_t asmx_fwrite(const void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* fp) {
    iostream_t* io = asmx_io_stream(fp);
    assert(io->pos <= io->size);
    uint32_t num_bytes = size * count;
    if ((io->pos + num_bytes) > MAX_IOBUF_SIZE) {
        return 0;
    }
    memcpy(&io->buf[io->pos], ptr, num_bytes);
    io->pos = io->pos + num_bytes;
    if (io->pos > io->size) {
        io->size = io->pos;
    }
    return num_bytes;
}

asmx_size_t asmx_fread(void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* fp) {
    iostream_t* io = asmx_io_stream(fp);
    assert(io->pos <= io->size);
    uint32_t num_bytes = size * count;
    if ((io->pos + num_bytes) >= io->size) {
        num_bytes = io->size - io->pos;
    }
    if (num_bytes > 0) {
        memcpy(ptr, &io->buf[io->pos], num_bytes);
        io->pos += num_bytes;
    }
    return num_bytes;
}

int asmx_fgetc(asmx_FILE* fp) {
    iostream_t* io = asmx_io_stream(fp);
    if (io->pos < io->size) {
        return io->buf[io->pos++];
    }
    else {
        return ASMX_EOF;
    }
}

int asmx_fputc(int character, asmx_FILE* fp) {
    iostream_t* io = asmx_io_stream(fp);
    assert(io->pos <= io->size);
    if (io->pos < MAX_IOBUF_SIZE) {
        io->buf[io->pos++] = character;
        if (io->pos > io->size) {
            io->size = io->pos;
        }
        return character;
    }
    else {
        return ASMX_EOF;
    }
}

int asmx_ungetc(int character, asmx_FILE* fp) {
    iostream_t* io = asmx_io_stream(fp);
    if (io->pos > 0) {
        io->buf[--io->pos] = (uint8_t) character;
        return character;
    }
    else {
        return ASMX_EOF;
    }
}

long int asmx_ftell(asmx_FILE* fp) {
    iostream_t* io = asmx_io_stream(fp);
    return io->pos;
}

int asmx_fseek(asmx_FILE* fp, long int offset, int origin) {
    iostream_t* io = asmx_io_stream(fp);
    switch (origin) {
        case ASMX_SEEK_SET:
            if ((uint32_t)offset <= io->size) {
                io->pos = offset;
                return 0;
            }
            break;
        case ASMX_SEEK_CUR:
            if (((io->pos + offset) >= 0) && ((io->pos + offset) <= io->size)) {
                io->pos += offset;
                return 0;
            }
            break;
        case ASMX_SEEK_END:
            if (((io->size - offset) >= 0) && ((io->size - offset) <= io->size)) {
                io->pos = io->size - offset;
                return 0;
            }
            break;
    }
    // error
    return -1;
}

int asmx_vfprintf(asmx_FILE* fp, const char* format, va_list vl) {
    iostream_t* io = asmx_io_stream(fp);
    int max_bytes = MAX_IOBUF_SIZE - io->pos;
    assert(max_bytes >= 0);
    int num_bytes = vsnprintf((char*)&io->buf[io->pos], max_bytes, format, vl);
    int written_bytes = (num_bytes > max_bytes) ? max_bytes : num_bytes;
    io->pos = io->size = io->pos + written_bytes;
    return num_bytes;
}

int asmx_fprintf(asmx_FILE* fp, const char* format, ...) {
    va_list vl;
    va_start(vl,format);
    int res = asmx_vfprintf(fp, format, vl);
    va_end(vl);
    return res;
}

int asmx_sprintf(char* str, const char* format, ...) {
    va_list vl;
    va_start(vl,format);
    int res = vsprintf(str, format, vl);
    va_end(vl);
    return res;
}

void* asmx_malloc(asmx_size_t size) {
    if ((state.alloc_pos + size) <= MAX_MALLOC_SIZE) {
        void* res = (void*) &state.alloc_buf[state.alloc_pos];
        state.alloc_pos = ((state.alloc_pos + size) + 7) & ~7;
        return res;
    }
    else {
        assert(false);
        return 0;
    }
}

void asmx_free(void* ptr) {
    (void)ptr;
    // nothing to do here
}

asmx_size_t asmx_strlen(const char* str) {
    return (asmx_size_t) strlen(str);
}

char* asmx_strcpy(char* destination, const char* source) {
    return strcpy(destination, source);
}

char* asmx_strncpy(char* destination, const char* source, asmx_size_t num) {
    return strncpy(destination, source, num);
}

int asmx_strcmp(const char* str1, const char* str2) {
    return strcmp(str1, str2);
}

char* asmx_strchr(const char* str, int c) {
    return strchr(str, c);
}

char* asmx_strcat(char * destination, const char * source) {
    return strcat(destination, source);
}

int asmx_toupper(int c) {
    return toupper(c);
}

int asmx_isdigit(int c) {
    return isdigit(c);
}

void* asmx_memcpy(void* dest, const void* src, asmx_size_t count) {
    return memcpy(dest, src, count);
}

int asmx_abs(int n) {
    return abs(n);
}

void asm_init(void) {
    state.src_line_pos = 0;
    for (int i = 0; i < NUM_IOSTREAMS; i++) {
        state.io[i].pos = 0;
        state.io[i].size = 0;
        state.io[i].buf[0] = 0;
    }
    state.alloc_pos = 0;
    state.num_errors = 0;
}

void asm_source_open(void) {
    asmx_fopen("src.asm", "w");
    state.src_line_pos = 0;
}

void asm_source_write(const char* str, uint32_t tab_width) {
    assert(str);
    assert(tab_width >= 1);
    int c;
    while ((c = *str++)) {
        if (c == '\t') {
            while ((++state.src_line_pos % (tab_width+1))!= 0) {
                asmx_fputc(' ', asmx_file_ptr(IOSTREAM_SRC));
            }
        }
        else {
            asmx_fputc(c, asmx_file_ptr(IOSTREAM_SRC));
            state.src_line_pos++;
        }
        if (c == '\n') {
            state.src_line_pos = 0;
        }
    }
}

void asm_source_close(void) {
    asmx_fclose(asmx_file_ptr(IOSTREAM_SRC));
}

static const char* asm_get_string(uintptr_t io_stream) {
    iostream_t* io = &state.io[io_stream];
    if (io->pos < MAX_IOBUF_SIZE) {
        io->buf[io->pos] = 0;
    }
    else {
        io->buf[MAX_IOBUF_SIZE-1] = 0;
    }
    return (const char*) io->buf;
}

const char* asm_source(void) {
    return asm_get_string(IOSTREAM_SRC);
}

const char* asm_stderr(void) {
    return asm_get_string(IOSTREAM_STDERR);
}

const char* asm_listing(void) {
    return asm_get_string(IOSTREAM_LST);
}

static char* asm_strtok_r(char* str, const char* delims, char** context) {
    #if defined(_MSC_VER)
        return strtok_s(str, delims, context);
    #else
        return strtok_r(str, delims, context);
    #endif
}

#define MAX_ERR_LINES (64)
static void asm_parse_errors(void) {
    assert(state.num_errors == 0);

    // split into lines and extract error lines
    strcpy((char*)state.parse_buf, asm_stderr());
    char* line_context;
    for (char* line = asm_strtok_r((char*)state.parse_buf, "\n", &line_context);
         line;
         line = asm_strtok_r(0, "\n", &line_context))
    {
        if (strstr(line, "*** Error:") || strstr(line, "*** Warning:")) {
            char* tok_context, *tok;
            int i;
            if (state.num_errors >= MAX_ERRORS) {
                break;
            }
            asm_error_t* err = &state.errors[state.num_errors++];
            for (i = 0, tok = asm_strtok_r(line, ":", &tok_context);
                 tok;
                 tok = asm_strtok_r(0, ":", &tok_context), i++)
            {
                switch (i) {
                    case 0:
                        err->filename = tok;
                        break;
                    case 1:
                        err->line_nr = atoi(tok);
                        break;
                    case 2:
                        err->warning = (0 == strcmp(" *** Warning", tok));
                        break;
                    case 3:
                        // remove trailing " ***"
                        tok[strlen(tok)-4] = 0;
                        // skip leading "  "
                        err->msg = &tok[2];
                        break;
                }
            }
            assert(err->filename && err->msg);
        }
    }
}

int asm_num_errors(void) {
    return state.num_errors;
}

const asm_error_t* asm_error(int index) {
    assert((index >= 0) && (index < state.num_errors));
    return &state.errors[index];
}

static bool asm_has_errors(void) {
    for (int i = 0; i < state.num_errors; i++) {
        if (!state.errors[i].warning) {
            return true;
        }
    }
    return false;
}

static bool asm_has_warnings(void) {
    for (int i = 0; i < state.num_errors; i++) {
        if (state.errors[i].warning) {
            return true;
        }
    }
    return false;
}

asm_result_t asm_assemble(void) {
    assert(0 == state.alloc_pos);
    #if defined(CHIP_6502) || defined(CHIP_2A03)
    const char* cpu_type = "6502";
    #else
    const char* cpu_type = "Z80";
    #endif
    asmx_Assemble(&(asmx_Options){
        .srcName = "src.asm",
        .objName = "out.obj",
        .lstName = "list.lst",
        .cpuType = cpu_type,
        .showErrors = true,
        .showWarnings = true
    });
    asm_parse_errors();
    return (asm_result_t) {
        .errors = asm_has_errors(),
        .warnings = asm_has_warnings(),
        .addr = (uint16_t) asmx_Shared.binBase,
        .len = (uint16_t) (asmx_Shared.binEnd - asmx_Shared.binBase),
        .bytes = state.io[IOSTREAM_OBJ].buf,
    };
}

