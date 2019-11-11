//------------------------------------------------------------------------------
//  asm.c
//  Wrapper and glue code for asmx.
//------------------------------------------------------------------------------
#include "asmx.h"
#include <assert.h>

char* asmx_optarg;
int asmx_optind;
uint32_t* asmx_stderr;
uint32_t* asmx_stdout;

// C runtime wrapper functions
asmx_FILE* asmx_fopen(const char* filename, const char* mode) {
    assert(false);
    return 0;
}

int asmx_fclose(asmx_FILE* stream) {
    assert(false);
    return 0;
}

asmx_size_t asmx_fwrite(const void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* stream) {
    assert(false);
    return 0;
}

asmx_size_t asmx_fread(void* ptr, asmx_size_t size, asmx_size_t count, asmx_FILE* stream) {
    assert(false);
    return 0;
}

int asmx_fgetc(asmx_FILE* stream) {
    assert(false);
    return 0;
}

int asmx_ungetc(int character, asmx_FILE* stream) {
    assert(false);
    return 0;
}

long int asmx_ftell(asmx_FILE* stream) {
    assert(false);
    return 0;
}

int asmx_fseek(asmx_FILE* stream, long int offset, int origin) {
    assert(false);
    return 0;
}

int asmx_fputc(int character, asmx_FILE* stream) {
    assert(false);
    return 0;
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
    assert(false);
    return 0;
}

void asmx_free(void* ptr) {
    assert(false);
}

asmx_size_t asmx_strlen(const char* str) {
    assert(false);
    return 0;
}

char* asmx_strcpy(char* destination, const char* source) {
    assert(false);
    return 0;
}

char* asmx_strncpy(char* destination, const char* source, asmx_size_t num) {
    assert(false);
    return 0;
}

int asmx_strcmp(const char* str1, const char* str2) {
    assert(false);
    return 0;
}

char* asmx_strchr(const char* str, int c) {
    assert(false);
    return 0;
}

char* asmx_strcat(char * destination, const char * source) {
    assert(false);
    return 0;
}

int asmx_toupper(int c) {
    assert(false);
    return 0;
}

int asmx_isdigit(int c) {
    assert(false);
    return 0;
}

int asmx_sprintf(char* str, const char* format, ...) {
    assert(false);
    return 0;
}

void* asmx_memcpy(void* dest, const void* src, asmx_size_t count) {
    assert(false);
    return 0;
}

int asmx_abs(int n) {
    assert(false);
    return 0;
}

void asmx_exit(int status) {
    assert(false);
}

int asmx_getopt(int argc, char* const argv[], const char* optstring) {
    assert(false);
    return 0;
}
