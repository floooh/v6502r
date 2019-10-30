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

static struct {
    uint64_t last_time;
    sg_pass_action pass_action;
} state;

void ui_setup() {
    // setup sokol-gfx
    sg_desc gfx_desc = { };
    gfx_desc.buffer_pool_size = 8;
    gfx_desc.image_pool_size = 8;
    gfx_desc.pipeline_pool_size = 8;
    gfx_desc.context_pool_size = 1;
    gfx_desc.mtl_device = sapp_metal_get_device();
    gfx_desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
    gfx_desc.mtl_drawable_cb = sapp_metal_get_drawable;
    gfx_desc.d3d11_device = sapp_d3d11_get_device();
    gfx_desc.d3d11_device_context = sapp_d3d11_get_device_context();
    gfx_desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
    gfx_desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;
    sg_setup(&gfx_desc);

    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_desc.max_vertices = 500000;
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
}

void ui_discard() {
    simgui_shutdown();
    sg_shutdown();
}

void ui_input(const sapp_event* ev) {
    simgui_handle_event(ev);
}

void ui_begin() {
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60.0);
}

void ui_end() {
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    simgui_render();
    sg_end_pass();
    sg_commit();
}
