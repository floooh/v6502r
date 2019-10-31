#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CHIP_LAYERS (6)

typedef struct {
    float x, y, z, w;
} float4_t;

typedef struct {
    float x, y;
} float2_t;

typedef struct {
    float aspect;
    float scale;
    float2_t scale_pivot;
    float2_t offset;
    float4_t layer_colors[MAX_CHIP_LAYERS];
    bool layer_visible[MAX_CHIP_LAYERS];
} chipvis_t;

void chipvis_init(void);
void chipvis_draw(const chipvis_t* params);
void chipvis_shutdown(void);

#ifdef __cplusplus
} // extern "C"
#endif
