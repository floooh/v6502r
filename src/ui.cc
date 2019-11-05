//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "imgui.h"
#define CHIPS_IMPL
#include "v6502r.h"
#include "fonts/fonts.h"
#include "fonts/iconsfontawesome4_c.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"
#define SOKOL_GFX_IMGUI_IMPL
#include "sokol_gfx_imgui.h"

static void ui_menu(void);
static void ui_picking(void);
static void ui_controls(void);
static uint8_t ui_mem_read(int layer, uint16_t addr, void* user_data);
static void ui_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data);

void ui_init() {
    // default window open state
    app.ui.cpu_controls_open = true;

    // initialize the sokol-gfx debugging UI
    sg_imgui_init(&app.ui.sg_imgui);

    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_desc.no_default_font = true;
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;

    // setup ImGui font with custom icons
    auto& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(dump_fontawesome_ttf,
        sizeof(dump_fontawesome_ttf),
        16.0f, &icons_config, icons_ranges);

    unsigned char* font_pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
    {
        sg_image_desc desc = { };
        desc.width = font_width;
        desc.height = font_height;
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        desc.min_filter = SG_FILTER_LINEAR;
        desc.mag_filter = SG_FILTER_LINEAR;
        desc.content.subimage[0][0].ptr = font_pixels;
        desc.content.subimage[0][0].size = font_width * font_height * 4;
        io.Fonts->TexID = (ImTextureID)(uintptr_t) sg_make_image(&desc).id;
    }

    // initialize helper windows from the chips projects
    float disp_w = (float) sapp_width();
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
    sg_imgui_discard(&app.ui.sg_imgui);
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
    ui_controls();
    sg_imgui_draw(&app.ui.sg_imgui);
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
            ImGui::MenuItem("Controls", 0, &app.ui.cpu_controls_open);
            ImGui::MenuItem("Memory Editor", 0, &app.ui.memedit.open);
            ImGui::MenuItem("Disassembler", 0, &app.ui.dasm.open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Sokol")) {
            ImGui::MenuItem("Buffers", 0, &app.ui.sg_imgui.buffers.open);
            ImGui::MenuItem("Images", 0, &app.ui.sg_imgui.images.open);
            ImGui::MenuItem("Shaders", 0, &app.ui.sg_imgui.shaders.open);
            ImGui::MenuItem("Pipelines", 0, &app.ui.sg_imgui.pipelines.open);
            ImGui::MenuItem("Calls", 0, &app.ui.sg_imgui.capture.open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ui_controls(void) {
    if (!app.ui.cpu_controls_open) {
        return;
    }
    ImGui::SetNextWindowSize({ 300, 64 }, ImGuiCond_Once);
    if (ImGui::Begin("6502 Controls", &app.ui.cpu_controls_open, ImGuiWindowFlags_NoResize)) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 10.0f } );
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 7.0f, 7.0f } );
        ImGui::PushStyleColor(ImGuiCol_Button, 0xFFFFFFFF);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF000000);
        if (ImGui::Button(ICON_FA_PAUSE, { 28, 25 })) {

        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_STEP_FORWARD, { 28, 25 })) {

        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLAY, { 28, 25 })) {

        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FAST_FORWARD, { 28, 25 })) {

        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_EJECT, { 28, 25 })) {

        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }
    ImGui::End();
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

