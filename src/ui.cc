//------------------------------------------------------------------------------
//  ui.cc
//------------------------------------------------------------------------------
#include "imgui.h"
#define CHIPS_IMPL
#include "v6502r.h"
#include "res/fonts.h"
#include "res/iconsfontawesome4_c.h"
#include "res/markdown.h"
#include "imgui_markdown/imgui_markdown.h"
#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"
#define SOKOL_GFX_IMGUI_IMPL
#include "sokol_gfx_imgui.h"
#include <math.h>

static void ui_menu(void);
static void ui_picking(void);
static void ui_tracelog(void);
static void ui_controls(void);
static void ui_listing(void);
static void ui_help_assembler(void);
static void ui_help_opcodes(void);
static void ui_help_about(void);
static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data);

static ImGui::MarkdownConfig md_conf;

// read and write callback for memory editor windows
static uint8_t ui_mem_read(int /*layer*/, uint16_t addr, void* /*user_data*/) {
    return sim_r8(addr);
}

static void ui_mem_write(int /*layer*/, uint16_t addr, uint8_t data, void* /*user_data*/) {
    sim_w8(addr, data);
}

void ui_init() {
    // default window open state
    app.ui.cpu_controls_open = true;
    app.ui.tracelog_open = true;

    // initialize the sokol-gfx debugging UI
    sg_imgui_init(&app.ui.sg_imgui);

    // setup sokol-imgui
    simgui_desc_t simgui_desc = { };
    simgui_desc.no_default_font = true;
    simgui_desc.disable_hotkeys = true;
    simgui_setup(&simgui_desc);
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.Alpha = 1.0f;
    style.TabRounding = 0.0f;

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
    ImFontConfig h1Conf;
    h1Conf.SizePixels = 26.0f;
    md_conf.headingFormats[0].font = io.Fonts->AddFontDefault(&h1Conf);

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
    {
        ui_memedit_desc_t desc = { };
        desc.title = "Memory Editor";
        desc.open = false;
        desc.num_rows = 8;
        desc.h = 300;
        desc.x = 50;
        desc.y = 50;
        desc.hide_ascii = true;
        desc.read_cb = ui_mem_read;
        desc.write_cb = ui_mem_write;
        ui_memedit_init(&app.ui.memedit, &desc);
        desc.title = "Integrated Memory Editor";
        desc.hide_options = true;
        desc.hide_addr_input = true;
        ui_memedit_init(&app.ui.memedit_integrated, &desc);
    }
    {
        ui_dasm_desc_t desc = { };
        desc.title = "Disassembler";
        desc.open = false;
        desc.cpu_type = UI_DASM_CPUTYPE_M6502;
        desc.start_addr = 0;
        desc.read_cb = ui_mem_read;
        desc.x = 50;
        desc.y = 50;
        ui_dasm_init(&app.ui.dasm, &desc);
    }
    ui_asm_init();

    // setup ImGui::Markdown configuration
    md_conf.linkCallback = markdown_link_callback;
    md_conf.headingFormats[0].separator = true;
    md_conf.headingFormats[0].newline_above = false;
    md_conf.headingFormats[0].newline_below = false;
    md_conf.headingFormats[1].newline_below = false;
    md_conf.linkIcon = ICON_FA_LINK;
}

void ui_shutdown() {
    ui_asm_discard();
    ui_dasm_discard(&app.ui.dasm);
    ui_memedit_discard(&app.ui.memedit);
    ui_memedit_discard(&app.ui.memedit_integrated);
    sg_imgui_discard(&app.ui.sg_imgui);
    simgui_shutdown();
}

static bool test_alt(const sapp_event* ev, sapp_keycode key_code) {
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        if ((ev->modifiers == SAPP_MODIFIER_ALT) && (ev->key_code == key_code)) {
            return true;
        }
    }
    return false;
}

static bool test_ctrl(const sapp_event* ev, sapp_keycode key_code) {
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        uint32_t mod = util_is_osx() ? SAPP_MODIFIER_SUPER : SAPP_MODIFIER_CTRL;
        if ((ev->modifiers == mod) && (ev->key_code == key_code)) {
            return true;
        }
    }
    return false;
}

