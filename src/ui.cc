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
#include "sokol_imgui.h"
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
        desc.data.subimage[0][0].ptr = font_pixels;
        desc.data.subimage[0][0].size = font_width * font_height * 4;
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
        gfx_toggle_layer_visibility(l);
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
    if (app.ui.link_hovered) {
        util_html5_cursor_to_pointer();
    }
    else {
        util_html5_cursor_to_default();
    }
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
            #if defined(__EMSCRIPTEN__)
            ImGui::MenuItem("Paste", is_osx?"Cmd+V":"Ctrl+V");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Please use keyboard shortcut or\nbrowser menu for pasting!");
            }
            #else
            if (ImGui::MenuItem("Paste", is_osx?"Cmd+V":"Ctrl+V")) {
                ui_asm_paste();
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
                ImGui::MenuItem("Layer #1##V", "Alt+1", gfx_get_layer_visibility(0));
                ImGui::MenuItem("Layer #2##V", "Alt+2", gfx_get_layer_visibility(1));
                ImGui::MenuItem("Layer #3##V", "Alt+3", gfx_get_layer_visibility(2));
                ImGui::MenuItem("Layer #4##V", "Alt+4", gfx_get_layer_visibility(3));
                ImGui::MenuItem("Layer #5##V", "Alt+5", gfx_get_layer_visibility(4));
                ImGui::MenuItem("Layer #6##V", "Alt+6", gfx_get_layer_visibility(5));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Pickable")) {
                ImGui::MenuItem("Layer #1##P", 0, &app.pick.layer_enabled[0]);
                ImGui::MenuItem("Layer #2##P", 0, &app.pick.layer_enabled[1]);
                ImGui::MenuItem("Layer #3##P", 0, &app.pick.layer_enabled[2]);
                ImGui::MenuItem("Layer #4##P", 0, &app.pick.layer_enabled[3]);
                ImGui::MenuItem("Layer #5##P", 0, &app.pick.layer_enabled[4]);
                ImGui::MenuItem("Layer #6##P", 0, &app.pick.layer_enabled[5]);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Default")) {
                gfx_set_layer_palette(false, (gfx_palette_t){
                    .colors = {
                        { 1.0f, 0.0f, 0.0f, 1.0f },
                        { 0.0f, 1.0f, 0.0f, 1.0f },
                        { 0.0f, 0.0f, 1.0f, 1.0f },
                        { 1.0f, 1.0f, 0.0f, 1.0f },
                        { 0.0f, 1.0f, 1.0f, 1.0f },
                        { 1.0f, 0.0f, 1.0f, 1.0f },
                    }
                });
            }
            if (ImGui::MenuItem("Visual6502")) {
                gfx_set_layer_palette(false, (gfx_palette_t){
                    .colors = {
                        { 1.0f, 0.0f, 0.0f, 1.0f },
                        { 1.0f, 1.0f, 0.0f, 1.0f },
                        { 1.0f, 0.0f, 1.0f, 1.0f },
                        { 0.3f, 1.0f, 0.3f, 1.0f },
                        { 1.0f, 0.3f, 0.3f, 1.0f },
                        { 0.5f, 0.1f, 0.75f, 1.0f },
                    }
                });
            }
            if (ImGui::MenuItem("Matrix")) {
                gfx_set_layer_palette(true, (gfx_palette_t){
                    .colors = {
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                        { 0.0f, 0.5f, 0.0f, 1.0f },
                    }
                });
            }
            if (ImGui::MenuItem("X-Ray")) {
                gfx_set_layer_palette(true, (gfx_palette_t){
                    .colors = {
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                        { 0.5f, 0.5f, 0.5f, 1.0f },
                    }
                });
            }
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
            sim_pause(&app.sim, true);
            trace_revert_to_previous(&app.trace, &app.sim);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step back one half-cycle";
        }
        ImGui::SameLine();
        if (sim_paused(&app.sim)) {
            if (ImGui::Button(ICON_FA_PLAY, { 28, 25 })) {
                sim_pause(&app.sim, false);
            }
            if (ImGui::IsItemHovered()) {
                tooltip = "Run (one half-cycle per frame)";
            }
        }
        else {
            if (ImGui::Button(ICON_FA_PAUSE, { 28, 25 })) {
                sim_pause(&app.sim, true);
            }
            if (ImGui::IsItemHovered()) {
                tooltip = "Pause";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_STEP_FORWARD, { 28, 25 })) {
            sim_pause(&app.sim, true);
            sim_step(&app.sim, &app.trace, 1);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step one half-cycle";
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FAST_FORWARD, { 28, 25 })) {
            sim_pause(&app.sim, true);
            sim_step(&app.sim, &app.trace, 2);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step two half-cycles";
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_RIGHT, { 28, 25 })) {
            sim_pause(&app.sim, true);
            sim_step_op(&app.sim, &app.trace);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Step to next instruction";
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_EJECT, { 28, 25 })) {
            trace_clear(&app.trace);
            sim_init_or_reset(&app.sim);
            sim_start(&app.sim, &app.trace);
        }
        if (ImGui::IsItemHovered()) {
            tooltip = "Reset";
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        if (!tooltip) {
            tooltip = sim_paused(&app.sim) ? "Paused" : "Running";
        }
        ImGui::Text("%s", tooltip);
        ImGui::Separator();

        /* CPU state */
        const uint8_t ir = sim_get_ir(&app.sim);
        const uint8_t p = sim_get_p(&app.sim);
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
            sim_get_a(&app.sim), sim_get_x(&app.sim), sim_get_y(&app.sim), sim_get_sp(&app.sim), sim_get_pc(&app.sim));
        ImGui::Text("P:%02X (%s) Cycle: %d", p, p_str, sim_get_cycle()>>1);
        ImGui::Text("IR:%02X %s\n", ir, util_opcode_to_str(ir));
        ImGui::Text("Data:%02X Addr:%04X %s %s %s",
            sim_get_data(&app.sim), sim_get_addr(&app.sim),
            sim_get_rw(&app.sim)?"R":"W",
            sim_get_clk0(&app.sim)?"CLK0":"    ",
            sim_get_sync(&app.sim)?"SYNC":"    ");
        ImGui::Separator();
        ui_input_uint16("NMI vector (FFFA): ", "##nmi_vec", 0xFFFA);
        ui_input_uint16("RES vector (FFFC): ", "##res_vec", 0xFFFC);
        ui_input_uint16("IRQ vector (FFFE): ", "##irq_vec", 0xFFFE);
        ImGui::Separator();
        bool rdy_active = !sim_get_rdy(&app.sim);
        if (ImGui::Checkbox("RDY", &rdy_active)) {
            sim_set_rdy(&app.sim, !rdy_active);
        }
        ImGui::SameLine();
        bool irq_active = !sim_get_irq(&app.sim);
        if (ImGui::Checkbox("IRQ", &irq_active)) {
            sim_set_irq(&app.sim, !irq_active);
        }
        ImGui::SameLine();
        bool nmi_active = !sim_get_nmi(&app.sim);
        if (ImGui::Checkbox("NMI", &nmi_active)) {
            sim_set_nmi(&app.sim, !nmi_active);
        }
        ImGui::SameLine();
        bool res_active = !sim_get_res(&app.sim);
        if (ImGui::Checkbox("RES", &res_active)) {
            sim_set_res(&app.sim, !res_active);
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
    if (!trace_empty(&app.trace)) {
        if ((app.trace.ui.selected_cycle < trace_get_cycle(&app.trace, trace_num_items(&app.trace)-1)) ||
            (app.trace.ui.selected_cycle > trace_get_cycle(&app.trace, 0)))
        {
            app.trace.ui.selected = false;
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
        if (trace_num_items(&app.trace) > 0) {
            // set mouse-hovered-cycle to an impossible value
            for (int32_t i = trace_num_items(&app.trace)-1; i >= 0; i--) {
                const uint32_t cur_cycle = trace_get_cycle(&app.trace, i);
                uint32_t bg_color = 0;
                const uint32_t flip_bits = trace_get_flipbits(&app.trace, i);
                switch (flip_bits) {
                    case 0: bg_color = 0xFF327D2E; break;
                    case 1: bg_color = 0xFF3C8E38; break;
                    case 2: bg_color = 0xFFC06515; break;
                    case 3: bg_color = 0xFFD17619; break;
                }
                if (app.trace.ui.hovered && (cur_cycle == app.trace.ui.hovered_cycle)) {
                    bg_color = 0xFF3643F4;
                }
                if (app.trace.ui.selected && (cur_cycle == app.trace.ui.selected_cycle)) {
                    bg_color = 0xFF0000D5;
                }
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                ImVec2 p1 = { p0.x + window_width, p0.y + text_height};
                dl->AddRectFilled(p0, p1, bg_color);

                const uint8_t ir = trace_get_ir(&app.trace, i);
                const uint8_t p = trace_get_p(&app.trace, i);
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
                    trace_get_clk0(&app.trace, i)?'1':'0',
                    trace_get_rw(&app.trace, i)?"R":"W",
                    trace_get_addr(&app.trace, i),
                    trace_get_data(&app.trace, i),
                    trace_get_pc(&app.trace, i),
                    trace_get_a(&app.trace, i),
                    trace_get_x(&app.trace, i),
                    trace_get_y(&app.trace, i),
                    trace_get_sp(&app.trace, i),
                    p_str,
                    trace_get_sync(&app.trace, i)?"SYNC":"    ",
                    ir,
                    util_opcode_to_str(ir),
                    trace_get_irq(&app.trace, i)?"   ":"IRQ",
                    trace_get_nmi(&app.trace, i)?"   ":"NMI",
                    trace_get_res(&app.trace, i)?"   ":"RES",
                    trace_get_rdy(&app.trace, i)?"   ":"RDY");
                if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(p0, p1)) {
                    any_item_hovered = true;
                    app.trace.ui.hovered_cycle = trace_get_cycle(&app.trace, i);
                    if (ImGui::IsMouseClicked(0)) {
                        app.trace.ui.selected = true;
                        app.trace.ui.selected_cycle = app.trace.ui.hovered_cycle;
                    }
                }
            }
            if (app.trace.ui.log_scroll_to_end) {
                app.trace.ui.log_scroll_to_end = false;
                ImGui::SetScrollHereY();
            }
        }
        app.trace.ui.hovered = any_item_hovered;
        ImGui::EndChild();
        ImGui::Separator();
        if (ImGui::Button("Clear Log")) {
            trace_clear(&app.trace);
        }
        ImGui::SameLine();
        if (app.trace.ui.selected) {
            if (ImGui::Button("Revert to Selected")) {
                trace_revert_to_selected(&app.trace, &app.sim);
            }
        }
    }
    ImGui::End();
}

