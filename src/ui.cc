//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "imgui.h"
#define CHIPS_IMPL
#include "v6502r.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"

static void ui_menu(void);
static void ui_picking(void);
static uint8_t ui_mem_read(int layer, uint16_t addr, void* user_data);
static void ui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data);

void ui_init() {
    float disp_w = (float) sapp_width();

    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    {
        ui_memedit_desc_t desc = { };
        desc.title = "Memory Editor";
        desc.open = true;
        desc.num_rows = 8;
        desc.h = 300;
        desc.x = disp_w - 300.0f;
        desc.y = 50.0f;
        desc.hide_ascii = true;
        desc.read_cb = ui_mem_read;
        desc.write_cb = ui_mem_write;
        ui_memedit_init(&app.ui.memedit, &desc);
    }
    {
        ui_dasm_desc_t desc = { };
        desc.title = "Disassembler";
        desc.open = false;
        desc.cpu_type = UI_DASM_CPUTYPE_M6502;
        desc.start_addr = 0;
        desc.read_cb = ui_mem_read;
        desc.x = 50.0f;
        desc.y = 50.0f;
        ui_dasm_init(&app.ui.dasm, &desc);
    }
}

void ui_shutdown() {
    ui_dasm_discard(&app.ui.dasm);
    ui_memedit_discard(&app.ui.memedit);
    simgui_shutdown();
}

bool ui_input(const sapp_event* ev) {
    // layer visibility hotkeys?
    if ((ev->type == SAPP_EVENTTYPE_KEY_DOWN) && (0 != (ev->modifiers & SAPP_MODIFIER_ALT))) {
        int l = -1;
        switch (ev->key_code) {
            case SAPP_KEYCODE_1: l = 0; break;
            case SAPP_KEYCODE_2: l = 1; break;
            case SAPP_KEYCODE_3: l = 2; break;
            case SAPP_KEYCODE_4: l = 3; break;
            case SAPP_KEYCODE_5: l = 4; break;
            case SAPP_KEYCODE_6: l = 5; break;
            default: break;
        }
        if (l != -1) {
            app.chipvis.layer_visible[l] = !app.chipvis.layer_visible[l];
        }
    }
    return simgui_handle_event(ev) || ImGui::GetIO().WantCaptureMouse;
}

void ui_frame() {
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60.0);
    ui_menu();
    ui_picking();
    ui_memedit_draw(&app.ui.memedit);
    ui_dasm_draw(&app.ui.dasm);
}

void ui_draw() {
    simgui_render();
}

void ui_menu(void) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Layers")) {
            if (ImGui::BeginMenu("Visible")) {
                ImGui::MenuItem("Layer #1##V", "Alt+1", &app.chipvis.layer_visible[0]);
                ImGui::MenuItem("Layer #2##V", "Alt+2", &app.chipvis.layer_visible[1]);
                ImGui::MenuItem("Layer #3##V", "Alt+3", &app.chipvis.layer_visible[2]);
                ImGui::MenuItem("Layer #4##V", "Alt+4", &app.chipvis.layer_visible[3]);
                ImGui::MenuItem("Layer #5##V", "Alt+5", &app.chipvis.layer_visible[4]);
                ImGui::MenuItem("Layer #6##V", "Alt+6", &app.chipvis.layer_visible[5]);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Pickable")) {
                ImGui::MenuItem("Layer #1##P", 0, &app.picking.layer_enabled[0]);
                ImGui::MenuItem("Layer #2##P", 0, &app.picking.layer_enabled[1]);
                ImGui::MenuItem("Layer #3##P", 0, &app.picking.layer_enabled[2]);
                ImGui::MenuItem("Layer #4##P", 0, &app.picking.layer_enabled[3]);
                ImGui::MenuItem("Layer #5##P", 0, &app.picking.layer_enabled[4]);
                ImGui::MenuItem("Layer #6##P", 0, &app.picking.layer_enabled[5]);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows")) {
            ImGui::MenuItem("Memory Editor", 0, &app.ui.memedit.open);
            ImGui::MenuItem("Disassembler", 0, &app.ui.dasm.open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ui_picking(void) {
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
}

// read and write callback for memory editor windows
uint8_t ui_mem_read(int /*layer*/, uint16_t addr, void* /*user_data*/) {
    return sim_r8(addr);
}

void ui_mem_write(int /*layer*/, uint16_t addr, uint8_t data, void* /*user_data*/) {
    sim_w8(addr, data);
}

