// compile the sokol_imgui impls as C++, because we use the ImGui C++ API
#include "imgui.h"
#include "sokol_defines.h"
#include "sokol_gfx.h"
#include "sokol_app.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"
