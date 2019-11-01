//------------------------------------------------------------------------------
//  ui_chip.cc
//  Draw the chip visualization.
//------------------------------------------------------------------------------
#include "v6502r.h"
#include "imgui.h"

extern "C" {

void ui_chipvis(void) {
    ImGui::SetNextWindowPos({20, 20}, ImGuiCond_Once);
    //ImGui::SetNextWindowSize({150, 200}, ImGuiCond_Once);
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
    ImGui::End();
}

} // extern "C"
