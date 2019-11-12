//------------------------------------------------------------------------------
//  asm.c
//  Wrapper and glue code for asmx.
//------------------------------------------------------------------------------
#include "asmx.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_MALLOC_SIZE (1024*1024)
#define MAX_IOBUF_SIZE  (1024 * 1024)

#define IOSTREAM_STDOUT     (0)
#define IOSTREAM_STDERR     (1)
#define IOSTREAM_SRC        (2)
#define IOSTREAM_OBJ        (3)
#define IOSTREAM_LST        (4)
#define NUM_IOSTREAMS       (5)
#define IOSTREAM_INVALID    (0xFFFFFFFF)

asmx_FILE* asmx_stderr = (asmx_FILE*) IOSTREAM_STDERR;

typedef struct {
    uint32_t pos;
    uint32_t size;
    uint8_t buf[MAX_IOBUF_SIZE];
} iostream_t;

static struct {
    iostream_t io[NUM_IOSTREAMS];
    uint32_t alloc_pos;
    uint8_t malloc_buf[MAX_MALLOC_SIZE];
} state;

// C runtime wrapper functions
asmx_FILE* asmx_fopen(const char* filename, const char* mode) {
    assert(filename && mode);
    uintptr_t io_stream = IOSTREAM_INVALID;
    if (0 == strcmp(filename, "src.asm")) {
        io_stream = IOSTREAM_SRC;
    }
    else if (0 == strcmp(filename, "out.obj")) {
        io_stream = IOSTREAM_OBJ;
    }
    else if (0 == strcmp(filename, "list.lst")) {
        io_stream = IOSTREAM_LST;
    }
    if (IOSTREAM_INVALID != io_stream) {
        if (0 == strchr(mode, 'r')) {
            state.io[io_stream].size = 0;
        }
        state.io[io_stream].pos = 0;
    }
    return (asmx_FILE*)io_stream;
}

int asmx_fclose(asmx_FILE* stream) {
    return 0;
}

asmx_size_t asmx_fwrite(const void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* stream) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
    assert(io->pos == io->size);
    uint32_t num_bytes = size * count;
    if ((io->pos + num_bytes) > MAX_IOBUF_SIZE) {
        return 0;
    }
    memcpy(&io->buf[io->pos], ptr, num_bytes);
    io->pos = io->size = io->pos + num_bytes;
    return num_bytes;
}

asmx_size_t asmx_fread(void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* stream) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
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

int asmx_fgetc(asmx_FILE* stream) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
    if (io->pos < io->size) {
        return io->buf[io->pos++];
    }
    else {
        return ASMX_EOF;
    }
}

int asmx_fputc(int character, asmx_FILE* stream) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
    if (io->pos < io->size) {
        io->buf[io->pos++] = character;
        return character;
    }
    else {
        return ASMX_EOF;
    }
}


int asmx_ungetc(int character, asmx_FILE* stream) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
    if (io->pos > 0) {
        io->buf[--io->pos] = (uint8_t) character;
        return character;
    }
    else {
        return ASMX_EOF;
    }
}

long int asmx_ftell(asmx_FILE* stream) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
    return io->pos;
}

int asmx_fseek(asmx_FILE* stream, long int offset, int origin) {
    uintptr_t io_stream = (uintptr_t)stream;
    assert(io_stream < NUM_IOSTREAMS);
    iostream_t* io = &state.io[io_stream];
    switch (origin) {
        case ASMX_SEEK_SET:
            if (offset < io->size) {
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

int asmx_fprintf(asmx_FILE* stream, const char* format, ...) {
    assert(false);
    return 0;
}

int asmx_printf(const char* format, ...) {
    assert(false);
    return 0;
}

void* asmx_malloc(asmx_size_t size) {
    if ((state.alloc_pos + size) <= MAX_MALLOC_SIZE) {
        void* res = (void*) &state.malloc_buf[state.alloc_pos];
        state.alloc_pos = ((state.alloc_pos + size) + 7) & ~7;
        return res;
    }
    else {
        assert(false);
        return 0;
    }
}

void asmx_free(void* ptr) {
    // nothing to do here
}

asmx_size_t asmx_strlen(const char* str) {
    return strlen(str);
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

int asmx_sprintf(char* str, const char* format, ...) {
    assert(false);
    return 0;
}

void* asmx_memcpy(void* dest, const void* src, asmx_size_t count) {
    return memcpy(dest, src, count);
}

int asmx_abs(int n) {
    return abs(n);
}

void asm_init(void) {
    memset(&state, 0, sizeof(state));
}

void asm_source_open(void) {
    asmx_FILE* fp = asmx_fopen("src.asm", "w");
    assert(IOSTREAM_SRC == (uintptr_t)fp);
}

void asm_source_write(const char* str) {
    assert(str);
    asmx_fwrite(str, strlen(str), 1, (asmx_FILE*)IOSTREAM_SRC);
}

void asm_source_close(void) {
    asmx_fclose((asmx_FILE*)IOSTREAM_SRC);
}

void asm_assemble(void) {
    asmx_Assemble(&(asmx_Options){
        .srcName = "src.asm",
        .objName = "out.obj",
        .lstName = "list.lst",
        .cpuType = "6502",
        .showErrors = true,
        .showWarnings = true
    });
}

void asm_test(void) {
    asm_init();
    asm_source_open();
    asm_source_write(
        "        .org $200\n"\
        "loop    lda #0\n"\
        "        jsr sub\n"\
        "        jmp loop\n"\
        "sub     inx\n"\
        "        dey\n"\
        "        inc $f\n"\
        "        sec\n"\
        "        adc #2\n"\
        "        rts\n"\
        "        .end\n");
    asm_source_close();
    asm_assemble();
}