void ui_picking(void) {
    if (app.pick.result.num_hits > 0) {
        char str[256] = { 0 };
        for (int i = 0; i < app.pick.result.num_hits; i++) {
            int node_index = app.pick.result.node_index[i];
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

// Atari 8-bitter font extracted from ROM listing, starting at 0x20
static const uint8_t atari_font[96][8] = {
    { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },    // - space
    { 0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x00 },    // - !
    { 0x00,0x66,0x66,0x66,0x00,0x00,0x00,0x00 },    // - "
    { 0x00,0x66,0xFF,0x66,0x66,0xFF,0x66,0x00 },    // - #
    { 0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00 },    // - $
    { 0x00,0x66,0x6C,0x18,0x30,0x66,0x46,0x00 },    // - %
    { 0x1C,0x36,0x1C,0x38,0x6F,0x66,0x3B,0x00 },    // - &
    { 0x00,0x18,0x18,0x18,0x00,0x00,0x00,0x00 },    // - '
    { 0x00,0x0E,0x1C,0x18,0x18,0x1C,0x0E,0x00 },    // - (
    { 0x00,0x70,0x38,0x18,0x18,0x38,0x70,0x00 },    // - )
    { 0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00 },    // - asterisk
    { 0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00 },    // - plus
    { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30 },    // - comma
    { 0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00 },    // - minus
    { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00 },    // - period
    { 0x00,0x06,0x0C,0x18,0x30,0x60,0x40,0x00 },    // - /
    { 0x00,0x3C,0x66,0x6E,0x76,0x66,0x3C,0x00 },    // - 0
    { 0x00,0x18,0x38,0x18,0x18,0x18,0x7E,0x00 },    // - 1
    { 0x00,0x3C,0x66,0x0C,0x18,0x30,0x7E,0x00 },    // - 2
    { 0x00,0x7E,0x0C,0x18,0x0C,0x66,0x3C,0x00 },    // - 3
    { 0x00,0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x00 },    // - 4
    { 0x00,0x7E,0x60,0x7C,0x06,0x66,0x3C,0x00 },    // - 5
    { 0x00,0x3C,0x60,0x7C,0x66,0x66,0x3C,0x00 },    // - 6
    { 0x00,0x7E,0x06,0x0C,0x18,0x30,0x30,0x00 },    // - 7
    { 0x00,0x3C,0x66,0x3C,0x66,0x66,0x3C,0x00 },    // - 8
    { 0x00,0x3C,0x66,0x3E,0x06,0x0C,0x38,0x00 },    // - 9
    { 0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00 },    // - colon
    { 0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x30 },    // - semicolon
    { 0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00 },    // - <
    { 0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00 },    // - =
    { 0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00 },    // - >
    { 0x00,0x3C,0x66,0x0C,0x18,0x00,0x18,0x00 },    // - ?
    { 0x00,0x3C,0x66,0x6E,0x6E,0x60,0x3E,0x00 },    // - @
    { 0x00,0x18,0x3C,0x66,0x66,0x7E,0x66,0x00 },    // - A
    { 0x00,0x7C,0x66,0x7C,0x66,0x66,0x7C,0x00 },    // - B
    { 0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00 },    // - C
    { 0x00,0x78,0x6C,0x66,0x66,0x6C,0x78,0x00 },    // - D
    { 0x00,0x7E,0x60,0x7C,0x60,0x60,0x7E,0x00 },    // - E
    { 0x00,0x7E,0x60,0x7C,0x60,0x60,0x60,0x00 },    // - F
    { 0x00,0x3E,0x60,0x60,0x6E,0x66,0x3E,0x00 },    // - G
    { 0x00,0x66,0x66,0x7E,0x66,0x66,0x66,0x00 },    // - H
    { 0x00,0x7E,0x18,0x18,0x18,0x18,0x7E,0x00 },    // - I
    { 0x00,0x06,0x06,0x06,0x06,0x66,0x3C,0x00 },    // - J
    { 0x00,0x66,0x6C,0x78,0x78,0x6C,0x66,0x00 },    // - K
    { 0x00,0x60,0x60,0x60,0x60,0x60,0x7E,0x00 },    // - L
    { 0x00,0x63,0x77,0x7F,0x6B,0x63,0x63,0x00 },    // - M
    { 0x00,0x66,0x76,0x7E,0x7E,0x6E,0x66,0x00 },    // - N
    { 0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00 },    // - O
    { 0x00,0x7C,0x66,0x66,0x7C,0x60,0x60,0x00 },    // - P
    { 0x00,0x3C,0x66,0x66,0x66,0x6C,0x36,0x00 },    // - Q
    { 0x00,0x7C,0x66,0x66,0x7C,0x6C,0x66,0x00 },    // - R
    { 0x00,0x3C,0x60,0x3C,0x06,0x06,0x3C,0x00 },    // - S
    { 0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x00 },    // - T
    { 0x00,0x66,0x66,0x66,0x66,0x66,0x7E,0x00 },    // - U
    { 0x00,0x66,0x66,0x66,0x66,0x3C,0x18,0x00 },    // - V
    { 0x00,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00 },    // - W
    { 0x00,0x66,0x66,0x3C,0x3C,0x66,0x66,0x00 },    // - X
    { 0x00,0x66,0x66,0x3C,0x18,0x18,0x18,0x00 },    // - Y
    { 0x00,0x7E,0x0C,0x18,0x30,0x60,0x7E,0x00 },    // - Z
    { 0x00,0x1E,0x18,0x18,0x18,0x18,0x1E,0x00 },    // - [
    { 0x00,0x40,0x60,0x30,0x18,0x0C,0x06,0x00 },    // - backslash
    { 0x00,0x78,0x18,0x18,0x18,0x18,0x78,0x00 },    // - ]
    { 0x00,0x08,0x1C,0x36,0x63,0x00,0x00,0x00 },    // - ^
    { 0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00 },    // - underline
    { 0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00 },    // - Spanish ! (should be `)
    { 0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00 },    // - a
    { 0x00,0x60,0x60,0x7C,0x66,0x66,0x7C,0x00 },    // - b
    { 0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00 },    // - c
    { 0x00,0x06,0x06,0x3E,0x66,0x66,0x3E,0x00 },    // - d
    { 0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00 },    // - e
    { 0x00,0x0E,0x18,0x3E,0x18,0x18,0x18,0x00 },    // - f
    { 0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C },    // - g
    { 0x00,0x60,0x60,0x7C,0x66,0x66,0x66,0x00 },    // - h
    { 0x00,0x18,0x00,0x38,0x18,0x18,0x3C,0x00 },    // - i
    { 0x00,0x06,0x00,0x06,0x06,0x06,0x06,0x3C },    // - j
    { 0x00,0x60,0x60,0x6C,0x78,0x6C,0x66,0x00 },    // - k
    { 0x00,0x38,0x18,0x18,0x18,0x18,0x3C,0x00 },    // - l
    { 0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00 },    // - m
    { 0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00 },    // - n
    { 0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00 },    // - o
    { 0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60 },    // - p
    { 0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06 },    // - q
    { 0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00 },    // - r
    { 0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00 },    // - s
    { 0x00,0x18,0x7E,0x18,0x18,0x18,0x0E,0x00 },    // - t
    { 0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00 },    // - u
    { 0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00 },    // - v
    { 0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00 },    // - w
    { 0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00 },    // - x
    { 0x00,0x00,0x66,0x66,0x66,0x3E,0x0C,0x78 },    // - y
    { 0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00 },    // - z
    { 0x66,0x66,0x18,0x3C,0x66,0x7E,0x66,0x00 },    // - umlaut A
    { 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18 },    // - |
    { 0x00,0x7E,0x78,0x7C,0x6E,0x66,0x06,0x00 },    // - display clear
    { 0x08,0x18,0x38,0x78,0x38,0x18,0x08,0x00 },    // - display backspace
    { 0x10,0x18,0x1C,0x1E,0x1C,0x18,0x10,0x00 },    // - display tab
};

static const uint32_t pal_0[8] = {
    0x000000FF,     // red
    0x000000FF,     // red
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x0000FF00,     // green
    0x0000FF00,     // green
};

static const uint32_t pal_1[8] = {
    0x0000FF00,     // green
    0x0000FF00,     // green
    0x00FFFF00,     // teal
    0x00FFFF00,     // teal
    0x00FF8888,     // blue
    0x00FF8888,     // blue
    0x00FF88FF,     // violet
    0x00FF88FF,     // violet
};

static void draw_string_wobble(ImDrawList* dl, const char* str, ImVec2 pos, float r, float d, const uint32_t* palette, float time, float warp0, float warp1, float warp2, float alpha) {
    uint32_t a = ((uint32_t)(255.0f * alpha))<<24;
    uint8_t chr;
    int chr_index = 0;
    while ((chr = (uint8_t)*str++) != 0) {
        uint8_t font_index = chr - 0x20;
        if (font_index >= 96) {
            continue;
        }
        float py = pos.y;
        for (int y = 0; y < 8; y++, py += d) {
            float px = pos.x + chr_index*8*d + sinf((time + (py * warp0)) * warp1) * warp2;
            uint8_t pixels = atari_font[font_index][y];
            for (int x = 7; x >= 0; x--, px += d) {
                if (pixels & (1<<x)) {
                    dl->AddRectFilled({px-r,py-r},{px+r,py+r}, palette[y]|a);
                }
            }
        }
        chr_index++;
    }
}

static float hdr_time;
static ImVec2 draw_header(ImVec2 c_pos, float win_width) {
    hdr_time += 0.05f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = 1.0f;
    const float d = 3.0f;
    c_pos.x += (win_width - 19.0f * d * 8.0f) * 0.5f;
    c_pos.y += 10.0f;
    ImVec2 p = c_pos;
    draw_string_wobble(dl, "*** VISUAL 6502 ***", p, r, d, pal_0, hdr_time, 0.1f, 1.0f, 5.0f, 1.0f);
    p = { c_pos.x, c_pos.y + 8.0f * d };
    draw_string_wobble(dl, " ***   remix   ***", p, r, d, pal_1, hdr_time, 0.1f, 1.0f, 5.0f, 1.0f);
    p = { c_pos.x, c_pos.y + 16.0f * d };
    return p;
}

#define NUM_FOOTERS (26)
static const char* footers[NUM_FOOTERS] = {
    "SOWACO proudly presents",
    "a 2019 production",
    "VISUAL 6502 REMIX",
    "Built with:",
    "visual6502",
    "perfect6502",
    "Dear ImGui",
    "ImGuiColorTextEdit",
    "ImGuiMarkdown",
    "ASMX",
    "Sokol Headers",
    "Chips Headers",
    "Tripy",
    "IconFontCppHeaders",
    "Font Awesome 4",
    "Coded in C99",
    "(and a bit of C++)",
    "(and some Python)",
    "(even some JS!)",
    "compiled to WASM with",
    "EMSCRIPTEN",
    "Special Kudos to:",
    "The visual6502.org Team",
    "for reversing the 6502!",
    "...and of course...",
    "6502 4EVER!!!",
};

static const uint32_t pal_2[32] = {
    0x000000FF,     // red
    0x000000FF,     // red
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x0000FF00,     // green
    0x0000FF00,     // green
    0x00FFFF00,     // teal
    0x00FFFF00,     // teal
    0x00FF8888,     // blue
    0x00FF8888,     // blue
    0x00FF88FF,     // violet
    0x00FF88FF,     // violet
    0x00FF8888,     // blue
    0x00FF8888,     // blue
    0x00FFFF00,     // teal
    0x00FFFF00,     // teal
    0x0000FF00,     // green
    0x0000FF00,     // green
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x000000FF,     // red
    0x000000FF,     // red
    0x000088FF,     // orange
    0x000088FF,     // orange
    0x0000FFFF,     // yellow
    0x0000FFFF,     // yellow
    0x0000FF00,     // green
    0x0000FF00,     // green
};


static float footer_time;
static uint32_t footer_frame_count;
static void draw_footer(ImVec2 c_pos, float win_width) {
    footer_frame_count++;
    // a small delay after opening the about box
    if (footer_frame_count < 60) {
        return;
    }
    const float time_per_msg = 4.0f;
    // stop at last message
    if (footer_time < ((NUM_FOOTERS * time_per_msg) - 1.0f)) {
        footer_time += 1.0f / 60.0f;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    //const float r = 1.0f;
    uint32_t msg_index = (uint32_t) (footer_time / time_per_msg);
    if (msg_index >= NUM_FOOTERS) {
        msg_index = NUM_FOOTERS;
    }
    const char* msg = footers[msg_index];
    float x = (c_pos.x + win_width * 0.5f);
    float y = (c_pos.y + 4 * 3.0f);

    float t = fmodf(footer_time, time_per_msg);
    float t1 = 0.0f;
    if (t < 1.0f) {
        t1 = 1.0f - t*t*t;
    }
    else if (t > (time_per_msg - 1.0f)) {
        t -= (time_per_msg - 1.0f);
        t1 = t*t*t;
    }
    float d = (30.0f * t1) + 3.0f;
    const float r = 1.0f;
    x -= strlen(msg) * 8 * d * 0.5f;
    y -= 4 * d;
    draw_string_wobble(dl, msg, {x,y}, r, d, &pal_2[23-((footer_frame_count/2)%24)], footer_time*10.0f, 5.0f, 1.0f, 1000.0f * t1, 1.0f - t1);
}

static void ui_help_about(void) {
    if (!app.ui.help_about_open) {
        footer_time = 0.0f;
        footer_frame_count = 0;
        return;
    }
    ImVec2 disp_center = display_center();
    const float win_width = 600.0f;
    const float win_height = 420.0f;
    const float box_padding = 40.0f;
    ImGui::SetNextWindowSize({win_width, win_height}, ImGuiCond_Once);
    ImGui::SetNextWindowPos(disp_center, ImGuiCond_Once, { 0.5f, 0.5f });
    if (ImGui::Begin("About", &app.ui.help_about_open, ImGuiWindowFlags_NoResize)) {
        ImVec2 c_pos = ImGui::GetCursorScreenPos();
        ImVec2 p = draw_header(c_pos, win_width);
        c_pos.y = p.y + 16.0f;
        c_pos.x += box_padding;
        ImGui::SetCursorScreenPos(c_pos);
        ImGui::BeginChild("##about", {win_width-2*box_padding, -40.0f}, false);
        ImGui::Markdown(dump_about_md, sizeof(dump_about_md)-1, md_conf);
        ImGui::EndChild();
        c_pos = ImGui::GetCursorScreenPos();
        c_pos.x -= 10.0f;
        draw_footer(c_pos, win_width);
    }
    ImGui::End();
}
