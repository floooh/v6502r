#pragma once
#include "common.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    const char* filename;
    int line_nr;                // 1-based!
    bool warning;               // true: warning, false: error
    const char* msg;
} asm_error_t;

typedef struct {
    bool errors;
    bool warnings;
    uint16_t addr;
    uint16_t len;
    const uint8_t* bytes;
} asm_result_t;

void asm_init(void);
void asm_source_open(void);
void asm_source_write(const char* src, uint32_t tab_width);
void asm_source_close(void);
asm_result_t asm_assemble(void);
int asm_num_errors(void);
const asm_error_t* asm_error(int index);
const char* asm_stderr(void);
const char* asm_listing(void);
const char* asm_source(void);

#if defined(__cplusplus)
} // extern "C"
#endif
