//------------------------------------------------------------------------------
//  ui_chip.cc
//  Draw the chip visualization.
//------------------------------------------------------------------------------
#include <stdint.h>
#include "segdefs.h"
#include "imgui.h"

extern "C" {

static uint32_t colors[8] = {
    0x88FF0000,
    0x8800FF00,
    0x880000FF,
    0x88FFFF00,
    0x8800FFFF,
    0x88FF00FF,
    0xFFFFFFFF,
    0xFFFFFFFF,
};

static float scale = 0.05f;
static bool layer_visible[8] = { true, true, true, true, true, true, true, true };

void ui_chip(void) {
    ImGui::SetNextWindowPos({20, 20}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({800, 600}, ImGuiCond_Once);
    ImGui::Begin("6502");
    ImGui::BeginChild("#chip", { 512, 512 }, true);
    ImDrawList* l = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    for (int si = 0; si < seg_segments_num; si++) {
        int layer_index = seg_segments[si][0];
        uint32_t color = colors[seg_segments[si][0]];
        if (layer_visible[layer_index]) {
            l->PathClear();
            ImVec2 v2;
            for (int i = 0; i < seg_segments[si][2]; i += 2) {
                int vi = seg_segments[si][1] + i;
                v2.x = (float)seg_vertices[vi] * scale + p0.x;
                v2.y = (float)seg_vertices[vi+1] * scale + p0.y;
                l->PathLineTo(v2);
            }
            l->PathStroke(color, true);
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("#controls");
    ImGui::Checkbox("Layer 0", &layer_visible[0]);
    ImGui::Checkbox("Layer 1", &layer_visible[1]);
    ImGui::Checkbox("Layer 2", &layer_visible[2]);
    ImGui::Checkbox("Layer 3", &layer_visible[3]);
    ImGui::Checkbox("Layer 4", &layer_visible[4]);
    ImGui::Checkbox("Layer 5", &layer_visible[5]);
    ImGui::SliderFloat("Scale", &scale, 0.01f, 1.0f, "%.3f", 3.0f);
    ImGui::EndChild();
    ImGui::End();
}

} // extern "C"