static bool test_ctrl_shift(const sapp_event* ev, sapp_keycode key_code) {
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
        uint32_t mod = SAPP_MODIFIER_SHIFT | (util_is_osx() ? SAPP_MODIFIER_SUPER : SAPP_MODIFIER_CTRL);
        if ((ev->modifiers == mod) && (ev->key_code == key_code)) {
            return true;
        }
    }
    return false;
}

static bool test_click(const sapp_event* ev, bool hovered) {
    if (hovered && (ev->type == SAPP_EVENTTYPE_MOUSE_UP) && (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT)) {
        return true;
    }
    else {
        return false;
    }
}

static bool handle_special_link(void) {
    if (0 == strcmp(app.ui.link_url, "ui://listing")) {
        app.ui.listing_open = true;
        ImGui::SetWindowFocus("Listing");
        return true;
    }
    else if (0 == strcmp(app.ui.link_url, "ui://assembler")) {
        app.ui.asm_open = true;
        ImGui::SetWindowFocus("Assembler");
        return true;
    }
    return false;
}

bool ui_input(const sapp_event* ev) {
    int l = -1;
    if (test_click(ev, app.ui.link_hovered)) {
        app.ui.link_hovered = false;
        if (!handle_special_link()) {
            util_html5_open_link(app.ui.link_url);
        }
    }
    if (test_alt(ev, SAPP_KEYCODE_1)) {
        l = 0;
    }
    else if (test_alt(ev, SAPP_KEYCODE_2)) {
        l = 1;
    }
    else if (test_alt(ev, SAPP_KEYCODE_3)) {
        l = 2;
    }
    else if (test_alt(ev, SAPP_KEYCODE_4)) {
        l = 3;
    }
    else if (test_alt(ev, SAPP_KEYCODE_5)) {
        l = 4;
    }
    else if (test_alt(ev, SAPP_KEYCODE_6)) {
        l = 5;
    }
    if (l != -1) {
        app.chipvis.layer_visible[l] = !app.chipvis.layer_visible[l];
        return true;
    }
    if (test_alt(ev, SAPP_KEYCODE_C)) {
        app.ui.cpu_controls_open = !app.ui.cpu_controls_open;
    }
    if (test_alt(ev, SAPP_KEYCODE_T)) {
        app.ui.tracelog_open = !app.ui.tracelog_open;
    }
    if (test_alt(ev, SAPP_KEYCODE_A)) {
        app.ui.asm_open = !app.ui.asm_open;
    }
    if (test_alt(ev, SAPP_KEYCODE_L)) {
        app.ui.listing_open = !app.ui.listing_open;
    }
    if (test_alt(ev, SAPP_KEYCODE_M)) {
        app.ui.memedit.open = !app.ui.memedit.open;
    }
    if (test_alt(ev, SAPP_KEYCODE_D)) {
        app.ui.dasm.open = !app.ui.dasm.open;
    }
    if (test_click(ev, app.ui.open_source_hovered) || test_ctrl_shift(ev, SAPP_KEYCODE_O)) {
        util_html5_load();
        app.ui.open_source_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, app.ui.save_source_hovered) || test_ctrl_shift(ev, SAPP_KEYCODE_S)) {
        util_html5_download_string("v6502r.asm", ui_asm_source());
        app.ui.save_source_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, app.ui.save_listing_hovered)) {
        util_html5_download_string("v6502r.lst", asm_listing());
        app.ui.save_listing_hovered = false;
    }
    if (test_click(ev, app.ui.save_binary_hovered)) {
        if (app.binary.num_bytes > 0) {
            util_html5_download_binary("v6502r.bin", app.binary.buf, app.binary.num_bytes);
        }
        app.ui.save_binary_hovered = false;
    }
    if (test_ctrl(ev, SAPP_KEYCODE_Z) || test_ctrl(ev, SAPP_KEYCODE_Y)) {
        ui_asm_undo();
        sapp_consume_event();
    }
    if (test_ctrl_shift(ev, SAPP_KEYCODE_Z) || test_ctrl_shift(ev, SAPP_KEYCODE_Y)) {
        ui_asm_redo();
        sapp_consume_event();
    }
    if (test_click(ev, app.ui.cut_hovered) || test_ctrl(ev, SAPP_KEYCODE_X)) {
        ui_asm_cut();
        app.ui.cut_hovered = false;
        sapp_consume_event();
    }
    if (test_click(ev, app.ui.copy_hovered) || test_ctrl(ev, SAPP_KEYCODE_C)) {
        ui_asm_copy();
        app.ui.copy_hovered = false;
        sapp_consume_event();
    }
    if (ev->type == SAPP_EVENTTYPE_CLIPBOARD_PASTED) {
        ui_asm_paste();
    }
    if (0 != (ev->modifiers & (SAPP_MODIFIER_CTRL|SAPP_MODIFIER_ALT|SAPP_MODIFIER_SUPER))) {
        return true;
    }
    else {
        return simgui_handle_event(ev);
    }
}

