//------------------------------------------------------------------------------
//  ui_chip.cc
//  Draw the chip visualization.
//------------------------------------------------------------------------------
#include <stdint.h>
#include "ui.h"
#include "imgui.h"

extern "C" {

void ui_chipvis(chipvis_t* params) {
    ImGui::SetNextWindowPos({20, 20}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({150, 250}, ImGuiCond_Once);
    ImGui::Begin("6502 Controls");
    ImGui::Checkbox("Layer 0", &params->layer_visible[0]);
    ImGui::Checkbox("Layer 1", &params->layer_visible[1]);
    ImGui::Checkbox("Layer 2", &params->layer_visible[2]);
    ImGui::Checkbox("Layer 3", &params->layer_visible[3]);
    ImGui::Checkbox("Layer 4", &params->layer_visible[4]);
    ImGui::Checkbox("Layer 5", &params->layer_visible[5]);
    ImGui::SliderFloat("Scale", &params->scale, 1.0f, 100.0f, "%.3f", 2.0f);
    ImGui::SliderFloat("Offset X", &params->offset.x, -2.0f, +1.0f);
    ImGui::SliderFloat("Offset Y", &params->offset.y, -2.0f, +1.0f);
    ImGui::End();
}

} // extern "C"
