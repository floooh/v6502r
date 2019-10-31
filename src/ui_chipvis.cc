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
    ImGui::SetNextWindowSize({150, 200}, ImGuiCond_Once);
    ImGui::Begin("6502 Controls");
    ImGui::Text("Mouse-drag to pan\nscroll to zoom.\n");
    ImGui::Checkbox("Layer 0", &params->layer_visible[0]);
    ImGui::Checkbox("Layer 1", &params->layer_visible[1]);
    ImGui::Checkbox("Layer 2", &params->layer_visible[2]);
    ImGui::Checkbox("Layer 3", &params->layer_visible[3]);
    ImGui::Checkbox("Layer 4", &params->layer_visible[4]);
    ImGui::Checkbox("Layer 5", &params->layer_visible[5]);
    ImGui::End();
}

} // extern "C"