void ui_frame() {
    app.ui.link_hovered = false;
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60.0);
    ui_menu();
    ui_picking();
    ui_memedit_draw(&app.ui.memedit);
    ui_dasm_draw(&app.ui.dasm);
    ui_tracelog();
    ui_controls();
    ui_asm_draw();
    ui_listing();
    ui_help_assembler();
    ui_help_opcodes();
    ui_help_about();
    sg_imgui_draw(&app.ui.sg_imgui);
}

void ui_draw() {
    simgui_render();
}

void ui_menu(void) {
    app.ui.open_source_hovered = false;
    app.ui.save_source_hovered = false;
    app.ui.save_binary_hovered = false;
    app.ui.save_listing_hovered = false;
    if (ImGui::BeginMainMenuBar()) {
        bool is_osx = util_is_osx();
        if (ImGui::BeginMenu("File")) {
            // this looks all a bit weired because on the web platforms
            // these actions must be invoked from within an input handler
            ImGui::MenuItem("Open Source...", is_osx?"Cmd+Shift+O":"Ctrl+Shift+O");
            app.ui.open_source_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save Source...", is_osx?"Cmd+Shift+S":"Ctrl+Shift+S");
            app.ui.save_source_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save .BIN/.PRG...", 0);
            app.ui.save_binary_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Save Listing...", 0);
            app.ui.save_listing_hovered = ImGui::IsItemHovered();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", is_osx?"Cmd+Z|Y":"Ctrl+Z|Y")) {
                ui_asm_undo();
            }
            if (ImGui::MenuItem("Redo", is_osx?"Cmd+Shift+Z|Y":"Ctrl+Shift+Z|Y")) {
                ui_asm_redo();
            }
            ImGui::Separator();
            ImGui::MenuItem("Cut", is_osx?"Cmd+X":"Ctrl+X");
            app.ui.cut_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Copy", is_osx?"Cmd+C":"Ctrl+C");
            app.ui.copy_hovered = ImGui::IsItemHovered();
            ImGui::MenuItem("Paste", is_osx?"Cmd+V":"Ctrl+V");
            #if defined(__EMSCRIPTEN__)
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Please use keyboard shortcut or\nbrowser menu for pasting!");
            }
            #endif
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controls", "Alt+C", &app.ui.cpu_controls_open);
            ImGui::MenuItem("Trace Log", "Alt+T", &app.ui.tracelog_open);
            ImGui::MenuItem("Assembler", "Alt+A", &app.ui.asm_open);
            ImGui::MenuItem("Listing", "Alt+L", &app.ui.listing_open);
            ImGui::MenuItem("Memory Editor", "Alt+M", &app.ui.memedit.open);
            ImGui::MenuItem("Disassembler", "Alt+D", &app.ui.dasm.open);
            ImGui::EndMenu();
        }
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
        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Default")) {
                app.chipvis.layer_palette = { {
                    { 1.0f, 0.0f, 0.0f, 1.0f },
                    { 0.0f, 1.0f, 0.0f, 1.0f },
                    { 0.0f, 0.0f, 1.0f, 1.0f },
                    { 1.0f, 1.0f, 0.0f, 1.0f },
                    { 0.0f, 1.0f, 1.0f, 1.0f },
                    { 1.0f, 0.0f, 1.0f, 1.0f },
                } };
                app.chipvis.use_additive_blend = false;
            }
            if (ImGui::MenuItem("Visual6502")) {
                app.chipvis.layer_palette = { {
                    { 1.0f, 0.0f, 0.0f, 1.0f },
                    { 1.0f, 1.0f, 0.0f, 1.0f },
                    { 1.0f, 0.0f, 1.0f, 1.0f },
                    { 0.3f, 1.0f, 0.3f, 1.0f },
                    { 1.0f, 0.3f, 0.3f, 1.0f },
                    { 0.5f, 0.1f, 0.75f, 1.0f },
                } };
                app.chipvis.use_additive_blend = false;
            }
            if (ImGui::MenuItem("Matrix")) {
                app.chipvis.layer_palette = { {
                    { 0.0f, 0.5f, 0.0f, 1.0f },
                    { 0.0f, 0.5f, 0.0f, 1.0f },
                    { 0.0f, 0.5f, 0.0f, 1.0f },
                    { 0.0f, 0.5f, 0.0f, 1.0f },
                    { 0.0f, 0.5f, 0.0f, 1.0f },
                    { 0.0f, 0.5f, 0.0f, 1.0f },
                } };
                app.chipvis.use_additive_blend = true;
            }
            if (ImGui::MenuItem("X-Ray")) {
                app.chipvis.layer_palette = { {
                    { 0.5f, 0.5f, 0.5f, 1.0f },
                    { 0.5f, 0.5f, 0.5f, 1.0f },
                    { 0.5f, 0.5f, 0.5f, 1.0f },
                    { 0.5f, 0.5f, 0.5f, 1.0f },
                    { 0.5f, 0.5f, 0.5f, 1.0f },
                    { 0.5f, 0.5f, 0.5f, 1.0f },
                } };
                app.chipvis.use_additive_blend = true;
            }
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
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Assembler", 0, &app.ui.help_asm_open);
            ImGui::MenuItem("Opcode Table", 0, &app.ui.help_opcodes_open);
            ImGui::MenuItem("About", 0, &app.ui.help_about_open);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

