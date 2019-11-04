//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "imgui.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

static void ui_chipvis(void);

void ui_init() {
    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    app.ui.about_open = true;
}

void ui_shutdown() {
    simgui_shutdown();
}

bool ui_input(const sapp_event* ev) {
    return simgui_handle_event(ev) || ImGui::GetIO().WantCaptureMouse;
}

void ui_frame() {
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60.0);
    ui_chipvis();
}

void ui_draw() {
    simgui_render();
}

void ui_menu(void) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("About", 0, &app.ui.about_open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ui_chipvis(void) {
    ImGui::SetNextWindowPos({20, 20}, ImGuiCond_Once);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Mouse-drag to pan\nscroll to zoom.\n");
    if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Layer 0##V", &app.chipvis.layer_visible[0]);
        ImGui::Checkbox("Layer 1##V", &app.chipvis.layer_visible[1]);
        ImGui::Checkbox("Layer 2##V", &app.chipvis.layer_visible[2]);
        ImGui::Checkbox("Layer 3##V", &app.chipvis.layer_visible[3]);
        ImGui::Checkbox("Layer 4##V", &app.chipvis.layer_visible[4]);
        ImGui::Checkbox("Layer 5##V", &app.chipvis.layer_visible[5]);
    }
    if (ImGui::CollapsingHeader("Picking", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Layer 0##P", &app.picking.layer_enabled[0]);
        ImGui::Checkbox("Layer 1##P", &app.picking.layer_enabled[1]);
        ImGui::Checkbox("Layer 2##P", &app.picking.layer_enabled[2]);
        ImGui::Checkbox("Layer 3##P", &app.picking.layer_enabled[3]);
        ImGui::Checkbox("Layer 4##P", &app.picking.layer_enabled[4]);
        ImGui::Checkbox("Layer 5##P", &app.picking.layer_enabled[5]);
    }
    // if there are picking results, build a tooltip
    if (app.picking.result.num_hits > 0) {
        char str[256] = { 0 };
        for (int i = 0; i < app.picking.result.num_hits; i++) {
            int node_index = app.picking.result.node_index[i];
            assert((node_index >= 0) && (node_index < max_node_names));
            if (node_names[node_index][0] != 0) {
                if (i != 0) {
                    strcat(str, "\n");
                }
                strcat(str, node_names[node_index]);
            }
        }
        if ((str[0] != 0) && !ImGui::GetIO().WantCaptureMouse) {
            ImGui::SetTooltip("%s", str);
        }
    }
    ImGui::End();
}

