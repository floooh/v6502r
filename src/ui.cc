//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "ui.h"
#include "imgui.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_time.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

void ui_init() {
    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
}

void ui_shutdown() {
    simgui_shutdown();
}

bool ui_input(const sapp_event* ev) {
    return simgui_handle_event(ev) || ImGui::GetIO().WantCaptureMouse;
}

void ui_new_frame() {
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60.0);
}

void ui_draw() {
    simgui_render();
}
