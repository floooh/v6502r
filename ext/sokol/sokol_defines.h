#define SOKOL_TRACE_HOOKS
#if defined(_MSC_VER)
    #define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
    #define SOKOL_GLES3
#elif defined(__APPLE__)
    #define SOKOL_METAL
#else
    #define SOKOL_GLCORE
#endif