static void ui_input_uint16(const char* label, const char* id, uint16_t addr) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label); ImGui::SameLine();
    sim_w16(addr, ui_util_input_u16(id, sim_r16(addr)));
}

void ui_controls(void) {
    if (!app.ui.cpu_controls_open) {
        return;
    }
    const float disp_w = (float) sapp_width();
    ImGui::SetNextWindowPos({ disp_w - 300, 50 }, ImGuiCond_Once);
    ImGui::SetNextWindowSize({ 270, 480 }, ImGuiCond_Once);
    if (ImGui::Begin("MOS 6502", &app.ui.cpu_controls_open, ImGuiWindowFlags_None)) {
        /* cassette deck controls */
        const char* tooltip = 0;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 10.0f } );
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 7.0f, 7.0f } );
        ImGui::PushStyleColor(ImGuiCol_Button, 0xFFFFFFFF);
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF000000);
        if (ImGui::Button(ICON_FA_STEP_BACKWARD, { 28, 25 })) {
            sim_pause(true);
            trace_revert_to_previous();
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step back one half-cycle";
        }
        ImGui::SameLine();
        if (sim_paused()) {
            if (ImGui::Button(ICON_FA_PLAY, { 28, 25 })) {
                sim_pause(false);
            }
            if (ImGui::IsItemHovered()) {
                tooltip = "Run (one half-cycle per frame)";
            }
        }
        else {
            if (ImGui::Button(ICON_FA_PAUSE, { 28, 25 })) {
                sim_pause(true);
            }
            if (ImGui::IsItemHovered()) {
                tooltip = "Pause";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_STEP_FORWARD, { 28, 25 })) {
            sim_pause(true);
            sim_step(1);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step one half-cycle";
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FAST_FORWARD, { 28, 25 })) {
            sim_pause(true);
            sim_step(2);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step two half-cycles";
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_RIGHT, { 28, 25 })) {
            sim_pause(true);
            sim_step_op();
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step to next instruction";
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_EJECT, { 28, 25 })) {
            trace_clear();
            sim_init_or_reset();
            sim_start();
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Reset";
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        if (!tooltip) {
            tooltip = sim_paused() ? "Paused" : "Running";
        }
        ImGui::Text("%s", tooltip);
        ImGui::Separator();

        /* CPU state */
        const uint8_t ir = sim_get_ir();
        const uint8_t p = sim_get_p();
        char p_str[9] = {
            (p & (1<<7)) ? 'N':'n',
            (p & (1<<6)) ? 'V':'v',
            (p & (1<<5)) ? 'X':'x',
            (p & (1<<4)) ? 'B':'b',
            (p & (1<<3)) ? 'D':'d',
            (p & (1<<2)) ? 'I':'i',
            (p & (1<<1)) ? 'Z':'z',
            (p & (1<<0)) ? 'C':'c',
            0,
        };
        ImGui::Text("A:%02X X:%02X Y:%02X SP:%02X PC:%04X",
            sim_get_a(), sim_get_x(), sim_get_y(), sim_get_sp(), sim_get_pc());
        ImGui::Text("P:%02X (%s) Cycle: %d", p, p_str, sim_get_cycle()>>1);
        ImGui::Text("IR:%02X %s\n", ir, util_opcode_to_str(ir));
        ImGui::Text("Data:%02X Addr:%04X %s %s %s",
            sim_get_data(), sim_get_addr(),
            sim_get_rw()?"R":"W",
            sim_get_clk0()?"CLK0":"    ",
            sim_get_sync()?"SYNC":"    ");
        ImGui::Separator();
        ui_input_uint16("NMI vector (FFFA): ", "##nmi_vec", 0xFFFA);
        ui_input_uint16("RES vector (FFFC): ", "##res_vec", 0xFFFC);
        ui_input_uint16("IRQ vector (FFFE): ", "##irq_vec", 0xFFFE);
        ImGui::Separator();
        bool rdy_active = !sim_get_rdy();
        if (ImGui::Checkbox("RDY", &rdy_active)) {
            sim_set_rdy(!rdy_active);
        }
        ImGui::SameLine();
        bool irq_active = !sim_get_irq();
        if (ImGui::Checkbox("IRQ", &irq_active)) {
            sim_set_irq(!irq_active);
        }
        ImGui::SameLine();
        bool nmi_active = !sim_get_nmi();
        if (ImGui::Checkbox("NMI", &nmi_active)) {
            sim_set_nmi(!nmi_active);
        }
        ImGui::SameLine();
        bool res_active = !sim_get_res();
        if (ImGui::Checkbox("RES", &res_active)) {
            sim_set_res(!res_active);
        }
        ImGui::Separator();

        /* memory dump */
        ImGui::Text("Memory:");
        ImGui::BeginChild("##memedit");
        ui_memedit_draw_content(&app.ui.memedit_integrated);
        ImGui::EndChild();
    }
    ImGui::End();
}

