#include "imgui.h"

extern "C" {

void ui_hello(void) {
    ImGui::SetNextWindowSize({ 120, 60 });
    ImGui::Begin("Hello World!");
    ImGui::End();
}

} // extern "C"

