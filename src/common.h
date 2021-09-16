#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // size_t
#include <assert.h>
#include <string.h>

#define MAX_LAYERS (6)
#define MAX_NODES (4096)    // NOTE: ALSO CHANGE IN SHADER!
#define MAX_TRACE_ITEMS (256)
#define MAX_BINARY_SIZE ((1<<16)+2)
#define MAX_LINKURL_SIZE (128)
#define PICK_MAX_HITS (16)
#define TRACE_FLIPBIT_CLK0 (1<<0)
#define TRACE_FLIPBIT_OP (1<<1)

#if defined(CHIP_6502)
#define WINDOW_TITLE "Visual 6502 Remix"
#else
#define WINDOW_TITLE "Visual Z80 Remix"
#endif

typedef struct {
    float x, y, z, w;
} float4_t;

typedef struct {
    float x, y;
} float2_t;

typedef struct {
    void* ptr;
    size_t size;
} range_t;