void ui_listing(void) {
    if (!app.ui.listing_open) {
        return;
    }
    ImGui::SetNextWindowPos({60, 320}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({480, 200}, ImGuiCond_Once);
    if (ImGui::Begin("Listing", &app.ui.listing_open, ImGuiWindowFlags_None)) {
        ImGui::Text("%s", asm_listing());
    }
    ImGui::End();
}

void ui_tracelog(void) {
    if (!app.ui.tracelog_open) {
        return;
    }
    // clear the selected item is outside the trace log
    if (!trace_empty()) {
        if ((app.trace.selected_cycle < trace_get_cycle(trace_num_items()-1)) ||
            (app.trace.selected_cycle > trace_get_cycle(0)))
        {
            app.trace.selected = false;
        }
    }

    const float disp_w = (float) sapp_width();
    const float disp_h = (float) sapp_height();
    const float footer_h = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::SetNextWindowPos({ disp_w / 2, disp_h - 150 }, ImGuiCond_Once, { 0.5f, 0.0f });
    ImGui::SetNextWindowSize({ 600, 128 }, ImGuiCond_Once);
    if (ImGui::Begin("Trace Log", &app.ui.tracelog_open, ImGuiWindowFlags_None)) {
        ImGui::Text("cycle/h rw ab   db pc   a  x  y  s  p        sync ir mnemonic    irq nmi res rdy"); ImGui::NextColumn();
        ImGui::Separator();
        ImGui::BeginChild("##trace_data", ImVec2(0, -footer_h));
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float text_height = ImGui::GetTextLineHeightWithSpacing();
        float window_width = ImGui::GetWindowWidth();
        bool any_item_hovered = false;
        if (trace_num_items() > 0) {
            // set mouse-hovered-cycle to an impossible value
            for (int32_t i = trace_num_items()-1; i >= 0; i--) {
                const uint32_t cur_cycle = trace_get_cycle(i);
                uint32_t bg_color = 0;
                const uint32_t flip_bits = trace_get_flipbits(i);
                switch (flip_bits) {
                    case 0: bg_color = 0xFF327D2E; break;
                    case 1: bg_color = 0xFF3C8E38; break;
                    case 2: bg_color = 0xFFC06515; break;
                    case 3: bg_color = 0xFFD17619; break;
                }
                if (app.trace.hovered && (cur_cycle == app.trace.hovered_cycle)) {
                    bg_color = 0xFF3643F4;
                }
                if (app.trace.selected && (cur_cycle == app.trace.selected_cycle)) {
                    bg_color = 0xFF0000D5;
                }
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = { p0.x + window_width, p0.y + text_height};
                dl->AddRectFilled(p0, p1, bg_color);

                const uint8_t ir = trace_get_ir(i);
                const uint8_t p = trace_get_p(i);
                char p_str[9] = {
                    (p & (1<<7)) ? 'N':'n',
                    (p & (1<<6)) ? 'V':'v',
                    (p & (1<<5)) ? 'X':'x',
                    (p & (1<<4)) ? 'B':'b',
                    (p & (1<<3)) ? 'D':'d',
                    (p & (1<<2)) ? 'I':'i',
                    (p & (1<<1)) ? 'Z':'z',
                    (p & (1<<0)) ? 'C':'c',
                    0,
                };
                ImGui::Text("%5d/%c %s  %04X %02X %04X %02X %02X %02X %02X %s %s %02X %s %s %s %s %s",
                    cur_cycle>>1,
                    trace_get_clk0(i)?'1':'0',
                    trace_get_rw(i)?"R":"W",
                    trace_get_addr(i),
                    trace_get_data(i),
                    trace_get_pc(i),
                    trace_get_a(i),
                    trace_get_x(i),
                    trace_get_y(i),
                    trace_get_sp(i),
                    p_str,
                    trace_get_sync(i)?"SYNC":"    ",
                    ir,
                    util_opcode_to_str(ir),
                    trace_get_irq(i)?"   ":"IRQ",
                    trace_get_nmi(i)?"   ":"NMI",
                    trace_get_res(i)?"   ":"RES",
                    trace_get_rdy(i)?"   ":"RDY");
                if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(p0, p1)) {
                    any_item_hovered = true;
                    app.trace.hovered_cycle = trace_get_cycle(i);
                    if (ImGui::IsMouseClicked(0)) {
                        app.trace.selected = true;
                        app.trace.selected_cycle = app.trace.hovered_cycle;
                    }
                }
            }
            if (app.ui.tracelog_scroll_to_end) {
                app.ui.tracelog_scroll_to_end = false;
                ImGui::SetScrollHere();
            }
        }
        app.trace.hovered = any_item_hovered;
        ImGui::EndChild();
        ImGui::Separator();
        if (ImGui::Button("Clear Log")) {
            trace_clear();
        }
        ImGui::SameLine();
        if (app.trace.selected) {
            if (ImGui::Button("Revert to Selected")) {
                trace_revert_to_selected();
            }
        }
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

static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data) {
    assert(data.link && (data.linkLength > 0));
    int i = 0;
    for (i = 0; (i < data.linkLength) && (i < MAX_LINKURL_SIZE); i++) {
        app.ui.link_url[i] = data.link[i];
    }
    if (i < MAX_LINKURL_SIZE) {
        app.ui.link_url[i] = 0;
        app.ui.link_hovered = true;
    }
    else {
        app.ui.link_url[0] = 0;
        app.ui.link_hovered = false;
    }
}

static ImVec2 display_center(void) {
    return ImVec2((float)sapp_width()*0.5f, (float)sapp_height()*0.5f);
}

static void ui_help_assembler(void) {
    if (!app.ui.help_asm_open) {
        return;
    }
    ImGui::SetNextWindowSize({640, 400}, ImGuiCond_Once);
    ImGui::SetNextWindowPos(display_center(), ImGuiCond_Once, { 0.5f, 0.5f });
    if (ImGui::Begin("Assembler Help", &app.ui.help_asm_open, ImGuiWindowFlags_None)) {
        ImGui::Markdown(dump_help_assembler_md, sizeof(dump_help_assembler_md)-1, md_conf);
    }
    ImGui::End();
}

static void ui_help_opcodes(void) {
    if (!app.ui.help_opcodes_open) {
        return;
    }
    ImGui::SetNextWindowSize({640, 400}, ImGuiCond_Once);
    if (ImGui::Begin("Opcode Help", &app.ui.help_opcodes_open, ImGuiWindowFlags_None)) {
        ImGui::Text("TODO!");
    }
    ImGui::End();
}

static const uint8_t c_V       [8] = { 0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00  };
static const uint8_t c_I       [8] = { 0x00,0x7E,0x18,0x18,0x18,0x18,0x7E,0x00  };
static const uint8_t c_S       [8] = { 0x00,0x3C,0x60,0x3C,0x06,0x06,0x3C,0x00  };
static const uint8_t c_U       [8] = { 0x00,0x66,0x66,0x66,0x66,0x66,0x7E,0x00  };
static const uint8_t c_A       [8] = { 0x00,0x18,0x3C,0x66,0x66,0x7E,0x66,0x00  };
static const uint8_t c_L       [8] = { 0x00,0x60,0x60,0x60,0x60,0x60,0x7E,0x00  };
static const uint8_t c_6       [8] = { 0x00,0x3C,0x60,0x7C,0x66,0x66,0x3C,0x00  };
static const uint8_t c_5       [8] = { 0x00,0x7E,0x60,0x7C,0x06,0x66,0x3C,0x00  };
static const uint8_t c_0       [8] = { 0x00,0x3C,0x66,0x6E,0x76,0x66,0x3C,0x00  };
static const uint8_t c_2       [8] = { 0x00,0x3C,0x66,0x0C,0x18,0x30,0x7E,0x00  };
static const uint8_t c_r       [8] = { 0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00  };
static const uint8_t c_e       [8] = { 0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00  };
static const uint8_t c_m       [8] = { 0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00  };
static const uint8_t c_i       [8] = { 0x00,0x18,0x00,0x38,0x18,0x18,0x3C,0x00  };
static const uint8_t c_x       [8] = { 0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00  };
static const uint8_t c_asterix [8] = { 0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00  };
static const uint8_t c_space   [8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  };

static const uint32_t colors[8] = {
    0xFF0000FF,     // red
    0xFF0088FF,     // orange
    0xFF00FFFF,     // yellow
    0xFF00FF00,     // green
    0xFF00FF00,     // green
    0xFFFFFF00,     // teal
    0xFFFF8888,     // blue
    0xFFFF88FF,     // violet
};

static float warp_time;
static ImVec2 draw_char(ImDrawList* dl, const uint8_t* bitmap, const ImVec2& pos, float r, float d, int color_offset) {
    ImVec2 p = pos;
    for (int y = 0; y < 8; y++, p.y += d) {
        p.x = pos.x + sinf(warp_time + (y+color_offset*8) * 0.25f) * 5.0f;
        uint8_t pixels = bitmap[y];
        for (int x = 7; x >= 0; x--, p.x += d) {
            if (pixels & (1<<x)) {
                dl->AddRectFilled({p.x-r,p.y-r},{p.x+r,p.y+r}, colors[y/2+color_offset*4]);
            }
        }
    }
    return { pos.x + 8 * d, pos.y };
}

static ImVec2 draw_rainbow_text(ImVec2 c_pos, float win_width) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = 1.0f;
    const float d = 3.0f;
    c_pos.x += (win_width - 19.0f * d * 8.0f) * 0.5f;
    c_pos.y += 10.0f;
    ImVec2 p = c_pos;
    for (int i = 0; i < 3; i++) {
        p = draw_char(dl, c_asterix, p, r, d, 0);
    }
    p = draw_char(dl, c_space, p, r, d, 0);
    p = draw_char(dl, c_V, p, r, d, 0);
    p = draw_char(dl, c_I, p, r, d, 0);
    p = draw_char(dl, c_S, p, r, d, 0);
    p = draw_char(dl, c_U, p, r, d, 0);
    p = draw_char(dl, c_A, p, r, d, 0);
    p = draw_char(dl, c_L, p, r, d, 0);
    p = draw_char(dl, c_space, p, r, d, 0);
    p = draw_char(dl, c_6, p, r, d, 0);
    p = draw_char(dl, c_5, p, r, d, 0);
    p = draw_char(dl, c_0, p, r, d, 0);
    p = draw_char(dl, c_2, p, r, d, 0);
    p = draw_char(dl, c_space, p, r, d, 0);
    for (int i = 0; i < 3; i++) {
        p = draw_char(dl, c_asterix, p, r, d, 0);
    }
    p = c_pos;
    p.y += 8.0f * d;
    for (int i = 0; i < 2; i++) {
        p = draw_char(dl, c_space, p, r, d, 1);
    }
    for (int i = 0; i < 3; i++) {
        p = draw_char(dl, c_asterix, p, r, d, 1);
    }
    for (int i = 0; i < 2; i++) {
        p = draw_char(dl, c_space, p, r, d, 1);
    }
    p = draw_char(dl, c_r, p, r, d, 1);
    p = draw_char(dl, c_e, p, r, d, 1);
    p = draw_char(dl, c_m, p, r, d, 1);
    p = draw_char(dl, c_i, p, r, d, 1);
    p = draw_char(dl, c_x, p, r, d, 1);
    for (int i = 0; i < 2; i++) {
        p = draw_char(dl, c_space, p, r, d, 1);
    }
    for (int i = 0; i < 3; i++) {
        p = draw_char(dl, c_asterix, p, r, d, 1);
    }
    for (int i = 0; i < 2; i++) {
        p = draw_char(dl, c_space, p, r, d, 1);
    }
    p.y += 8.0f * d;
    return p;
}

static void ui_help_about(void) {
    if (!app.ui.help_about_open) {
        return;
    }
    warp_time += 0.05f;
    ImVec2 disp_center = display_center();
    const float win_width = 560.0f;
    const float win_height = 390.0f;
    const float box_padding = 20.0f;
    ImGui::SetNextWindowSize({win_width, win_height}, ImGuiCond_Once);
    ImGui::SetNextWindowPos(disp_center, ImGuiCond_Once, { 0.5f, 0.5f });
    if (ImGui::Begin("About", &app.ui.help_about_open, ImGuiWindowFlags_NoResize)) {
        ImVec2 c_pos = ImGui::GetCursorScreenPos();
        ImVec2 p = draw_rainbow_text(c_pos, win_width);
        c_pos.y = p.y + 16.0f;
        c_pos.x += box_padding;
        ImGui::SetCursorScreenPos(c_pos);
        ImGui::BeginChild("##about", {win_width-2*box_padding, -1.0f}, false);
        ImGui::Markdown(dump_about_md, sizeof(dump_about_md)-1, md_conf);
        ImGui::EndChild();
    }
    ImGui::End();
}
